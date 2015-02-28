// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "MFXEffect.h"

CMFXEffect::CMFXEffect()
{
}
CMFXEffect::~CMFXEffect()
{
}

void CMFXEffect::Execute(SMFXRunTimeEffectParams& params)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	// TEMP Code: testing footsteps; can this stay here???
	IEntity *pEnt = NULL;

	if (m_effectParams.libName == "footstep_player")
	{
		if (params.audioProxyEntityId != 0)
		{
			pEnt = gEnv->pEntitySystem->GetEntity(params.audioProxyEntityId);

			if (pEnt != NULL)
			{
				IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEnt->CreateProxy(ENTITY_PROXY_AUDIO));

				TAudioControlID nFootstepTriggerID = INVALID_AUDIO_CONTROL_ID;
				gEnv->pAudioSystem->GetAudioTriggerID(m_effectParams.libName, nFootstepTriggerID);

				TAudioControlID nFirstPersonSwitchID		= INVALID_AUDIO_CONTROL_ID;
				TAudioSwitchStateID nFirstPersonStateID	= INVALID_AUDIO_SWITCH_STATE_ID;
				gEnv->pAudioSystem->GetAudioSwitchID("1stOr3rdP", nFirstPersonSwitchID);
				gEnv->pAudioSystem->GetAudioSwitchStateID(nFirstPersonSwitchID, params.playSoundFP ? "1stP" : "3rdP", nFirstPersonStateID);

				TAudioControlID nSurfaceSwitchID		= INVALID_AUDIO_CONTROL_ID;
				TAudioSwitchStateID nSurfaceStateID	= INVALID_AUDIO_SWITCH_STATE_ID;
				gEnv->pAudioSystem->GetAudioSwitchID("SurfaceType", nSurfaceSwitchID);
				gEnv->pAudioSystem->GetAudioSwitchStateID(nSurfaceSwitchID, m_effectParams.name, nSurfaceStateID);

				TAudioControlID nSpeedRtpcId = INVALID_AUDIO_CONTROL_ID;
				gEnv->pAudioSystem->GetAudioRtpcID("character_speed", nSpeedRtpcId);

				float fSpeed = 0.0f;

				for (int i=0; i < params.MAX_SOUND_PARAMS; ++i)
				{
					const char* soundParamName = params.soundParams[i].paramName;
					
					if ((soundParamName != NULL) && (soundParamName[0] != '\0') && (_stricmp(soundParamName, "speed") == 0))
					{
						float const fValue = params.soundParams[i].paramValue;
						CRY_ASSERT(NumberValid(fValue));

						fSpeed = fValue;
						break;
					}
				} 

				pIEntityAudioProxy->SetSwitchState(nFirstPersonSwitchID, nFirstPersonStateID);
				pIEntityAudioProxy->SetSwitchState(nSurfaceSwitchID, nSurfaceStateID);
				pIEntityAudioProxy->SetRtpcValue(nSpeedRtpcId, fSpeed);
				pIEntityAudioProxy->ExecuteTrigger(nFootstepTriggerID, eLSM_None);
			}
		}
	}
	else if ((m_effectParams.libName == "bulletimpacts") && ((params.playflags & MFX_PLAY_SOUND) != 0))
	{
		TAudioControlID nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
		gEnv->pAudioSystem->GetAudioTriggerID(m_effectParams.name, nAudioTriggerID);

		if (nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
		{
			IAudioProxy* const pIAudioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();

			if (pIAudioProxy != NULL)
			{
				pIAudioProxy->Initialize("Projectile");
				pIAudioProxy->SetPosition(params.pos);
				pIAudioProxy->SetObstructionCalcType(eAOOCT_SINGLE_RAY);
				pIAudioProxy->SetCurrentEnvironments();
				pIAudioProxy->ExecuteTrigger(nAudioTriggerID, eLSM_None);
				pIAudioProxy->Release();
			}
		}
	}
	else if ((m_effectParams.libName == "collisions") && ((params.playflags & MFX_PLAY_SOUND) != 0))
	{
		TAudioControlID nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
		gEnv->pAudioSystem->GetAudioTriggerID(m_effectParams.name, nAudioTriggerID);

		if (nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
		{
			IAudioProxy* const pIAudioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();

			if (pIAudioProxy != NULL)
			{
				pIAudioProxy->Initialize("Collision");
				pIAudioProxy->SetPosition(params.pos);
				pIAudioProxy->SetObstructionCalcType(eAOOCT_SINGLE_RAY);
				pIAudioProxy->SetCurrentEnvironments();
				pIAudioProxy->ExecuteTrigger(nAudioTriggerID, eLSM_None);
				pIAudioProxy->Release();
			}
		}
	}

	std::vector<IMFXEffectPtr>::iterator iter = m_effects.begin();
	std::vector<IMFXEffectPtr>::iterator iterEnd = m_effects.end();
	while (iter != iterEnd)
	{
		IMFXEffectPtr& cur = *iter;
		if (cur)
		{
			if (cur->CanExecute(params))
				cur->Execute(params);
		}
		++iter;
	}
}

void CMFXEffect::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	std::vector<IMFXEffectPtr>::iterator iter = m_effects.begin();
	std::vector<IMFXEffectPtr>::iterator iterEnd = m_effects.end();
	while (iter != iterEnd)
	{
		IMFXEffectPtr& cur = *iter;
		if (cur)
		{
			cur->SetCustomParameter(customParameter ,customParameterValue);
		}
		++iter;
	}
}

IMFXEffectPtr CMFXEffect::Clone()
{
	CMFXEffect *clone = new CMFXEffect();
	clone->m_effectParams = m_effectParams;

	size_t nEffects = 0;
	std::vector<IMFXEffectPtr>::iterator iter = m_effects.begin();
	std::vector<IMFXEffectPtr>::iterator iterEnd = m_effects.end();
	while (iter != iterEnd)
	{
		IMFXEffectPtr& cur = *iter;
		if (cur)
			++nEffects;
		++iter;
	}

	clone->m_effects.reserve(nEffects);
	iter = m_effects.begin();
	while (iter != iterEnd)
	{
		IMFXEffectPtr& cur = *iter;
		if (cur)
		{
			IMFXEffectPtr childClone = cur->Clone();
			clone->AddChild(childClone);
		}
		++iter;
	}
	return clone;
}

void CMFXEffect::ReadXMLNode(XmlNodeRef& node)
{
	IMFXEffect::ReadXMLNode(node);
}

void CMFXEffect::GetResources(SMFXResourceList& rlist)
{
	std::vector<IMFXEffectPtr>::iterator it = m_effects.begin();
	while (it != m_effects.end())
	{
		IMFXEffectPtr cur = *it;
		if (cur)
		{
			cur->GetResources(rlist);
		}
		++it;
	}
}

void CMFXEffect::GetMemoryUsage( ICrySizer *pSizer ) const
{
	IMFXEffect::GetMemoryUsage(pSizer);
}