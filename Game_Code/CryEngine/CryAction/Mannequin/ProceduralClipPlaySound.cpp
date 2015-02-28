////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>

class CAudioContext : public IProceduralContext
{
private:

	typedef IProceduralContext BaseClass;

public:

	PROCEDURAL_CONTEXT(CAudioContext, "AudioContext", 0xC6C087F64CE14854, 0xADCA544D252834BD);
	
	virtual void Initialise(IEntity &entity, IActionController &actionController)
	{
		BaseClass::Initialise(entity, actionController);

		m_pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr> (entity.CreateProxy(ENTITY_PROXY_AUDIO));
	}

	virtual void Update(float timePassed)
	{
	}

	void ExecuteAudioTrigger(TAudioControlID const nID, bool playFacial)
	{
		if (m_pIEntityAudioProxy != NULL)
		{
			m_pIEntityAudioProxy->ExecuteTrigger(nID, playFacial ? eLSM_MatchAnimationToSoundName : eLSM_None);
		}
	}

	void StopAudioTrigger(TAudioControlID const nID)
	{
		if (m_pIEntityAudioProxy != NULL)
		{
			m_pIEntityAudioProxy->StopTrigger(nID);
		}
	}

	void SetAudioObjectPos(QuatT const& rOffset)
	{
		if (m_pIEntityAudioProxy != NULL)
		{
			m_pIEntityAudioProxy->SetAuxAudioProxyOffset(SATLWorldPosition(Matrix34(IDENTITY, rOffset.t), ZERO));
		}
	}

private:

	IEntityAudioProxyPtr m_pIEntityAudioProxy;
};

CAudioContext::CAudioContext() : m_pIEntityAudioProxy()
{
}

CAudioContext::~CAudioContext()
{
}

CRYREGISTER_CLASS(CAudioContext);

// data layout access twinned with data lookup inside CProcClipKey::SetKeyType()
struct SAudioParams : public SProceduralParams
{
	float radius;
	float synchStop;
	float forceStopOnExit;
	float isVoice;
	float playFacial;
};

class CProceduralClipAudio : public TProceduralContextualClip<SAudioParams, CAudioContext>
{
private:
	
	struct SAudioParamInfo
	{
		SAudioParamInfo() : paramIndex(-1), paramCRC(0), paramValue(-1.f) {}
		SAudioParamInfo(int _index, uint32 _crc, float _value) : paramIndex(_index), paramCRC(_crc), paramValue(_value) {}

		int    paramIndex;
		uint32 paramCRC;
		float  paramValue;
	};

