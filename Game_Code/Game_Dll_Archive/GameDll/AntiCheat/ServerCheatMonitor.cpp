/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Central storage and processing of all gamecode based server  
							side monitoring. By centralising, the system can load balance,
							share data, and limit the intrusiveness into general game code. 
							General code is only required to supply samples of data which 
							this monitor processes and if required flags any suspicious
							activity with the IAntiCheatManager interface.

-------------------------------------------------------------------------
History:
- 09:11:2011: Created by Stewart Needham

*************************************************************************/
#include "StdAfx.h"
#include "AntiCheat/ServerCheatMonitor.h"

#if SERVER_CHEAT_MONITOR_ENABLED

#include <INetwork.h>
#include <IGameRulesSystem.h>
#include "../GameRules.h"

#include "IVehicleSystem.h"

#include "AutoAimManager.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"

#include "AntiCheat/ShotCounter.h"
#include "AntiCheatManager.h"
#include "AntiCheat/ServerPlayerTracker.h"
#include "RecordingSystem.h"
#include "WeaponSystem.h"
#include "Player.h"


/*static */ const float CServerCheatMonitor::kMaxHorizontalPlayerSpeed = 10.f;

//========================================================================
// Construction/Destruction
//========================================================================
CServerCheatMonitor::CServerCheatMonitor()
	:m_pFramework(NULL)
{
	m_KillcamPlaybackInstanceData.Reset();
}

	CServerCheatMonitor::~CServerCheatMonitor()
{
	m_pFramework->UnregisterListener(this);
}

//========================================================================
void CServerCheatMonitor::Init(IGameFramework *pFramework)
{
	pFramework->RegisterListener(this,"CServerCheatMonitor", FRAMEWORKLISTENERPRIORITY_DEFAULT);

	m_pFramework = pFramework;
}


//========================================================================
// IGameFrameworkListener Interface
//========================================================================
void CServerCheatMonitor::OnPostUpdate(float fDeltaTime)
{
#ifdef SERVER_CHECKS
	CShotCounter::Update(fDeltaTime);
#endif

	CServerPlayerTracker::UpdateAllPlayerTrackers(fDeltaTime);


























}

void CServerCheatMonitor::OnSaveGame(ISaveGame* pSaveGame)
{
}

void CServerCheatMonitor::OnLoadGame(ILoadGame* pLoadGame)
{
}

void CServerCheatMonitor::OnLevelEnd(const char* nextLevel)
{
}

void CServerCheatMonitor::OnActionEvent(const SActionEvent& event)
{
}

enum EHitValidationStage
{
	eHVS_SelfHarmOtherPlayer = 1,
	eHVS_InvalidDamageType = 2,
	eHVS_WeaponClassMissmatch = 3,
	eHVS_SelfHarmNoWeapon = 4,
	eHVS_InvalidWeaponClass = 5,
	eHVS_NoWeaponClass = 6,
	eHVS_InvalidShooterId = 7,
	eHVS_InvalidHitType = 8,
	eHVS_InvalidWeaponOwner = 9,
	eHVS_InvalidWeaponType = 10,
	eHVS_InvalidHitPos = 11,
	eHVS_InvalidShooterPos = 12,
	eHVS_NoLineOfSight = 13,
};

