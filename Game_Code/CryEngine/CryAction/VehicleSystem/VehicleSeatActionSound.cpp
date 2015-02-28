/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action for sounds (ie: honk on trucks)

-------------------------------------------------------------------------
History:
- 16:11:2005: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "IVehicleSystem.h"
#include "Vehicle.h"
#include "VehicleSeat.h"
#include "VehicleSeatActionSound.h"

CVehicleSeatActionSound::CVehicleSeatActionSound() : 
	m_pHelper(NULL), 
	m_pSeat(NULL), 
	m_pVehicle(NULL),
	m_nAudioControlIDStart(INVALID_AUDIO_CONTROL_ID),
	m_nAudioControlIDStop(INVALID_AUDIO_CONTROL_ID)
{
}

//------------------------------------------------------------------------
bool CVehicleSeatActionSound::Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table)
{
	m_pVehicle = pVehicle;
	m_pSeat = static_cast<CVehicleSeat*>(pSeat);

	CVehicleParams soundTable = table.findChild("Audio");
	if (!soundTable)
		return false;

	gEnv->pAudioSystem->GetAudioTriggerID(soundTable.getAttr("startTrigger"), m_nAudioControlIDStart);
	gEnv->pAudioSystem->GetAudioTriggerID(soundTable.getAttr("stopTrigger"), m_nAudioControlIDStop);

	if (soundTable.haveAttr("helper"))
		m_pHelper = m_pVehicle->GetHelper(soundTable.getAttr("helper"));

	if (!m_pHelper)
		return false;

	m_enabled = false;
	return true;
}

//------------------------------------------------------------------------
void CVehicleSeatActionSound::Serialize(TSerialize ser, EEntityAspects aspects)
{
	if (aspects&CVehicle::ASPECT_SEAT_ACTION)
	{
		NET_PROFILE_SCOPE("SeatAction_Sound", ser.IsReading());

		bool enabled=m_enabled;
		
		ser.Value("enabled", enabled, 'bool');

		if(ser.IsReading())
		{
			if(m_enabled != enabled)
			{
				if (enabled)
					ExecuteTrigger(m_nAudioControlIDStart);
				else
					StopTrigger();

				m_enabled=enabled;
			}
		}
	}
}

//------------------------------------------------------------------------
void CVehicleSeatActionSound::StopUsing()
{
	if (m_enabled)
		StopTrigger();
}

//------------------------------------------------------------------------
void CVehicleSeatActionSound::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
	if (actionId == eVAI_Horn && activationMode == eAAM_OnPress)
	{
		ExecuteTrigger(m_nAudioControlIDStart);
	}
	else if (actionId == eVAI_Horn && activationMode == eAAM_OnRelease)
	{
		StopTrigger();
	}
}

void CVehicleSeatActionSound::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
}

void CVehicleSeatActionSound::ExecuteTrigger(const TAudioControlID& controlID)
{
	if (controlID == INVALID_AUDIO_CONTROL_ID)
		return;

	if (m_pSeat)
		m_pSeat->ChangedNetworkState(CVehicle::ASPECT_SEAT_ACTION);

	IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(m_pVehicle->GetEntity()->CreateProxy(ENTITY_PROXY_AUDIO));
	assert(pIEntityAudioProxy.get());

	pIEntityAudioProxy->ExecuteTrigger(controlID, eLSM_None);

	// Report the AI system about the vehicle movement sound.
	if (!gEnv->bMultiplayer && gEnv->pAISystem)
	{
		Vec3 pos = m_pHelper->GetVehicleSpaceTranslation();
		SAIStimulus stim(AISTIM_SOUND, AISOUND_MOVEMENT_LOUD, m_pVehicle->GetEntityId(), 0, pos, ZERO, 200.0f);
		gEnv->pAISystem->RegisterStimulus(stim);
	}

	m_enabled = true;
}

void CVehicleSeatActionSound::StopTrigger()
{
	if (m_pSeat)
		m_pSeat->ChangedNetworkState(CVehicle::ASPECT_SEAT_ACTION);

	if (m_nAudioControlIDStop != INVALID_AUDIO_CONTROL_ID)
	{
		ExecuteTrigger(m_nAudioControlIDStop);
	}
	else if(m_nAudioControlIDStart != INVALID_AUDIO_CONTROL_ID)
	{
		IEntityAudioProxyPtr pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(m_pVehicle->GetEntity()->CreateProxy(ENTITY_PROXY_AUDIO));
		assert(pIEntityAudioProxy.get());
		pIEntityAudioProxy->StopTrigger(m_nAudioControlIDStart);
	}

	m_enabled = false;
}

DEFINE_VEHICLEOBJECT(CVehicleSeatActionSound);
