/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	A module for servers to use to track information about
							players for the purposes of cheat detection

-------------------------------------------------------------------------
History:
- 16:02:2012: Created by Richard Semmens

*************************************************************************/
#include "StdAfx.h"
#include "AntiCheat/ServerPlayerTracker.h"
#include "AntiCheatManager.h"

#include "GameCVars.h"

#if SERVER_CHEAT_MONITOR_ENABLED
#include "RecordingSystem.h"


const float CServerPlayerTracker::kKillCamValidationCheckFrequency = 2.0f;
const float CServerPlayerTracker::kInvalidEndLockTime = -1.0f;
float CServerPlayerTracker::s_fLastTickTime = 0.0f;

DEFINE_INTRUSIVE_LINKED_LIST(CServerPlayerTracker);

CServerPlayerTracker::CServerPlayerTracker()
	: m_nTotalHostileKills(0)
{
}

bool CServerPlayerTracker::Init( IGameObject * pGameObject )
{
	SetGameObject(pGameObject);
	return true;
}

/*static*/ void CServerPlayerTracker::UpdateAllPlayerTrackers(float fFrameTime)
{
	CServerPlayerTracker * pTracker = CServerPlayerTracker::get_intrusive_list_root();

	if(CRecordingSystem::KillCamEnabled())
	{
		const float fCurrentTime = gEnv->pTimer->GetCurrTime();
		bool updateTracker = false;
		if(s_fLastTickTime < fCurrentTime - kKillCamValidationCheckFrequency)
		{
			updateTracker = true;
			s_fLastTickTime = fCurrentTime;
		}
		const float fCurrentFrameTime = gEnv->pTimer->GetFrameTime();
		while(pTracker)
		{
			if(updateTracker)
			{
				pTracker->UpdateTracker(fCurrentTime);
			}
			pTracker->m_killCamStreamer.Update(fCurrentFrameTime);
			pTracker = pTracker->m_next_intrusive;
		}
	}
}

void CServerPlayerTracker::UpdateTracker(float fCurrentTime)
{
	CheckKillCamValidities(fCurrentTime);
}


// Handle clients disconnecting when another client is sending them kill cam data.
/*static */ void CServerPlayerTracker::OnClientDisconnect( uint16 channelId )
{
	CServerPlayerTracker * pTracker = CServerPlayerTracker::get_intrusive_list_root();

	if(CRecordingSystem::KillCamEnabled())
	{
		while(pTracker)
		{
			pTracker->ClientDisconnected( channelId );

			pTracker = pTracker->m_next_intrusive;
		}
	}
}

void CServerPlayerTracker::ClientDisconnected( uint16 channelId )
{
	TKillCamValidationMap::iterator KillCamValidationData = m_KillCamValidationMap.find(channelId);

	if(KillCamValidationData != m_KillCamValidationMap.end())
	{
		m_KillCamValidationMap.erase(KillCamValidationData);
	}
}

void CServerPlayerTracker::CheckKillCamValidities(float fCurrentTime)
{
	static const float kLongestDelayToGetKillCam = 10.0f; //TODO: Get from anti cheat vars

	const float fLastAcceptableTime = fCurrentTime - kLongestDelayToGetKillCam;

	for(TKillCamValidationMap::iterator iter = m_KillCamValidationMap.begin(), iterEnd = m_KillCamValidationMap.end(); iter != iterEnd; ++iter)
	{
		SKillCamValidation& rVictimData = iter->second;

		if(rVictimData.uKillCams < rVictimData.uKills && rVictimData.fLastKillTime < fLastAcceptableTime)
		{
			if(IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(GetEntityId()))
			{
				g_pGame->GetAntiCheatManager()->FlagActivity(eCT_NoKillCam, pActor->GetChannelId());
			}			

			//Wipe stats to prevent repeated flagging for the same infraction
			rVictimData.uKillCams					= 0;
			rVictimData.uKills						= 0;
			rVictimData.fLastKillTime			= 0.0f;
			rVictimData.fLastKillCamTime	= 0.0f;
		}
	}
}

void CServerPlayerTracker::KillCamDataReceived(uint16 victimChannelId)
{
	SKillCamValidation& victimData = GetOrCreateVictimData(victimChannelId);

	victimData.uKillCams++;
	victimData.fLastKillCamTime = gEnv->pTimer->GetCurrTime();	//Currently for debugging information. May be necessary if we change the algorithm for marking people as cheaters
}