//-------------------------------------------------------------------------------------
// ::EvaluateHitValidity() is only ever called on the server, for a client hit request.
//-------------------------------------------------------------------------------------
bool CServerCheatMonitor::EvaluateHitValidity(INetChannel * pNetChannel, const HitInfo& rHitInfo) const
{
	CGameRules * pGameRules = g_pGame->GetGameRules();

	uint16 channelId = g_pGame->GetIGameFramework()->GetGameChannelId(pNetChannel);

	if ( IActor* pActor = pGameRules->GetActorByChannelId(channelId) )
	{
		EntityId clientActorId = pActor->GetEntityId();

		const HitTypeInfo * pHitTypeInfo = pGameRules->GetHitTypeInfo(rHitInfo.type);

		//Verify that the hit type is valid
		if(!pHitTypeInfo)
		{
			g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.type, "Invalid hit type supplied");
			return false;
		}

		//Verify that the client actor is either the shooter or the target
		if ( rHitInfo.shooterId != clientActorId && rHitInfo.targetId != clientActorId )
		{
			g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.type, "Shooter id doesn't belong to the channel requesting the shot");
			return false;
		}

		//////////////////////////////////////////////////////////////////////////

		//		Custom validation code for special hit types

		ECustomValidationResult validationResult = eCVR_NoCustomValidation;

		// Perform Custom hit type checks
		if((pHitTypeInfo->m_flags & CGameRules::EHitTypeFlag::CustomValidationRequired) != 0)
		{
			validationResult = CustomValidation(channelId, *pActor, rHitInfo);

			if(validationResult == eCVR_Failed)
				return false;
		}

		//////////////////////////////////////////////////////////////////////////
		
		if((validationResult & (eCVR_NoCustomValidation | eCVR_Passed_DoNormalValidation)) )
		{
			//Make sure that self harm damage types are applying from the shooter to the shooter
			if((pHitTypeInfo->m_flags & CGameRules::EHitTypeFlag::ClientSelfHarm) != 0)
			{
				if(clientActorId != rHitInfo.targetId)
				{
					g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.weaponClassId, "Self harm damage requested on an actor that isn't the client one!");
					return false;
				}
			}

			// Check for damage types that are either server only or not valid in mp
			if (pHitTypeInfo->m_flags & (CGameRules::EHitTypeFlag::SinglePlayerOnly | CGameRules::EHitTypeFlag::Server))
			{
				g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.type, "Hit type specified which clients cannot apply");
				return false;
			}

			//Check the weapon class is present
			if ( rHitInfo.weaponClassId == uint16(~0) )
			{
				g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.type, "No weapon class");
				return false;
			}

			//Check the weapon class is valid
			char className[128];
			bool bSuccess = g_pGame->GetIGameFramework()->GetNetworkSafeClassName(className, sizeof(className), rHitInfo.weaponClassId);
			if (!bSuccess)
			{
				g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.weaponClassId, "Invalid weapon class");
				return false;
			}

			//Check that the weapon is valid, and of the same class as the hit info claims
			if (IEntity *pWeaponEntity = gEnv->pEntitySystem->GetEntity(rHitInfo.weaponId))
			{
				if (stricmp(pWeaponEntity->GetClass()->GetName(), className))
				{
					g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.weaponClassId, "Weapon is not of the specified class");
					return false;
				}
				IItem *pItem=g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(rHitInfo.weaponId);
				if (pItem)
				{
					float stage = 0.f;
					EntityId ownerId = pItem->GetOwnerId();
					if (ownerId == rHitInfo.shooterId)
					{
						bool bValidHit = EvaluateHitLineOfSight(pActor, rHitInfo, stage);
						if (bValidHit == false)
						{
							//g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, stage, "EvalLoS failed");
							return false;
						}
					}
				}
			}
			else if (!pActor->IsDead()) // && (!pWeaponEntity)
			{
				//Code else where will verify that we are not doing damage too long after dying, but at the point of death our inventory can be removed,
				//	resulting in the weapon entity that fired the shot no longer being valid. Unfortunately.
				g_pGame->GetAntiCheatManager()->FlagActivity(eCT_ValidHitInfo, channelId, (float)rHitInfo.weaponClassId, "Requested damage without a valid weapon");
				return false;
			}
		}

		return true;
	}
	else
	{
		CryLog("Hit requested by a channel that doesn't have an actor!");
		return false;
	}
}

void CServerCheatMonitor::NewCloseCombatSnapTarget(const IActor& rPlayer, const IActor * pTargetActor)
{
	if(pTargetActor)
	{
		const Vec3 playerPosn = rPlayer.GetEntity()->GetWorldPos();
		const Vec3 targetPosn = pTargetActor->GetEntity()->GetWorldPos();

		const float fMaxDist				= g_pGameCVars->pl_melee.melee_snap_target_select_range * 2.0f;
		const float fMaxDistSqr			= sqr(fMaxDist);
		const float fActualDistSqr	= playerPosn.GetSquaredDistance(targetPosn);

		if(fActualDistSqr > fMaxDistSqr)
		{
			const float fActualDist	= sqrtf(fActualDistSqr);

			if(CAntiCheatManager * pAntiCheatManager = static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager()))
			{
				pAntiCheatManager->FlagActivity(eCT_MeleeLungeRange, rPlayer.GetChannelId(), fActualDist / fMaxDist);
			}
		}
	}

	if(CServerPlayerTracker * pServerPlayerTracker = GetServerPlayerTracker(rPlayer.GetEntityId()))
	{
		pServerPlayerTracker->NewCloseCombatSnapTarget(pTargetActor);
	}
}

