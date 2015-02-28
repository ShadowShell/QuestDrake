////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
//
// -------------------------------------------------------------------------
//  File name:   FlowAudioPreloadData.cpp
//  Version:     v1.00
//  Created:     09/04/2014 by Thomas Wollenzin
//  Description: Allows for loading/unloading preload requests.
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioPreloadData : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_AudioPreloadData(SActivationInfo* pActInfo)
		:	m_bEnable(false)
	{}

	~CFlowNode_AudioPreloadData() {}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioPreloadData(pActInfo);
	}

	enum INPUTS : uint32
	{
		eIn_PreloadRequestFirst = 0,
		eIn_PreloadRequest1,
		eIn_PreloadRequest2,
		eIn_PreloadRequest3,
		eIn_PreloadRequest4,
		eIn_PreloadRequest5,
		eIn_PreloadRequest6,
		eIn_PreloadRequestLast,
		eIn_Enable,
		eIn_Disable,
	};

	enum OUTPUTS 
	{
	};

	VIRTUAL void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = 
		{
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest0", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest1", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest2", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest3", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest4", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest5", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest6", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest7", _HELP("name of preload request"), "Preload Request"),
			InputPortConfig_Void("Load", _HELP("loads all supplied preload requests")),
			InputPortConfig_Void("Unload", _HELP("unloads all supplied preload requests")),
			{0}
		};

		static const SOutputPortConfig outputs[] = 
		{
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Node that allows for handling audio preload requests.");
		config.SetCategory(EFLN_APPROVED);
	}

	void Enable(SActivationInfo *pActInfo, bool bEnable)
	{
		if (m_bEnable != bEnable && gEnv->pAudioSystem != NULL)
		{
			for (uint32 i = eIn_PreloadRequestFirst; i <= eIn_PreloadRequestLast; ++i)
			{
				string const& rPreloadName = GetPortString(pActInfo, static_cast<int>(i));

				if (!rPreloadName.empty())
				{
					SAudioRequest oAudioRequestData;
					TAudioPreloadRequestID nPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;

					if (gEnv->pAudioSystem->GetAudioPreloadRequestID(rPreloadName.c_str(), nPreloadRequestID))
					{
						if (bEnable)
						{
							SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> oAMData(nPreloadRequestID);
							oAudioRequestData.pData = &oAMData;
							gEnv->pAudioSystem->PushRequest(oAudioRequestData);
						}
						else
						{
							SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST> oAMData(nPreloadRequestID);
							oAudioRequestData.pData = &oAMData;
							gEnv->pAudioSystem->PushRequest(oAudioRequestData);
						}
					}
				}
			}

			m_bEnable = bEnable;
		}
	}

	VIRTUAL void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		bool bEnable = m_bEnable;
		ser.BeginGroup("FlowAudioPreloadData");
		ser.Value("enable", bEnable);
		ser.EndGroup();

		if (ser.IsReading())
		{
			Enable(pActInfo, bEnable);
		}
	}


	VIRTUAL void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				if (gEnv->IsEditor() && !gEnv->IsEditing())
				{
					Enable(pActInfo, false);
				}

				m_bEnable = false;

				break;
			}
		case eFE_Activate:
			{
				// Enable
				if (IsPortActive(pActInfo, eIn_Enable))
				{
					Enable(pActInfo, true);
				}

				// Disable
				if (IsPortActive(pActInfo, eIn_Disable))
				{
					Enable(pActInfo, false);
				}

				break;
			}
		}
	}

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

private:

	bool m_bEnable;
};

REGISTER_FLOW_NODE("Audio:PreloadData", CFlowNode_AudioPreloadData);
