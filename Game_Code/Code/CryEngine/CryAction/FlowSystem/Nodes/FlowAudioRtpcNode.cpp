////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
//
// -------------------------------------------------------------------------
//  File name:   FlowAudioRtpcNode.cpp
//  Version:     v1.00
//  Created:     15/05/2014 by Mikhail Korotyaev
//  Description: FlowGraph Node that sets RTPC values.
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioRtpc : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_AudioRtpc(SActivationInfo* pActInfo)
		: m_fValue(0.0f)
		, m_nRtpcID(INVALID_AUDIO_CONTROL_ID)
	{}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioRtpc() {}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioRtpc(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS : uint32
	{
		eIn_RtpcName,
		eIn_RtpcValue,
	};

	enum OUTPUTS 
	{
	};

	VIRTUAL void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = 
		{
			InputPortConfig<string>("audioRTPC_Name", _HELP("RTPC name"), "Name"),
			InputPortConfig<float>("value", _HELP("RTPC value"), "Value"),
			{0}
		};

		static const SOutputPortConfig outputs[] = 
		{
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node sets RTPC values.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		float fValue = m_fValue;
		ser.BeginGroup("FlowAudioRtpcNode");
		ser.Value("value", fValue);
		ser.EndGroup();

		if (ser.IsReading())
		{
			SetValue(pActInfo->pEntity, fValue, true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Init(pActInfo);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_RtpcValue))
				{
					SetValue(pActInfo->pEntity, GetPortFloat(pActInfo, eIn_RtpcValue));
				}

				if (IsPortActive(pActInfo, eIn_RtpcName))
				{
					GetRtpcID(pActInfo);
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

	static float const EPSILON;

	//////////////////////////////////////////////////////////////////////////
	void GetRtpcID(SActivationInfo* const pActInfo)  
	{
		string const& rRtpcName = GetPortString(pActInfo, eIn_RtpcName);
		if (!rRtpcName.empty())
		{
			gEnv->pAudioSystem->GetAudioRtpcID(rRtpcName.c_str(), m_nRtpcID);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetValue(IEntity* const pEntity, float const fValue, bool const bForceSet = false)
	{
		if (abs(fValue-m_fValue) > EPSILON || bForceSet)
		{
			m_fValue = fValue;

			if (pEntity != NULL)
			{
				SetOnProxy(pEntity, m_fValue);
			}
			else
			{
				SetOnGlobalObject(m_fValue);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo)
	{
		if (gEnv->pAudioSystem != NULL)
		{
			GetRtpcID(pActInfo);

			m_oRequest.pData = &m_oRequestData;
			m_oRequestData.nControlID = m_nRtpcID;

			float fValue = GetPortFloat(pActInfo, eIn_RtpcValue);
			SetValue(pActInfo->pEntity, fValue, true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetOnProxy(IEntity* const pEntity, float const fValue)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));
		
		if (pIEntityAudioProxy != NPTR)
		{
			pIEntityAudioProxy->SetRtpcValue(m_nRtpcID, fValue);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetOnGlobalObject(float const fValue)
	{
		m_oRequestData.fValue = fValue;
		gEnv->pAudioSystem->PushRequest(m_oRequest);
	}

	//////////////////////////////////////////////////////////////////////////
	float						m_fValue;
	TAudioControlID	m_nRtpcID;

	SAudioRequest m_oRequest;
	SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> m_oRequestData;
};

float const CFlowNode_AudioRtpc::EPSILON = 1.0e-6;

REGISTER_FLOW_NODE("Audio:Rtpc", CFlowNode_AudioRtpc);