CServerCheatMonitor::ECustomValidationResult CServerCheatMonitor::CustomValidation( uint16 channelId, IActor& rActor, const HitInfo& rHitInfo ) const
{
	switch(rHitInfo.type)
	{
	case CGameRules::EHitType::EnvironmentalMelee:
		return CustomValidation_EnvironmentalMelee( channelId, rActor, rHitInfo );
		break;
	default:
		break;
	}
	
	return eCVR_Passed_SkipNormalValidation;
}

CServerCheatMonitor::ECustomValidationResult CServerCheatMonitor::CustomValidation_EnvironmentalMelee(  uint16 channelId, IActor& rActor, const HitInfo& rHitInfo ) const
{
	//Ensure that we're doing damage to a player within a certain distance of our current position
	float fDistance2D, fDistanceMax, fNetLagSeconds;
	CServerCheatMonitor::GetHitValidationInfo(rActor, rHitInfo, fDistance2D, fDistanceMax, fNetLagSeconds);

	if(IActor * pTargetActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(rHitInfo.targetId))
	{
		if(!CServerCheatMonitor::ValidateTargetActorPositionAgainstHit(*pTargetActor, rHitInfo, fNetLagSeconds))
		{
			return eCVR_Failed;
		}
	}

	const static float kArbitraryMeleeRange = 5.0f;

	const float fExpectedMaxRange = (kArbitraryMeleeRange + (fNetLagSeconds * 2.0f * CServerCheatMonitor::kMaxHorizontalPlayerSpeed));

	if(fDistance2D > fExpectedMaxRange)
	{
		static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager())->FlagActivity(eCT_HitLocationMismatch, channelId, fDistance2D, fExpectedMaxRange, "Attacker was too far away from hit location");
		return eCVR_Failed;
	}

	return eCVR_Passed_SkipNormalValidation;
}

CServerPlayerTracker * CServerCheatMonitor::GetServerPlayerTracker(EntityId playerId) const
{
	CServerPlayerTracker * pServerPlayerTracker = NULL;
	if(IGameObject * pGameObject = g_pGame->GetIGameFramework()->GetGameObject(playerId))
		pServerPlayerTracker = static_cast<CServerPlayerTracker*>(pGameObject->QueryExtension("ServerPlayerTracker"));
	
	return pServerPlayerTracker;
}

void CServerCheatMonitor::OnPlayerRevived(EntityId player)
{
	if(CServerPlayerTracker * pServerPlayerTracker = GetServerPlayerTracker(player))
	{
		pServerPlayerTracker->OnPlayerRevived();
	}
}

void CServerCheatMonitor::KillOccurred(const HitInfo &hitInfo, uint16 victimChannelId, bool bExpectKillCam)
{
	if(CServerPlayerTracker * pServerPlayerTracker = GetServerPlayerTracker(hitInfo.shooterId))
	{
		pServerPlayerTracker->KillOccurred( hitInfo, victimChannelId, bExpectKillCam );
	}
}