	typedef std::vector<SAudioParamInfo> TAudioParamVec;

public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipAudio, "Audio", 0xB46A0D65B3EE4f26, 0x82A97ABFEA6BE43D)

	virtual void OnEnter(float blendTime, float duration, const SAudioParams &params)
	{
		m_referenceJointID = -1;
		m_synchedStop = (0.5f < params.synchStop);
		m_forceStopOnExit = (0.5f < params.forceStopOnExit);
		bool const playFacial = (0.5f < params.playFacial);
		bool const isVoice    = playFacial || (0.5f < params.isVoice);

		bool bIsSilentPlaybackMode = gEnv->IsEditor() && gEnv->pGame->GetIGameFramework()->GetMannequinInterface().IsSilentPlaybackMode();

		ICharacterInstance const* const pCharacterInstance = m_scope->GetCharInst();

		if (pCharacterInstance != NULL)
		{
			m_referenceJointID = pCharacterInstance->GetIDefaultSkeleton().GetJointIDByCRC32(params.dataCRC.crc);
		}

		QuatT const soundOffset = GetBoneAbsLocationByID(m_referenceJointID);

		if (!bIsSilentPlaybackMode)
		{
			if (!params.dataString.empty())
			{
				gEnv->pAudioSystem->GetAudioTriggerID(params.dataString.c_str(), m_nAudioControlIDStart);

				if (m_nAudioControlIDStart != INVALID_AUDIO_CONTROL_ID)
				{
					m_context->ExecuteAudioTrigger(m_nAudioControlIDStart, playFacial);
				}
			}

			if (!params.dataString2.empty())
			{
				gEnv->pAudioSystem->GetAudioTriggerID(params.dataString2.c_str(), m_nAudioControlIDStop);
			}

			/*_smart_ptr<ISound> pSound = gEnv->pAudioSystem->GetSound(m_soundID);

			if (pSound)
			{
				bool foundparam = true;
				int i = 0;
				TAudioParamVec paramVec;

				while(foundparam)
				{
					float fValue = 0.f;
					const char* paramName = NULL;
					foundparam = pSound->GetParam(i, &fValue, NULL, NULL, &paramName,false);

					if(foundparam && paramName && paramName[0])
					{
						const uint32 paramCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(paramName);

						float paramValue = 0.f;
						if(GetParam(paramCRC, paramValue))
						{
							pSound->SetParam(i, paramValue);
							paramVec.push_back(SAudioParamInfo(i, paramCRC, paramValue));
						}
					}

					i++;
				}

				m_oAudioParams = paramVec;
			}*/
		}
	}

	virtual void OnExit(float blendTime)
	{
		if (m_nAudioControlIDStop != INVALID_AUDIO_CONTROL_ID)
		{
			m_context->ExecuteAudioTrigger(m_nAudioControlIDStop, false);
		}
		else if (m_nAudioControlIDStart != INVALID_AUDIO_CONTROL_ID)
		{
			m_context->StopAudioTrigger(m_nAudioControlIDStart);
		}

		m_nAudioControlIDStart	= INVALID_AUDIO_CONTROL_ID;
		m_nAudioControlIDStop		= INVALID_AUDIO_CONTROL_ID;
	}

	virtual void Update(float timePassed)
	{
		UpdateSoundParams();

		UpdateSoundPosition();
	}

private:

	void UpdateSoundParams()
	{
		if (!m_oAudioParams.empty())
		{
			const size_t numParams = m_oAudioParams.size();
			for(size_t i = 0; i < numParams; i++)
			{
				float paramValue = 0.f;
				if(GetParam(m_oAudioParams[i].paramCRC, paramValue) && (m_oAudioParams[i].paramValue != paramValue))
				{
					REINST("update RTPCs")
					//pSound->SetParam(m_soundParams[i].paramIndex, paramValue);
					m_oAudioParams[i].paramValue = paramValue;
				}
			}
		}
	}

	void UpdateSoundPosition()
	{
		if(m_referenceJointID < 0)
			return;

		m_context->SetAudioObjectPos(GetBoneAbsLocationByID(m_referenceJointID));
	}

	QuatT GetBoneAbsLocationByID( const int jointID )
	{
		ICharacterInstance* pCharacterInstance = m_scope->GetCharInst();
		if ((pCharacterInstance != NULL) && (jointID >= 0))
		{
			return pCharacterInstance->GetISkeletonPose()->GetAbsJointByID(jointID);
		}

		return QuatT(ZERO, IDENTITY);
	}

	TAudioParamVec m_oAudioParams;

	int  m_referenceJointID;

	bool m_synchedStop;
	bool m_forceStopOnExit;
	TAudioControlID m_nAudioControlIDStart;
	TAudioControlID m_nAudioControlIDStop;
};

CProceduralClipAudio::CProceduralClipAudio()
: m_synchedStop(false)
, m_forceStopOnExit(false)
, m_nAudioControlIDStart(INVALID_AUDIO_CONTROL_ID)
, m_nAudioControlIDStop(INVALID_AUDIO_CONTROL_ID)
{
}

CProceduralClipAudio::~CProceduralClipAudio()
{
}

CRYREGISTER_CLASS(CProceduralClipAudio)
