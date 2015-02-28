////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
//
// -------------------------------------------------------------------------
//  File name:   FlowAudioTriggerNode.cpp
//  Version:     v1.00
//  Created:     12/05/2014 by Mikhail Korotyaev
//  Description: FlowGraph Node that executes AudioTriggers.
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioTrigger : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_AudioTrigger(SActivationInfo* pActInfo)
		:	m_bIsPlaying(false)
		, m_nPlayTriggerID(INVALID_AUDIO_CONTROL_ID)
		, m_nStopTriggerID(INVALID_AUDIO_CONTROL_ID)
	{}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioTrigger() {}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioTrigger(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS : uint32
	{
		eIn_PlayTrigger,
		eIn_StopTrigger,
		eIn_Play,
		eIn_Stop,
	};

	enum OUTPUTS 
	{
		eOut_Done,
	};

	VIRTUAL void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = 
		{
			InputPortConfig<string>("audioTrigger_PlayTrigger", _HELP("name of the Play Trigger"), "PlayTrigger"),
			InputPortConfig<string>("audioTrigger_StopTrigger", _HELP("name of the Stop Trigger"), "StopTrigger"),

			InputPortConfig_Void("Play", _HELP("Executes the Play Trigger")),
			InputPortConfig_Void("Stop", _HELP("Executes the Stop Trigger if it is provided, o/w stops all events started by the Start Trigger")),
			{0}
		};

		static const SOutputPortConfig outputs[] = 
		{
			OutputPortConfig_Void( "Done", _HELP("Activated when all of the triggered events have finished playing")),
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node executes Audio Triggers.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		bool bPlay = m_bIsPlaying;
		ser.BeginGroup("FlowAudioTriggerNode");
		ser.Value("play", bPlay);
		ser.EndGroup();

		if (ser.IsReading())
		{
			m_bIsPlaying = false;
			Init(pActInfo, bPlay);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Stop(pActInfo->pEntity);
				Init(pActInfo, false);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_Play))
				{
					m_oPlayActivationInfo = *pActInfo;
					Play(pActInfo->pEntity);
				}

				if (IsPortActive(pActInfo, eIn_Stop))
				{
					Stop(pActInfo->pEntity);
				}

				if (IsPortActive(pActInfo, eIn_PlayTrigger))
				{
					Stop(pActInfo->pEntity);
					GetTriggerID(pActInfo, eIn_PlayTrigger, m_nPlayTriggerID);
				}

				if (IsPortActive(pActInfo, eIn_StopTrigger))
				{
					GetTriggerID(pActInfo, eIn_StopTrigger, m_nStopTriggerID);
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

	//////////////////////////////////////////////////////////////////////////
	void TriggerFinished()
	{
		ActivateOutput(&m_oPlayActivationInfo, eOut_Done, true);
	}

private:

	//////////////////////////////////////////////////////////////////////////
	static void TriggerFinishedCallback(TAudioObjectID const nObjectID, TAudioControlID const nTriggerID, void* const pCookie)
	{
		CFlowNode_AudioTrigger* const pTriggerNode = static_cast<CFlowNode_AudioTrigger*>(pCookie);

		if (pTriggerNode != NULL)
		{
			pTriggerNode->TriggerFinished();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void GetTriggerID(SActivationInfo* const pActInfo, uint32 const nPortIdx, TAudioControlID& rOutTriggerID)  
	{
		string const& rTriggerName = GetPortString(pActInfo, nPortIdx);
		if (!rTriggerName.empty())
		{
			gEnv->pAudioSystem->GetAudioTriggerID(rTriggerName.c_str(), rOutTriggerID);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo, bool const bPlay)
	{
		m_bIsPlaying = false;
		if (gEnv->pAudioSystem != NULL)
		{
			GetTriggerID(pActInfo, eIn_PlayTrigger, m_nPlayTriggerID);
			GetTriggerID(pActInfo, eIn_StopTrigger, m_nStopTriggerID);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnProxy(IEntity* const pEntity, TAudioControlID const nTriggerID, bool const bStop)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		
		if (pIEntityAudioProxy != NPTR)
		{
			if (!bStop)
			{
				pIEntityAudioProxy->ExecuteTrigger(nTriggerID, eLSM_None, TriggerFinishedCallback, this);
			}
			else
			{
				pIEntityAudioProxy->StopTrigger(nTriggerID);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnGlobalObject(TAudioControlID const nTriggerID, bool const bStop)
	{
		if (!bStop)
		{
			m_oExecuteRequestData.nTriggerID = nTriggerID;
			m_oExecuteRequestData.pCallback = TriggerFinishedCallback;
			m_oExecuteRequestData.pCallbackCookie = this;
			m_oRequest.pData = &m_oExecuteRequestData;
			gEnv->pAudioSystem->PushRequest(m_oRequest);
		}
		else
		{
			m_oStopRequestData.nTriggerID = nTriggerID;
			m_oRequest.pData = &m_oStopRequestData;
			gEnv->pAudioSystem->PushRequest(m_oRequest);
		}

	}

	//////////////////////////////////////////////////////////////////////////
	void Play(IEntity* const pEntity)
	{
		if (m_nPlayTriggerID != INVALID_AUDIO_CONTROL_ID)
		{
			if (pEntity != NPTR)
			{
				ExecuteOnProxy(pEntity, m_nPlayTriggerID, false);
			}
			else
			{
				ExecuteOnGlobalObject(m_nPlayTriggerID, false);
			}
			m_bIsPlaying = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Stop(IEntity* const pEntity)
	{
		if (m_bIsPlaying)
		{
			if (m_nStopTriggerID != INVALID_AUDIO_CONTROL_ID)
			{
				if (pEntity != NPTR)
				{
					ExecuteOnProxy(pEntity, m_nStopTriggerID, false);
				}
				else
				{
					ExecuteOnGlobalObject(m_nStopTriggerID, false);
				}
			}
			else
			{
				if (pEntity != NPTR)
				{
					ExecuteOnProxy(pEntity, m_nPlayTriggerID, true);
				}
				else
				{
					ExecuteOnGlobalObject(m_nPlayTriggerID, true);
				}
			}

			m_bIsPlaying = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool						m_bIsPlaying;
	TAudioControlID	m_nPlayTriggerID;	
	TAudioControlID	m_nStopTriggerID;

	SActivationInfo m_oPlayActivationInfo;

	SAudioRequest m_oRequest;
	SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> m_oExecuteRequestData;
	SAudioObjectRequestData<eAORT_STOP_TRIGGER> m_oStopRequestData;
};

REGISTER_FLOW_NODE("Audio:Trigger", CFlowNode_AudioTrigger);