void CServerCheatMonitor::StoreKillcamDataPart(IActor * pActor, const CActor::KillCamFPData& packet)
{
 	if(CServerPlayerTracker * pServerPlayerTracker = GetServerPlayerTracker(packet.m_victim))
 	{
 		if(CRecordingSystem * pRecordingSystem = g_pGame->GetRecordingSystem())
 		{
			SKillCamExpectedData fakeExpectation(pActor->GetEntityId(),packet.m_victim,packet.m_bToEveryone);
			CKillCamDataStreamer& streamer = pServerPlayerTracker->GetKillCamStreamer();
			SKillCamStreamData* pStreamData = streamer.ExpectStreamData(fakeExpectation, true);
			
			CRY_ASSERT_MESSAGE(pStreamData,"Should really have a valid data stream.");

#if defined(_RELEASE)
			const float kValidationTime = 4.f;
#else
			const float kValidationTime = 8.f;
#endif
			
			pStreamData->SetValidationTimer(kValidationTime);

			bool bKillCamComplete = CRecordingSystem::ProcessKillCamData(pActor, packet, *pStreamData);

			if(bKillCamComplete)
			{
				SFirstPersonDataContainer* pFirstPersonData = new SFirstPersonDataContainer();

				// Get the data from the completed stream.
				CRecordingSystem::ExtractCompleteStreamData(*pStreamData, GetPlaybackInstanceData(), *pFirstPersonData);

				IActor * pVictimActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(packet.m_victim);
				CServerPlayerTracker * pServerKillerPlayerTracker = GetServerPlayerTracker(pActor->GetEntityId());

				bool bShouldAnalyzeKillcam = true;
				uint16 victimChannelId = 0;

				if(pVictimActor && pServerKillerPlayerTracker)
				{
					victimChannelId = pVictimActor->GetChannelId();
					bShouldAnalyzeKillcam = !pServerKillerPlayerTracker->DidTrackTargetDuringKillcam(packet.m_victim, victimChannelId);
				}

				// Mark us as having received the kill cam data. If they don't send it, they are filthy cheaters.
				if (pVictimActor)
				{
					pServerKillerPlayerTracker->KillCamDataReceived(pVictimActor->GetChannelId());
				}

				// Clear out the now-redundant data	
				GetPlaybackInstanceData().Reset();

				delete pFirstPersonData;
			}
 		}
 	}
}

void CServerCheatMonitor::OnClientConnect(uint16 channelId)
{
	if (CAntiCheatManager * pAntiCheatManager = static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager()))
	{
		if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
		{
			pAntiCheatManager->OnClientConnect(*pNetChannel);
		}
	}	
}

void CServerCheatMonitor::OnClientDisconnect(uint16 channelId, IActor *pActor)
{
	CServerPlayerTracker::OnClientDisconnect(channelId);

	if (CAntiCheatManager * pAntiCheatManager = static_cast<CAntiCheatManager*>(g_pGame->GetAntiCheatManager()))
	{
		if (INetChannel *pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId))
		{
			pAntiCheatManager->OnClientDisconnect(*pNetChannel, pActor);
		}
	}
}

/*static */ bool CServerCheatMonitor::ValidateTargetActorPositionAgainstHit(IActor& rTargetActor, const HitInfo& rHitInfo, float fNetLagSeconds)
{
	const Vec3& vTargetPosition = rTargetActor.GetEntity()->GetWorldPos();
	float fHitDifferential = sqrt_fast_tpl(rHitInfo.pos.GetSquaredDistance2D(vTargetPosition));

	//A large fudge factor for this one, as it doesn't need to be precise and it's going to be quite heavily affected by lag spikes
	const float fMaxDistance = (fHitDifferential - (rHitInfo.radius + 1.0f));
	const float fMismatch = fMaxDistance - (((fNetLagSeconds * 2.0f) + 0.5f) * CServerCheatMonitor::kMaxHorizontalPlayerSpeed);
	
	if(fMismatch > 2.0f)
	{
		if(IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(rHitInfo.shooterId))
		{
			g_pGame->GetAntiCheatManager()->FlagActivity(eCT_HitLocationMismatch, pActor->GetChannelId(), fMismatch, "Target was too far away from reported hit location");
		}

		return false;
	}
	else
	{
		return true;
	}
}

/*static*/ void CServerCheatMonitor::GetHitValidationInfo(const IActor& pShooterActor, const HitInfo& hitInfo, float& fDistance2D, float& fDistanceMax, float& fNetLagSeconds)
{
	const INetChannel*  pNetChannel = gEnv->pGame->GetIGameFramework()->GetNetChannel(pShooterActor.GetChannelId());
	fNetLagSeconds = ((pShooterActor.IsClient() || !pNetChannel) ? 0 : pNetChannel->GetPing(true));  // GetPing() is the round journey but we're giving some leeway

	const Vec3& vCurrentPosition = pShooterActor.GetEntity()->GetWorldPos();

	fDistance2D			= (vCurrentPosition - hitInfo.pos).GetLength2D();
	fDistanceMax	= vCurrentPosition.GetDistance(hitInfo.pos);
}

