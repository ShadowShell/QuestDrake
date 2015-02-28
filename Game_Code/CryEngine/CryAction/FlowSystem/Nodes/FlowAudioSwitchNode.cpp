////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
//
// -------------------------------------------------------------------------
//  File name:   FlowAudioSwitchNode.cpp
//  Version:     v1.00
//  Created:     13/05/2014 by Mikhail Korotyaev
//  Description: FlowGraph Node that sets AudioSwitches.
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioSwitch : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_AudioSwitch(SActivationInfo* pActInfo)
		:	m_nCurrentState(0)
		, m_nSwitchID(INVALID_AUDIO_CONTROL_ID)
	{
		//sanity checks
		assert((eIn_SwitchStateNameLast - eIn_SwitchStateNameFirst) == (NUM_STATES - 1));
		assert((eIn_SetStateLast - eIn_SetStateFirst) == (NUM_STATES - 1));

		for (uint32 i = 0; i < NUM_STATES; ++i)
		{
			m_aSwitchStates[i] = INVALID_AUDIO_SWITCH_STATE_ID;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioSwitch() {}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioSwitch(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS : uint32
	{
		eIn_SwitchName,

		eIn_SwitchStateNameFirst,
		eIn_SwitchStateName2,
		eIn_SwitchStateName3,
		eIn_SwitchStateNameLast,

		eIn_SetStateFirst,
		eIn_SetState2,
		eIn_SetState3,
		eIn_SetStateLast,
	};

	enum OUTPUTS 
	{
	};

	VIRTUAL void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = 
		{
			InputPortConfig<string>("audioSwitch_SwitchName", _HELP("name of the Audio Switch"), "Switch"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName1", _HELP("name of a Switch State"), "State1"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName2", _HELP("name of a Switch State"), "State2"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName3", _HELP("name of a Switch State"), "State3"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName4", _HELP("name of a Switch State"), "State4"),

			InputPortConfig_Void("audioSwitchState_SetState1", _HELP("Sets the switch to the corresponding state"), "SetState1"),
			InputPortConfig_Void("audioSwitchState_SetState2", _HELP("Sets the switch to the corresponding state"), "SetState2"),
			InputPortConfig_Void("audioSwitchState_SetState3", _HELP("Sets the switch to the corresponding state"), "SetState3"),
			InputPortConfig_Void("audioSwitchState_SetState4", _HELP("Sets the switch to the corresponding state"), "SetState4"),
			{0}
		};

		static const SOutputPortConfig outputs[] = 
		{
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node allows one to set Audio Switches.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		uint32 nCurrentState = m_nCurrentState;
		ser.BeginGroup("FlowAudioSwitchNode");
		ser.Value("current_state", nCurrentState);
		ser.EndGroup();

		if (ser.IsReading())
		{
			m_nCurrentState = 0;
			Init(pActInfo, nCurrentState);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Init(pActInfo, 0);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_SwitchName))
				{
					GetSwitchID(pActInfo);
				}

				for (uint32 iStateName = eIn_SwitchStateNameFirst; iStateName <= eIn_SwitchStateNameLast; ++iStateName)
				{
					if (IsPortActive(pActInfo, iStateName))
					{
						GetSwitchStateID(pActInfo, iStateName);
					}
				}

				for (uint32 iStatePort = eIn_SetStateFirst; iStatePort <= eIn_SetStateLast; ++iStatePort)
				{
					if (IsPortActive(pActInfo, iStatePort))
					{
						SetState(pActInfo->pEntity, iStatePort - eIn_SetStateFirst + 1);
						break;// short-circuit behaviour: set the first state activated and stop
					}
				}

				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:

	static const int NUM_STATES = 4;

	//////////////////////////////////////////////////////////////////////////
	void GetSwitchID(SActivationInfo* const pActInfo)  
	{
		string const& rRtpcName = GetPortString(pActInfo, eIn_SwitchName);
		if (!rRtpcName.empty())
		{
			gEnv->pAudioSystem->GetAudioSwitchID(rRtpcName.c_str(), m_nSwitchID);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void GetSwitchStateID(SActivationInfo* const pActInfo, uint32 const nStateInputIdx)
	{
		string const& rStateName = GetPortString(pActInfo, nStateInputIdx);
		if (!rStateName.empty() && (m_nSwitchID != INVALID_AUDIO_CONTROL_ID))
		{
			gEnv->pAudioSystem->GetAudioSwitchStateID(
				m_nSwitchID,
				rStateName.c_str(),
				m_aSwitchStates[nStateInputIdx-eIn_SwitchStateNameFirst]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo, uint32 const nCurrentState)
	{
		if (gEnv->pAudioSystem != NULL)
		{
			GetSwitchID(pActInfo);

			if (m_nSwitchID != INVALID_AUDIO_CONTROL_ID)
			{
				for (uint32 iStateName = eIn_SwitchStateNameFirst; iStateName <= eIn_SwitchStateNameLast; ++iStateName)
				{
					GetSwitchStateID(pActInfo, iStateName);
				}
			}

			m_oRequest.pData = &m_oRequestData;
			m_oRequestData.nSwitchID = m_nSwitchID;

			m_nCurrentState = 0;

			if (nCurrentState != 0)
			{
				SetState(pActInfo->pEntity, nCurrentState);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetStateOnProxy(IEntity* const pEntity, uint32 const nStateIdx)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

		if (pIEntityAudioProxy != NPTR)
		{
			pIEntityAudioProxy->SetSwitchState(m_nSwitchID, m_aSwitchStates[nStateIdx]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetStateOnGlobalObject(uint32 const nStateIdx)
	{
		m_oRequestData.nStateID = m_aSwitchStates[nStateIdx];

		gEnv->pAudioSystem->PushRequest(m_oRequest);
	}

	//////////////////////////////////////////////////////////////////////////
	void SetState(IEntity* const pEntity, int const nNewState)
	{
		assert((0 < nNewState) && (nNewState <= NUM_STATES));

		// Cannot check for m_nCurrentState != nNewState, because there can be several flowgraph nodes
		// setting the states on the same switch. This particular node might need to set the same state again, 
		// if another node has set a different one in between the calls to set the state on this node.
		m_nCurrentState = static_cast<uint32>(nNewState);

		if (pEntity != NULL)
		{
			SetStateOnProxy(pEntity, m_nCurrentState - 1);
		}
		else
		{
			SetStateOnGlobalObject(m_nCurrentState - 1);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	uint32							m_nCurrentState;

	TAudioControlID			m_nSwitchID;
	TAudioSwitchStateID	m_aSwitchStates[NUM_STATES];

	SAudioRequest m_oRequest;
	SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> m_oRequestData;
};

REGISTER_FLOW_NODE("Audio:Switch", CFlowNode_AudioSwitch);
