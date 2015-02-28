#include "StdAfx.h"

#if defined(SERVER_CHECKS)
#include "PlayerPositionChecker.h"
#include "GameRules.h"
#include "Actor.h"
#include "AntiCheatManager.h"
#include "physinterface.h"
#include "Player.h"
#include "IPlayerInput.h"

CPlayerPositionChecker::CPlayerPositionChecker(void)
{
	m_HeightCheck.nLastPlayerTested = -1;
	m_HeightCheck.rayId = 0;
	m_HeightCheck.fStartTimeInAir = 0.0f;
	m_HeightCheck.bHighFrequencyChecks = false;
	m_HeightCheck.lastPlayerTested = 0;
}

CPlayerPositionChecker::~CPlayerPositionChecker(void)
{
	CancelRayCastRequest(m_HeightCheck);
}

void CPlayerPositionChecker::Update(float fFrameTime)
{
	//TODO: Refactor into UpdateHeightCheck if we add further checks
	
	const float kMaxAirborneTime		= 10.f;

	//rayId will be 0 if the deferred raycast has returned
	CAntiCheatManager * pAntiCheatManager = static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager());
	if(m_HeightCheck.rayId == 0 && pAntiCheatManager->IsCheatProtectionEnabled(eCT_UserFlying))
	{
		if(m_HeightCheck.fStartTimeInAir == 0.0f)
		{
			//If on ground, queue next player for testing
			QueueNextPlayerHeightCheckRayCast( );
		}
		else
		{
			const float fTimeInAir = gEnv->pTimer->GetCurrTime() - m_HeightCheck.fStartTimeInAir;
			if(fTimeInAir > kMaxAirborneTime)
			{
				//The player has been airborne for too long. Flag as a cheat and move on to the next one
				if(IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_HeightCheck.lastPlayerTested))
				{
					g_pGame->GetAntiCheatManager()->FlagActivity(eCT_UserFlying, pActor->GetChannelId(), fTimeInAir);
				}				

				QueueNextPlayerHeightCheckRayCast();
			}
			else
			{
				EntityId playerId = g_pGame->GetGameRules()->GetPlayer(m_HeightCheck.nLastPlayerTested);
				if( m_HeightCheck.lastPlayerTested == playerId)
				{
					//The Nth player is still the same one
					QueueHeightCheckRayCast( playerId );
				}
				else
				{
					//The Nth player is no longer the same player ID. Move on.
					QueueNextPlayerHeightCheckRayCast();
				}
			}
		}
	}
}

bool CPlayerPositionChecker::ShouldCheckPlayer( CPlayer * pPlayer )
{
	return (pPlayer != NULL) && pPlayer->GetSpectatorMode() == CActor::eASM_None && !pPlayer->IsDead() && !pPlayer->IsSwimming();
}

void CPlayerPositionChecker::QueueNextPlayerHeightCheckRayCast( )
{
	CGameRules * pGameRules = g_pGame->GetGameRules();
	IActorSystem * pActorSystem = g_pGame->GetIGameFramework()->GetIActorSystem();

	//We're checking the next player - clear the values for the last player
	m_HeightCheck.fStartTimeInAir = 0.0f;
	m_HeightCheck.bHighFrequencyChecks = false;

	int nPlayerCount = pGameRules->GetPlayerCount();
	if(nPlayerCount > 0)
	{
		int nNextIndex = (m_HeightCheck.nLastPlayerTested + 1) % nPlayerCount;
		int nTestIndex = -1;
		for(int iter = 0, i = nNextIndex; iter < nPlayerCount; iter++, i = (i +  1) % nPlayerCount)
		{
			EntityId playerId = pGameRules->GetPlayer(i);
			CPlayer * pPlayer = static_cast<CPlayer*>(pActorSystem->GetActor(playerId));
			if(ShouldCheckPlayer(pPlayer))
			{
				nTestIndex = i;
				break;
			}
		}

		if(nTestIndex >= 0)
		{
			m_HeightCheck.nLastPlayerTested = nTestIndex;
			EntityId playerId = g_pGame->GetGameRules()->GetPlayer(nTestIndex);
			QueueHeightCheckRayCast(playerId);
		}
	}
}