bool CServerCheatMonitor::EvaluateHitLineOfSight(IActor* pActor, const HitInfo& rHitInfo, float &stage) const
{
	float maxLatency = gEnv->pPhysicalWorld->GetPhysVars()->netInterpTime + 0.05f;
	const INetChannel* pNetChannel = gEnv->pGame->GetIGameFramework()->GetNetChannel(pActor->GetChannelId());
	if (pNetChannel)
	{
		maxLatency += pNetChannel->GetPing(true);
	}
	IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(rHitInfo.targetId);
	if (pTargetEntity)
	{
		IPhysicalEntity* pTargetPhysics = pTargetEntity->GetPhysics();
		IActor* pTargetActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(rHitInfo.targetId);
		if (pTargetActor)
		{
			IVehicle* pVehicle = pTargetActor->GetLinkedVehicle();
			if (pVehicle)
			{
				pTargetPhysics = pVehicle->GetEntity()->GetPhysics();
			}
		}
		float targetPosTolerance = 2.0f;
		if (pTargetPhysics)
		{
			pe_status_dynamics dynamics;
			pTargetPhysics->GetStatus(&dynamics);
			float speed = dynamics.v.GetLength();
			targetPosTolerance += speed * maxLatency;
		}
		AABB targetBounds;
		pTargetEntity->GetWorldBounds(targetBounds);
		float distanceSq = Distance::Point_AABBSq(rHitInfo.pos, targetBounds);
		if (distanceSq <= targetPosTolerance * targetPosTolerance)
		{
			IEntity* pShooterEntity = NULL;
			IVehicle* pVehicle = pActor->GetLinkedVehicle();
			if (pVehicle)
			{
				pShooterEntity = pVehicle->GetEntity();
			}
			else
			{
				pShooterEntity = pActor->GetEntity();
			}
			IPhysicalEntity* pShooterPhysics = pShooterEntity->GetPhysics();
			float shooterPosTolerance = 2.0f;
			if (pShooterPhysics)
			{
				pe_status_dynamics dynamics;
				pShooterPhysics->GetStatus(&dynamics);
				float speed = dynamics.v.GetLength();
				shooterPosTolerance += speed * maxLatency;
			}
			Vec3 shooterPos = pShooterEntity->GetWorldPos();
			Vec3 delta = rHitInfo.pos - shooterPos;
			float u = delta.dot(rHitInfo.dir);
			Vec3 estimatedOrigin = rHitInfo.pos - u * rHitInfo.dir;
			AABB shooterBounds;
			pShooterEntity->GetWorldBounds(shooterBounds);
			distanceSq = Distance::Point_AABBSq(estimatedOrigin, shooterBounds);
			if (distanceSq <= shooterPosTolerance * shooterPosTolerance)
			{
				uint8 pierceability = 8;
				char className[128];
				bool bSuccess = g_pGame->GetIGameFramework()->GetNetworkSafeClassName(className, sizeof(className), rHitInfo.projectileClassId);
				if (bSuccess)
				{
					IEntityClass* pAmmoClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className);
					if (pAmmoClass)
					{
						const SAmmoParams* pAmmoParams = g_pGame->GetWeaponSystem()->GetAmmoParams(pAmmoClass);
						if (pAmmoParams && pAmmoParams->pParticleParams && !is_unused(pAmmoParams->pParticleParams->iPierceability))
						{
							pierceability=pAmmoParams->pParticleParams->iPierceability;
						}
					}
				}
				ray_hit hit;
				if (u <= 1.0f)
				{
					// Very short range, don't need to bother with line of sight test
					return true;
				}
				// Shrink the ray cast in by half a meter on both ends to avoid false positives
				Vec3 rayOrig = estimatedOrigin + 0.5f * rHitInfo.dir;
				Vec3 rayDir = rHitInfo.dir * (u - 1.0f);
				if (BulletWorldIntersection(rayOrig, rayDir, pierceability, hit))
				{
					stage = eHVS_NoLineOfSight;
					CryLog("CServerCheatMonitor::EvaluateHitLineOfSight No line of sight from shooter '%s' to target '%s', origin: %f, %f, %f target: %f, %f, %f hit point: %f, %f, %f, hit dist: %f, total dist: %f", pShooterEntity->GetName(), pTargetEntity->GetName(),
						estimatedOrigin.x, estimatedOrigin.y, estimatedOrigin.z, rHitInfo.pos.x, rHitInfo.pos.y, rHitInfo.pos.z, hit.pt.x, hit.pt.y, hit.pt.z, hit.dist, u);
					if (g_pGameCVars->g_dedi_hitValidateDebugDrawTime > 0.0f)
					{
						IPersistantDebug* pPersistantDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
						if (pPersistantDebug)
						{
							Vec3 p0 = estimatedOrigin;
							Vec3 p1 = rayOrig;
							Vec3 p2 = rayOrig + hit.dist * rHitInfo.dir;
							Vec3 p3 = rayOrig + rayDir;
							Vec3 p4 = rHitInfo.pos;
							float drawTime = g_pGameCVars->g_dedi_hitValidateDebugDrawTime;
							pPersistantDebug->Begin("Invalid Hit", false);
							pPersistantDebug->AddLine(p0, p1, Col_Gray, drawTime);	// Part of ray which is not tested for intersections
							pPersistantDebug->AddLine(p1, p2, Col_Green, drawTime);	// Part of ray which doesn't intersect with anything
							pPersistantDebug->AddLine(p2, p3, Col_Red, drawTime);		// Part of ray which failed intersection validation
							pPersistantDebug->AddLine(p3, p4, Col_Gray, drawTime);	// Part of ray which is not tested for intersections
						}
					}
				}
				else
				{
					return true;
				}
			}
			else
			{
				stage = eHVS_InvalidShooterPos;
				float distance = sqrtf(distanceSq);
				CryLog("CServerCheatMonitor::EvaluateHitLineOfSight Estimated shooter position is too far away from shooter entity %s (distance: %f, max allowed distance: %f)!", pShooterEntity->GetName(), distance, shooterPosTolerance);
			}
		}
		else
		{
			stage = eHVS_InvalidHitPos;
			float distance = sqrtf(distanceSq);
			Vec3 targetWorldPos = pTargetEntity->GetWorldPos();
			CryLog("CServerCheatMonitor::EvaluateHitLineOfSight Hit position (%f, %f, %f) is too far away from target entity %s (%f, %f, %f) (distance: %f, max allowed distance: %f)!",
				rHitInfo.pos.x, rHitInfo.pos.y, rHitInfo.pos.z, pTargetEntity->GetName(), targetWorldPos.x, targetWorldPos.y, targetWorldPos.z, distance, targetPosTolerance);
		}
	}
	else
	{
		return true;	// When shooting destroyable objects the target ID can be 0
	}
	return false;
}

