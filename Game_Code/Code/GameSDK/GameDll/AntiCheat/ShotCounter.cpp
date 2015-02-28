#include "StdAfx.h"

#include "ShotCounter.h"

#ifdef SERVER_CHECKS	

#include "GameCVars.h"
#include <IItemSystem.h>
#include <AntiCheatManager.h>

#if defined(_RELEASE)
#define SHOTDBGWATCH(...)do{}while(0)
#else
#include "Utility/CryWatch.h"
#define SHOTDBGWATCH(...)                                             \
{                                                                     \
	const SAntiCheatCVars& acCvars = g_pGame->GetCVars()->anticheat;    \
	if(acCvars.dbg.watch)                                               \
	{                                                                   \
		CryWatch(__VA_ARGS__);                                            \
	}                                                                   \
}                                                        
#endif // !defined(_RELEASE)

DEFINE_INTRUSIVE_LINKED_LIST(CShotCounter)

CShotCounter::CShotCounter(const EntityId ownerPlayer)
: m_ownerPlayer(ownerPlayer)
, m_headshotHits(0)
, m_hits(0)
, m_shotCount(0)
, m_longestHeadshotStreak(0)
, m_currentHeadshotStreak(0)
, m_changeSinceLastCheck(false)
{
	// ...
}

CShotCounter::~CShotCounter()
{
	// ...
}

void CShotCounter::RecordHit( const HitInfo &hitInfo, const bool isHeadshot )
{
	m_changeSinceLastCheck = true;

	if(isHeadshot)
	{
		m_headshotHits++;
		m_currentHeadshotStreak++;
		m_longestHeadshotStreak = MAX(m_longestHeadshotStreak, m_currentHeadshotStreak);
	}
	else
	{
		m_currentHeadshotStreak = 0;
	}

	m_hits++;

	CAntiCheatManager * pAntiCheatManager = static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager());

	const float fCurrentTime = hitInfo.time.GetSeconds();
	IActor * pShooter = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.shooterId);

	if(pShooter && m_shotCount > 10)
	{
		const float kMinAccuracyToStartChecks = pAntiCheatManager->GetAntiCheatVar(eAV_AB_MinAcc, 0.10f);
		const float kFastShotTime		= pAntiCheatManager->GetAntiCheatVar(eAV_AB_AimTime,	0.30f);
		const float kAcceptableDot	= pAntiCheatManager->GetAntiCheatVar(eAV_AB_AimDot,		0.33f);


		//This accuracy check isn't perfect but it should mean that we avoid flagging people who are spinning around like crazy
		//	and occasionally landing shots on targets that are a large angle apart
		if(((float)m_hits / (float)m_shotCount) > kMinAccuracyToStartChecks)
		{
			Vec3 dir = (hitInfo.pos - pShooter->GetEntity()->GetWorldPos());
			dir.Normalize();

			//Make sure that we have switched targets from one hit to another
			if(hitInfo.targetId != m_LastHit.hitInfo.targetId)
			{
				//Ensure this is a player target. Is there a faster way of doing this?
				if(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(hitInfo.targetId))
				{			
					const float fTimeDiff = fCurrentTime - m_LastHit.hitInfo.time.GetSeconds();
					const float fDot			= m_LastHit.dir.dot(dir);
					if(fTimeDiff < kFastShotTime && fTimeDiff >= 0.0f)
					{
						if(fDot < kAcceptableDot)
						{
							pAntiCheatManager->FlagActivity(eCT_AimBot, pShooter->GetChannelId(), fTimeDiff, fDot);
						}
					}
				}				
			}

			m_LastHit = HitInfoWithDir(hitInfo, dir);
		}
	}
}
//~Record input

//Evaluate
void CShotCounter::EvaluateCheatState()
{
	EvaluateTooManyHeadShots();
}

void CShotCounter::EvaluateTooManyHeadShots()
{
	if(!m_changeSinceLastCheck)
	{
		return;
	}

	m_changeSinceLastCheck = false;

	CAntiCheatManager* pAntiCheatManager	= static_cast<CAntiCheatManager*>(gEnv->pGame->GetAntiCheatManager());
	const float kMaxContiguousHeadShots		= pAntiCheatManager->GetAntiCheatVar(eAV_HS_MaxContiguous,	5.0f);

	if(IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_ownerPlayer))
	{
		// Check that the max number of consecutive head shots is less than the configured number
		if(m_currentHeadshotStreak > kMaxContiguousHeadShots)
		{
			pAntiCheatManager->FlagActivity(eCT_Headshots_ContiguousHits, pActor->GetChannelId(), float(m_currentHeadshotStreak));
		}

		const float kMinHitsRequired		= pAntiCheatManager->GetAntiCheatVar(eAV_HS_MinHitsRequired,	10.0f);
		if(m_hits > kMinHitsRequired)
		{
			// don't start flagging users before this limit has been hit
			const float headShotFraction = (float)m_headshotHits * __fres( m_hits );
			if( headShotFraction > pAntiCheatManager->GetAntiCheatVar(eAV_HS_MaxRatio,	0.05f))
			{
				pAntiCheatManager->FlagActivity(eCT_Headshots_RatioTooHigh, pActor->GetChannelId(), headShotFraction, m_headshotHits);
			}
		}
	}
}
//~Evaluate

void CShotCounter::Update(float fDeltaTime)
{
	CShotCounter *pShotCounter= CShotCounter::get_intrusive_list_root();
	while (pShotCounter)
	{
		pShotCounter->EvaluateCheatState();
		pShotCounter = pShotCounter->m_next_intrusive;
	}
}

#endif //def SERVER_CHECKS