void CServerPlayerTracker::KillOccurred(const HitInfo &hitInfo, uint16 victimChannelId, bool bExpectKillCam)
{
	if(hitInfo.targetId != GetEntityId() && bExpectKillCam) //We don't expect killcams when a player kills themselves
	{
		SKillCamValidation& victimData = GetOrCreateVictimData(victimChannelId);

		victimData.vLastKillDir = hitInfo.dir;
		victimData.uKills++;
		victimData.fLastKillTime = gEnv->pTimer->GetCurrTime();
	}

	CGameRules * pGameRules = g_pGame->GetGameRules();

	CGameRules::eThreatRating threatRating = pGameRules->GetThreatRatingByTeam(pGameRules->GetTeam(hitInfo.targetId), pGameRules->GetTeam(GetEntityId()));

	if(threatRating == CGameRules::eHostile)
	{
		m_nTotalHostileKills++;
	}
}

const static float kLeniency = 0.5f;

ILINE static float GetKillCamPart1Length()
{
	return g_pGameCVars->kc_kickInTime + kLeniency;
}

ILINE static float GetKillCamPart2Length()
{
	return (max(g_pGameCVars->kc_length, g_pGameCVars->kc_skillKillLength) + kLeniency) - g_pGameCVars->kc_kickInTime;
}

void CServerPlayerTracker::NewCloseCombatSnapTarget(const IActor * pTargetActor)
{
	float fCurrentTime = gEnv->pTimer->GetCurrTime();

	float fKillCamLength = (GetKillCamPart1Length() + GetKillCamPart2Length()) * 2.0f;
	while(!m_MeleeLockQueue.empty() && (m_MeleeLockQueue.front().fEndLock != kInvalidEndLockTime && m_MeleeLockQueue.front().fEndLock < (fCurrentTime - fKillCamLength)))
		m_MeleeLockQueue.pop_front();

	//Handle switching straight from target -> target or from target->nothing.
	if(!m_MeleeLockQueue.empty())
	{
		SMeleeLockRecord& rLastLock = m_MeleeLockQueue.back();

		if(rLastLock.fEndLock == kInvalidEndLockTime)
			rLastLock.fEndLock = fCurrentTime;
	}	

	//Add target & timestamp
	if(pTargetActor)
	{
		m_MeleeLockQueue.push_back(SMeleeLockRecord(pTargetActor->GetChannelId(), fCurrentTime));
	}
}

const Vec3& CServerPlayerTracker::GetLastKillDirection(uint16 victimChannelId)
{
	SKillCamValidation& victimData = GetOrCreateVictimData(victimChannelId);

	return victimData.vLastKillDir;
}

bool CServerPlayerTracker::DidTrackTargetDuringKillcam(EntityId victim, uint16 victimChannelId)
{	
	SKillCamValidation& victimData = GetOrCreateVictimData(victimChannelId);

	const float fStartTime	= (victimData.fLastKillTime - GetKillCamPart1Length());
	const float fEndTime		= (victimData.fLastKillTime + GetKillCamPart2Length());

	for(TMeleeLockQueue::const_iterator iter = m_MeleeLockQueue.begin(), end = m_MeleeLockQueue.end(); iter != end; ++iter)
	{
		if(iter->targetChannelId == victimChannelId && !(fStartTime > iter->fEndLock || fEndTime < iter->fStartLock))
		{
			return true;
		}
	}

	return false;
}

CServerPlayerTracker::SKillCamValidation& CServerPlayerTracker::GetOrCreateVictimData(uint16 victimChannelId)
{
	TKillCamValidationMap::iterator victimData = m_KillCamValidationMap.find(victimChannelId);

	if(victimData == m_KillCamValidationMap.end())
	{
		victimData = m_KillCamValidationMap.insert(std::pair<uint16, SKillCamValidation>(victimChannelId, SKillCamValidation())).first;
	}

	return victimData->second;
}

//Not on death - we need to catch round swaps.
void CServerPlayerTracker::OnPlayerRevived()
{
}

CServerPlayerTracker::~CServerPlayerTracker()
{
}

#endif //SERVER_CHEAT_MONITOR_ENABLED