bool CServerCheatMonitor::BulletWorldIntersection(const Vec3 &org, const Vec3 &dir, int pierceability, ray_hit &hit) const
{
	uint32 flags=(geom_colltype_ray|geom_colltype13)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces;	// Using the same flags as CSingle::GetProbableHit
	std::set<IRenderNode*> renderNodes;
	const int maxHits = 8;
	ray_hit hits[maxHits];
	Vec3 dest = org + dir;
	int numHits = gEnv->pPhysicalWorld->RayWorldIntersection(org, dir, ent_static|ent_terrain, flags, hits, maxHits);
	if (numHits)
	{
		int start = 0;
		int end = numHits;
		if (hits[0].dist == -1.0f)
		{
			// hits[0] is always the final non-pierceable hit, if there is no such hit then hits start at index 1
			start = 1;
			end = min(numHits+1, maxHits);
		}
		for (int i=start; i<end; ++i)
		{
			if (hits[i].bTerrain)
			{
				hit = hits[i];
				return true;
			}
			else if (hits[i].pCollider)
			{
				IRenderNode *pRenderNode = (IRenderNode*)hits[i].pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
				if (pRenderNode)
				{
					std::pair<std::set<IRenderNode*>::iterator, bool> result = renderNodes.insert(pRenderNode);	// Make sure we don't check the same render node twice for efficiency
					if (result.second)
					{
						Vec3 hitPos;
						int surfaceTypeId;
						if (LineSegIntersectsStatObj(pRenderNode, org, dest, hitPos, surfaceTypeId))
						{
							bool allowThrough = false;
							ISurfaceTypeManager* pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
							if (pSurfaceTypeManager)
							{
								// Deal with pierceability against render mesh surface type here
								ISurfaceType* pSurfaceType = pSurfaceTypeManager->GetSurfaceType(surfaceTypeId);
								allowThrough = pierceability < pSurfaceType->GetPhyscalParams().pierceability;
							}
							if (!allowThrough)
							{
								hit = hits[i];
								hit.pt = hitPos;
								hit.dist = org.GetDistance(hitPos);
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool CServerCheatMonitor::LineSegIntersectsStatObj(IRenderNode *pRenderNode, const Vec3 &orig, const Vec3 &dest, Vec3 &hitPos, int &surfaceTypeId) const
{
	// Does similar checks to what CRenderMeshUtils::RayIntersectionImpl is doing
	if (pRenderNode->GetRenderNodeType() == eERType_Brush)
	{
		const Matrix34& transform = ((IBrush*)pRenderNode)->GetMatrix();
		// Transform the line segment into the brush's local space
		Matrix34 inverse = transform.GetInverted();
		Lineseg lineSeg(inverse.TransformPoint(orig), inverse.TransformPoint(dest));

		IStatObj* pStatObj = pRenderNode->GetEntityStatObj();
		if (pStatObj)
		{
			if (pStatObj->LineSegIntersection(lineSeg, hitPos, surfaceTypeId))
			{
				hitPos = transform.TransformPoint(hitPos);	// Transform hit pos into world space
				return true;
			}
		}
	}
	return false;
}

#ifndef _RELEASE

void CServerCheatMonitor::DebugBulletWorldIntersection()
{
	const CCamera& camera = gEnv->pRenderer->GetCamera();

	static Vec3 pos = ZERO;
	static Vec3 dir = Vec3(0.0f, 0.0f, 1.0f);

	static bool updatePos = true;
	if (updatePos)
	{
		pos = camera.GetPosition();
		dir = camera.GetViewdir();
	}

	ray_hit hit;
	if (BulletWorldIntersection(pos, dir * 2000.0f, 8, hit))
	{
		IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		pAuxGeom->DrawSphere(hit.pt, 0.1f, Col_Red);
	}
}

void CServerCheatMonitor::CmdLoadRayCasts(IConsoleCmdArgs* pArgs)
{
	IPersistantDebug* pPersistantDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
	if (!pPersistantDebug)
	{
		CryLogAlways("GetIPersistantDebug is NULL!");
		return;
	}

	if (pArgs->GetArgCount() > 2)
	{
		CryLogAlways("usage: %s [filename]", pArgs->GetArg(0));
	}
	else
	{
		CCryFile file;
		stack_string filename;
		if (pArgs->GetArgCount() > 1)
		{
			filename = pArgs->GetArg(1);
		}
		else
		{
			const char* pLevelName = gEnv->pGame->GetIGameFramework()->GetLevelName();
			const char* pTrimmedLevelName = strrchr(pLevelName, '/');
			if (pTrimmedLevelName)
			{
				pTrimmedLevelName++;
			}
			else
			{
				pTrimmedLevelName = pLevelName;
			}
			filename.Format("%s.rays", pTrimmedLevelName);
		}
		if (file.Open(filename.c_str(), "r", ICryPak::FOPEN_HINT_QUIET | ICryPak::FOPEN_ONDISK))
		{
			const size_t fileSize = file.GetLength();
			char* pBuffer = new char [fileSize];

			file.ReadRaw(pBuffer, fileSize);
  
			pPersistantDebug->Begin("Loaded Ray Casts", true);

			Vec3 source, target, hit;
			const char* pLineStart = pBuffer;
			while (true)
			{
				int res = sscanf(pLineStart, "%f, %f, %f - %f, %f, %f - %f, %f, %f", &source.x, &source.y, &source.z, &target.x, &target.y, &target.z, &hit.x, &hit.y, &hit.z);
				if (res != 9)
					break;

				const float drawTime = FLT_MAX;
				pPersistantDebug->AddLine(source, hit, Col_Green, drawTime);
				pPersistantDebug->AddLine(hit, target, Col_Red, drawTime);

				while (pLineStart[0] && pLineStart[0] != '\n')
					++pLineStart;
				if (pLineStart[0])
					++pLineStart;
			}

			SAFE_DELETE_ARRAY(pBuffer);
		}
		else
		{
			CryLogAlways("Failed to open %s", filename.c_str());
		}
	}
}
#endif //_RELEASE

#endif //SERVER_CHEAT_MONITOR_ENABLED
