/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.

-------------------------------------------------------------------------
History:
  October 2010 : Jens Schöbel added CFlowNode_MoveOverlay

*************************************************************************/

#include "StdAfx.h"
#include "SimulatePlayerInputNode.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////
CFlowNode_SimulatePlayerInput::CFlowNode_SimulatePlayerInput(SActivationInfo* pActInfo)
{

}

void CFlowNode_SimulatePlayerInput::GetConfiguration( SFlowNodeConfig& config )
{
	static const SInputPortConfig in_ports[] = 
	{
		InputPortConfig<string>( "ActionInput", _HELP("Player Action input to trigger" ), _HELP("PlayerAction"), _UICONFIG("enum_global:input_actions")),
		InputPortConfig_Void( "Press", _HELP("Simulate press" )),
		InputPortConfig_Void( "Hold", _HELP("Simulate hold event")),
		InputPortConfig_Void( "Release", _HELP("Simulate release" )),
		InputPortConfig<float>( "Value", 0.0f, _HELP("Value between 0 and 1" )),
		{0}
	};

	static const SOutputPortConfig out_ports[] = 
	{
		OutputPortConfig_Void( "Pressed", _HELP("Pressed" )),
		OutputPortConfig_Void( "Held", _HELP("Held")),
		OutputPortConfig_Void( "Released", _HELP("Released" )),
		{0}
	};

	config.sDescription = _HELP("Simulate player actions input");
	config.pInputPorts = in_ports;
	config.pOutputPorts = out_ports;
	config.SetCategory(EFLN_APPROVED);
}

void CFlowNode_SimulatePlayerInput::ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo )
{
	if (event != eFE_Activate)
		return;

	bool isPressPort = IsPortActive(pActInfo, eInputPorts_Press);
	bool isReleasePort = IsPortActive(pActInfo, eInputPorts_Release);
	bool isHoldPort = IsPortActive(pActInfo, eInputPorts_Hold);

	bool triggerResponse = false;
	int playerInputEvent;
	int nodeOutput;

	if (isPressPort)
	{
		triggerResponse = true;
		playerInputEvent = eAAM_OnPress;
		nodeOutput = eOutputPort_Pressed;
	}
	else 
	if (isHoldPort)
	{
		triggerResponse = true;
		playerInputEvent = eAAM_OnHold;
		nodeOutput = eOutputPort_Held;
	}
	else 
	if (isReleasePort)
	{
		triggerResponse = true;
		playerInputEvent = eAAM_OnRelease;
		nodeOutput = eOutputPort_Released;
	}

	if (triggerResponse)
	{
		CActor* pClientActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());

		if (pClientActor)
		{
			if (pClientActor->GetActorClass() == CPlayer::GetActorClassType())
			{
				CPlayer* pClientPlayer = static_cast<CPlayer*>(pClientActor);
				const char* action = GetPortString(pActInfo, eInputPorts_Action).c_str();
				const float inputValue = GetPortFloat(pActInfo, eInputPorts_Value);

				IPlayerInput* pPlayerInput = pClientPlayer->GetPlayerInput();
				if (pPlayerInput)
				{
					pPlayerInput->OnAction(CCryName(action), playerInputEvent, isPressPort ? clamp_tpl(inputValue, 0.0f, 1.0f) : 0.0f);
				}
			}
		}

		ActivateOutput(pActInfo, nodeOutput, true);
	}
}

void CFlowNode_SimulatePlayerInput::GetMemoryUsage( ICrySizer* sizer ) const
{
	sizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////
class CFlowNode_MoveOverlay : public CFlowBaseNode<eNCT_Singleton>
{
  enum INPUTS 
  {
    eIP_ENABLE = 0,
    eIP_DISABLE,
    eIP_MOVE_X,
    eIP_MOVE_Y,
    eIP_WEIGHT
  };

public:
  CFlowNode_MoveOverlay( SActivationInfo * pActInfo ){}

  void GetConfiguration( SFlowNodeConfig& config )
  {
    static const SInputPortConfig inputs[] = 
    {
      InputPortConfig_Void("Enables", _HELP("Enables Overlay and Sets the parameters" )),
      InputPortConfig_Void("Disable", _HELP("Disables the overlay" )),
      InputPortConfig<float>("MoveX", 0.f, _HELP("Input Left/Right (positive is left)"), 0, _UICONFIG("v_min=-1,v_max=1") ),
      InputPortConfig<float>("MoveY", 0.5f, _HELP("Input Forward/Backward (positive is forward)"), 0, _UICONFIG("v_min=-1,v_max=1") ),
      InputPortConfig<float>("Weight", 0.5f, _HELP("Tells how strong the overlay is. (0 = no Overlay, 1 = Full Overlay)"), 0, _UICONFIG("v_min=0,v_max=1") ),
      {0}
    };
    static const SOutputPortConfig outputs[] = 
    {
      {0}
    };

    config.nFlags &= ~EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Inputs movement that overlays with the players input and mixes the movements.");
    config.SetCategory(EFLN_APPROVED);
  }

  //////////////////////////////////////////////////////////////////////////
  void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
  {
    switch (event)
    {	
    case eFE_Initialize:
      DisableOverlay();
      break;
    case eFE_Activate:
      ProvessActivate(pActInfo);
      break;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  void DisableOverlay()
  {
    CActor* pClientActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
    if (pClientActor)
    {
      if (pClientActor->GetActorClass() == CPlayer::GetActorClassType())
      {
        CPlayer* pClientPlayer = static_cast<CPlayer*>(pClientActor);
        IPlayerInput* pPlayerInput = pClientPlayer->GetPlayerInput();

        if (pPlayerInput)
        {
          pPlayerInput->OnAction(CCryName("move_overlay_disable"), eAAM_OnPress, 0.f);
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  void SendOverlayData(SActivationInfo *pActInfo)
  {
    CActor* pClientActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
    if (pClientActor)
    {
      if (pClientActor->GetActorClass() == CPlayer::GetActorClassType())
      {
        CPlayer* pClientPlayer = static_cast<CPlayer*>(pClientActor);
        IPlayerInput* pPlayerInput = pClientPlayer->GetPlayerInput();

        if (pPlayerInput)
        {
          const float moveX  = GetPortFloat(pActInfo, eIP_MOVE_X);
          const float moveY  = GetPortFloat(pActInfo, eIP_MOVE_Y);
          const float weight = GetPortFloat(pActInfo, eIP_WEIGHT);

          pPlayerInput->OnAction(CCryName("move_overlay_enable"), eAAM_OnHold, 0.f);
          pPlayerInput->OnAction(CCryName("move_overlay_x"), eAAM_OnHold, moveX);
          pPlayerInput->OnAction(CCryName("move_overlay_y"), eAAM_OnHold, moveY);
          pPlayerInput->OnAction(CCryName("move_overlay_weight"), eAAM_OnHold, weight);
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  void ProvessActivate(SActivationInfo *pActInfo)
  {
    if (IsPortActive(pActInfo, eIP_ENABLE))
    {
      SendOverlayData(pActInfo);
    }
    else if (IsPortActive(pActInfo, eIP_DISABLE))
    {
      DisableOverlay();
    }
  }

  //////////////////////////////////////////////////////////////////////////
  virtual void GetMemoryUsage(ICrySizer * s) const
  {
    s->Add(*this);
  }
};
REGISTER_FLOW_NODE("Input:MoveOverlay", CFlowNode_MoveOverlay);
REGISTER_FLOW_NODE("Input:SimulatePlayerInput", CFlowNode_SimulatePlayerInput);