void CPlayerPositionChecker::QueueHeightCheckRayCast( EntityId playerId )
{
	IEntity * pEntity = gEnv->pEntitySystem->GetEntity(playerId);
	bool bTested = false;
	if(pEntity)
	{
		CPlayer * pPlayer = static_cast<CPlayer*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));
		if(ShouldCheckPlayer(pPlayer))
		{
			//So sorry. Someone didn't make GetState() const in the interface and it's a bit late now.
			IPlayerInput * pPlayerInput = const_cast<IPlayerInput*>(pPlayer->GetPlayerInput());
			if(pPlayerInput)
			{
				SSerializedPlayerInput input;
				pPlayerInput->GetState(input);
				Vec3 posn = input.position;
				posn.z += 0.5f;

				//Add some noise to the position to avoid false positives from players balancing on thin things...
				posn.y += ((cry_frand() - 0.5f) * 0.5f);
				posn.x += ((cry_frand() - 0.5f) * 0.5f);

				Vec3 down(0.0f, 0.0f, -1.5f);
				IPhysicalEntity * pPhysicalEntity = pEntity->GetPhysics();
				int nNumToSkip = pPhysicalEntity ? 1 : 0;
				uint32 flags = rwi_stop_at_pierceable | rwi_ignore_back_faces;
				RayCastRequest request(posn, down, ent_static | ent_rigid | ent_sleeping_rigid | ent_terrain, flags, &pPhysicalEntity, nNumToSkip, 1);
				m_HeightCheck.rayId = g_pGame->GetRayCaster().Queue(RayCastRequest::MediumPriority, request, functor(*this, &CPlayerPositionChecker::OnHeightCheckRayCastDataReceived));
				m_HeightCheck.lastPlayerTested = playerId;
				bTested = true;
			}
		}		
	}
	
	if(!bTested)
	{
		//The player is no longer present. Zero data so we move onto the next player
		m_HeightCheck.fStartTimeInAir = 0.0f;
		m_HeightCheck.bHighFrequencyChecks = false;
		m_HeightCheck.lastPlayerTested = 0;
	}
}

void CPlayerPositionChecker::CancelRayCastRequest( SPlayerRayCast& rPlayerRayCast )
{
	g_pGame->GetRayCaster().Cancel(rPlayerRayCast.rayId);
	rPlayerRayCast.rayId = 0;
}

void CPlayerPositionChecker::OnHeightCheckRayCastDataReceived( const QueuedRayID& rayID, const RayCastResult& result )
{
	if(rayID != m_HeightCheck.rayId)
		return;

	m_HeightCheck.rayId = 0;

	if(result.hitCount == 0)
	{
		//if in air, make sure we are doing high frequency checks
		m_HeightCheck.bHighFrequencyChecks = true;

		//If this is the first point at which we have been in the air, set this as
		//	the time that the user started being in the air
		if(m_HeightCheck.fStartTimeInAir == 0.0f)
			m_HeightCheck.fStartTimeInAir = gEnv->pTimer->GetCurrTime();
	}
	else
	{
		const float kCheatAirborneTime	= 3.5f;
		if(m_HeightCheck.fStartTimeInAir != 0.f)
		{
			const float fTimeInAir = gEnv->pTimer->GetCurrTime() - m_HeightCheck.fStartTimeInAir;
			if(fTimeInAir > kCheatAirborneTime)
			{
				if(IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_HeightCheck.lastPlayerTested))
				{
					g_pGame->GetAntiCheatManager()->FlagActivity(eCT_UserFlying, pActor->GetChannelId(), fTimeInAir);
				}				
			}
		}		

		//Near the ground again. Remove the checks
		m_HeightCheck.bHighFrequencyChecks = false;
		m_HeightCheck.fStartTimeInAir = 0.0f;
	}
}

#endif
