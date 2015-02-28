// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "stdafx.h"
#include "EntityAudioProxy.h"
#include <IAudioSystem.h>
#include <ICryAnimation.h>
#include "Entity.h"

SATLWorldPosition const CEntityAudioProxy::s_oNULLOffset;
CEntityAudioProxy::TAudioProxyPair CEntityAudioProxy::s_oNULLAudioProxyPair(INVALID_AUDIO_PROXY_ID, static_cast<IAudioProxy*>(NULL));

//////////////////////////////////////////////////////////////////////////
CEntityAudioProxy::CEntityAudioProxy()
	:	m_pEntity(NULL)
	,	m_nAudioProxyIDCounter(INVALID_AUDIO_PROXY_ID)
	,	m_nAudioEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
	,	m_bHide(false)
	,	m_fFadeDistance(0.0f)
	,	m_fEnvironmentFadeDistance(0.0f)
	,	m_nBoneHead(-1)
	,	m_nAttachmentIndex(-1)
{
}

//////////////////////////////////////////////////////////////////////////
CEntityAudioProxy::~CEntityAudioProxy()
{
	m_pEntity = NULL;
	std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SReleaseAudioProxy());
	m_mapAuxAudioProxies.clear();
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::Initialize(SComponentInitializer const& init)
{
	m_pEntity = static_cast<CEntity*>(init.m_pEntity);
	assert(m_mapAuxAudioProxies.empty());

	// Creating the default AudioProxy.
	CreateAuxAudioProxy();

	m_bHide	= m_pEntity->IsHidden();
	SetObstructionCalcType(eAOOCT_IGNORE);
	OnMove();
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
	m_pEntity										= static_cast<CEntity*>(pEntity);
	m_bHide											= m_pEntity->IsHidden();
	m_fFadeDistance							= 0.0f;
	m_fEnvironmentFadeDistance	= 0.0f;
	m_nBoneHead									= -1;
	m_nAttachmentIndex					= -1;
	m_nAudioEnvironmentID				= INVALID_AUDIO_ENVIRONMENT_ID;

	std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SResetAudioProxy());

#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
	std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SInitializeAudioProxy(m_pEntity->GetName()));
#else
	std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SInitializeAudioProxy(NULL));
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

	SetObstructionCalcType(eAOOCT_IGNORE);
	OnMove();

	REINST("needs voice attachement placement");
	//PrecacheHeadBone();
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::PrecacheHeadBone()
{
	// checking for and caching HeadBone
	if (m_nBoneHead == -1 && m_nAttachmentIndex == -1)
	{
		if (ICharacterInstance* const pCharacter = m_pEntity->GetCharacter(0))
		{
			if (IAttachmentManager const* const pAttachmentManager = pCharacter->GetIAttachmentManager())
			{
				m_nAttachmentIndex = pAttachmentManager->GetIndexByName("voice");

				if (m_nAttachmentIndex == -1)
				{
					// There's no attachment so let's try to find the head bone.
					IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
					m_nBoneHead = rIDefaultSkeleton.GetJointIDByName("Bip01 Head");
					if (m_nBoneHead == -1)
					{
						// Has it been named differently?
						m_nBoneHead = rIDefaultSkeleton.GetJointIDByName("def_head");
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnMove()
{
	Matrix34 tm = m_pEntity->GetWorldTM();

	if (tm.IsValid())
	{
		SATLWorldPosition const oPosition(tm, ZERO);
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SRepositionAudioProxy(oPosition));

		if ((m_pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
		{
			SAudioRequest oRequest;
			oRequest.nFlags = eARF_PRIORITY_NORMAL;
			oRequest.pOwner = this;

			SAudioListenerRequestData<eALRT_SET_POSITION> oRequestData(oPosition);
			oRequest.pData = &oRequestData;

			gEnv->pAudioSystem->PushRequest(oRequest);

			// As this is an audio listener add its entity to the AreaManager for raising audio relevant events.
			gEnv->pEntitySystem->GetAreaManager()->MarkEntityForUpdate(m_pEntity->GetId());
		}
	}

	REINST("voice attachement lookup needs to be done by code creating entityaudioproxy for voice line on character");
	//if (m_nAttachmentIndex != -1)
	//{
	//	if (ICharacterInstance* const pCharacter = m_pEntity->GetCharacter(0))
	//	{
	//		if (m_nAttachmentIndex != -1)
	//		{
	//			if (IAttachmentManager const* const pAttachmentManager = pCharacter->GetIAttachmentManager())
	//			{
	//				if (IAttachment const* const pAttachment = pAttachmentManager->GetInterfaceByIndex(m_nAttachmentIndex))
	//				{
	//					tm = Matrix34(pAttachment->GetAttWorldAbsolute());
	//				}
	//			}
	//		}
	//		else if (m_nBoneHead != -1)
	//		{
	//			// re-query SkeletonPose to prevent crash on removed Character
	//			if (ISkeletonPose* const pSkeletonPose = pCharacter->GetISkeletonPose())
	//			{
	//				tm = tm * Matrix34(pSkeletonPose->GetAbsJointByID(m_nBoneHead));
	//			}
	//		}
	//	}
	//}
}
//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnListenerMoveInside(IEntity const* const pEntity)
{
	m_pEntity->SetPos(pEntity->GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnMoveInside(IEntity const* const pEntity)
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnListenerExclusiveMoveInside(IEntity const* const __restrict pEntity, IEntity const* const __restrict pAreaHigh, IEntity const* const __restrict pAreaLow, float const fFade)
{
	IEntityAreaProxy const* const __restrict pAreaProxyLow	= static_cast<IEntityAreaProxy const* const __restrict>(pAreaLow->GetProxy(ENTITY_PROXY_AREA));
	IEntityAreaProxy* const __restrict pAreaProxyHigh				= static_cast<IEntityAreaProxy* const __restrict>(pAreaHigh->GetProxy(ENTITY_PROXY_AREA));

	if (pAreaProxyLow != NULL && pAreaProxyHigh != NULL)
	{
		Vec3 OnHighHull3d(ZERO);
		Vec3 const oPos(pEntity->GetWorldPos());
		EntityId const nEntityID = pEntity->GetId();
		bool const bInsideLow = pAreaProxyLow->CalcPointWithin(nEntityID, oPos);

		if (bInsideLow)
		{
			float const fDistSq = pAreaProxyHigh->ClosestPointOnHullDistSq(nEntityID, oPos, OnHighHull3d);
			m_pEntity->SetPos(OnHighHull3d);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnExclusiveMoveInside(IEntity const* const __restrict pEntity, IEntity const* const __restrict pEntityAreaHigh, IEntity const* const __restrict pEntityAreaLow, float const fEnvironmentFade)
{
	SetEnvironmentAmountInternal(pEntity, fEnvironmentFade);
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnListenerEnter(IEntity const* const pEntity)
{
	m_pEntity->SetPos(pEntity->GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnEnter(IEntity const* const pEntity)
{
	SetEnvironmentAmountInternal(pEntity, 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnLeaveNear(IEntity const* const pEntity)
{
	SetEnvironmentAmountInternal(pEntity, 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnAreaCrossing()
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnListenerMoveNear(IEntity const* const __restrict pEntity, IEntity const* const __restrict pArea)
{
	IEntityAreaProxy* const pAreaProxy = static_cast<IEntityAreaProxy*>(pArea->GetProxy(ENTITY_PROXY_AREA));

	if (pAreaProxy != NULL)
	{
		Vec3 OnHull3d(ZERO);
		float const fDistSq = pAreaProxy->CalcPointNearDistSq(pEntity->GetId(), pEntity->GetWorldPos(), OnHull3d);
		m_pEntity->SetPos(OnHull3d);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnMoveNear(IEntity const* const __restrict pEntity, IEntity const* const __restrict pArea)
{
	IEntityAudioProxy* const pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
	IEntityAreaProxy* const pAreaProxy = static_cast<IEntityAreaProxy*>(pArea->GetProxy(ENTITY_PROXY_AREA));

	if ((pIEntityAudioProxy != NULL) && (pAreaProxy != NULL) && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
	{
		// Only set the Audio Environment Amount on the entities that already have an AudioProxy.
		// Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pEntityAudioProxy.
		Vec3 OnHull3d(ZERO);
		float const fDist = sqrt(pAreaProxy->CalcPointNearDistSq(pEntity->GetId(), pEntity->GetWorldPos(), OnHull3d));

		if ((fDist > m_fEnvironmentFadeDistance) || (m_fEnvironmentFadeDistance < 0.0001f))
		{
			pIEntityAudioProxy->SetEnvironmentAmount(m_nAudioEnvironmentID, 0.0f, INVALID_AUDIO_PROXY_ID);
		}
		else
		{
			pIEntityAudioProxy->SetEnvironmentAmount(m_nAudioEnvironmentID, 1.0f - (fDist/m_fEnvironmentFadeDistance), INVALID_AUDIO_PROXY_ID);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::OnHide(bool const bHide)
{
	m_bHide = bHide;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::ProcessEvent( SEntityEvent &event )
{
	if (m_pEntity != NULL)
	{
		switch (event.event)
		{
		case ENTITY_EVENT_XFORM:
			{
				int const nFlags = (int)event.nParam[0];

				if ((nFlags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT)) != 0)
				{
					OnMove();
				}

				break;
			}
		case ENTITY_EVENT_ENTERAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
				{
					EntityId const nEntityID = static_cast<EntityId>(event.nParam[0]); // Entering entity!
					IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

					if ((pEntity != NULL) && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
					{
						OnListenerEnter(pEntity);
					}
					else if ((pEntity != NULL) && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
					{
						OnEnter(pEntity);
					}
				}

				break;
			}
		case ENTITY_EVENT_LEAVEAREA:
			{

				break;
			}
		case ENTITY_EVENT_CROSS_AREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
				{
					EntityId const nEntityID = static_cast<EntityId>(event.nParam[0]); // Crossing entity!
					IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

					if ((pEntity != NULL) && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
					{
						OnAreaCrossing();
					}
				}

				break;
			}
		case ENTITY_EVENT_MOVENEARAREA:
		case ENTITY_EVENT_ENTERNEARAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
				{
					EntityId const nEntityID = static_cast<EntityId>(event.nParam[0]); // Near entering/moving entity!
					IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

					EntityId const nAreaEntityID = (EntityId)event.nParam[2];
					IEntity* const pArea = gEnv->pEntitySystem->GetEntity(nAreaEntityID);

					if ((pEntity != NULL) && (pArea != NULL))
					{
						if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
						{
							// This entity is an audio listener.
							OnListenerMoveNear(pEntity, pArea);
						}
						else if (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID)
						{
							OnMoveNear(pEntity, pArea);
						}
					}
				}

				break;
			}
		case ENTITY_EVENT_LEAVENEARAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
				{
					EntityId const nEntityID = static_cast<EntityId>(event.nParam[0]); // Leaving entity!
					IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

					if ((pEntity != NULL) && (pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) == 0)
					{
						OnLeaveNear(pEntity);
					}
				}
				break;
			}
		case ENTITY_EVENT_MOVEINSIDEAREA:
			{
				if ((m_pEntity->GetFlags() & ENTITY_FLAG_VOLUME_SOUND) != 0)
				{
					EntityId const nEntityID = static_cast<EntityId>(event.nParam[0]); // Inside moving entity!
					IEntity* const __restrict pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

					if (pEntity != NULL)
					{
						EntityId const nAreaID1 = static_cast<EntityId>(event.nParam[2]); // AreaEntityID (low)
						EntityId const nAreaID2 = static_cast<EntityId>(event.nParam[3]); // AreaEntityID (high)

						IEntity* const __restrict pArea1 = gEnv->pEntitySystem->GetEntity(nAreaID1);
						IEntity* const __restrict pArea2 = gEnv->pEntitySystem->GetEntity(nAreaID2);


						if (pArea1 != NULL)
						{
							if (pArea2 != NULL)
							{
								if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
								{
									OnListenerExclusiveMoveInside(pEntity, pArea2, pArea1, event.fParam[0]);
								}
								else
								{
									OnExclusiveMoveInside(pEntity, pArea2, pArea1, event.fParam[1]);
								}
							}
							else
							{
								if ((pEntity->GetFlagsExtended() & ENTITY_FLAG_EXTENDED_AUDIO_LISTENER) != 0)
								{
									OnListenerMoveInside(pEntity);
								}
								else
								{
									OnMoveInside(pEntity);
								}
							}
						}
					}
				}

				break;
			}
		case ENTITY_EVENT_ANIM_EVENT:
			{
				REINST("reintroduce anim event voice playing in EntityAudioProxy")
					/*if (!IsSoundAnimEventsHandledExternally())
					{
					const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
					ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
					const char* eventName = (pAnimEvent ? pAnimEvent->m_EventName : 0);
					if (eventName && stricmp(eventName, "sound") == 0)
					{
					Vec3 offset(ZERO);
					if (pAnimEvent->m_BonePathName && pAnimEvent->m_BonePathName[0])
					{
					if (pCharacter)
					{
					IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
					int id = rIDefaultSkeleton.GetJointIDByName(pAnimEvent->m_BonePathName);
					if (id >= 0)
					{
					ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
					QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
					offset = boneQuat.t;
					}
					}
					}

					int flags = FLAG_SOUND_DEFAULT_3D;
					if (strchr(pAnimEvent->m_CustomParameter, ':') == NULL)
					flags |= FLAG_SOUND_VOICE;
					PlaySound(pAnimEvent->m_CustomParameter, offset, FORWARD_DIRECTION, flags, 0, eSoundSemantic_Animation, 0, 0);
					}
					}*/

					break;
			}
		case ENTITY_EVENT_HIDE:
			OnHide(true);
			break;
		case ENTITY_EVENT_UNHIDE:
			OnHide(false);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CEntityAudioProxy::GetSignature(TSerialize signature)
{
	// EntityAudioProxy is not relevant to signature as it is always created again if needed
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::Serialize(TSerialize ser)
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::ExecuteTrigger(TAudioControlID const nTriggerID, ELipSyncMethod const eLipSyncMethod, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	ExecuteTriggerInternal(nTriggerID, eLipSyncMethod, NULL, NULL, nAudioProxyLocalID);
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::ExecuteTrigger(TAudioControlID const nTriggerID, ELipSyncMethod const eLipSyncMethod, TTriggerFinishedCallback const pCallback, void* const pCallbackCookie, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	ExecuteTriggerInternal(nTriggerID, eLipSyncMethod, pCallback, pCallbackCookie, nAudioProxyLocalID);
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::StopTrigger(TAudioControlID const nTriggerID, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			(SStopTrigger(nTriggerID))(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SStopTrigger(nTriggerID));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetSwitchState(TAudioControlID const nSwitchID, TAudioSwitchStateID const nStateID, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			(SSetSwitchState(nSwitchID, nStateID))(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetSwitchState(nSwitchID, nStateID));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetRtpcValue(TAudioControlID const nRtpcID, float const fValue, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			(SSetRtpcValue(nRtpcID, fValue))(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetRtpcValue(nRtpcID, fValue));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetObstructionCalcType(EAudioObjectObstructionCalcType const eObstructionType, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			(SSetObstructionCalcType(eObstructionType))(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetObstructionCalcType(eObstructionType));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetEnvironmentAmount(TAudioEnvironmentID const nEnvironmentID, float const fAmount, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			SSetEnvironmentAmount(nEnvironmentID, fAmount)(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetEnvironmentAmount(nEnvironmentID, fAmount));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetCurrentEnvironments(TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			SSetCurrentEnvironments(m_pEntity->GetId())(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetCurrentEnvironments(m_pEntity->GetId()));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetAuxAudioProxyOffset(SATLWorldPosition const& rOffset, TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			SSetAuxAudioProxyOffset(rOffset, m_pEntity->GetWorldTM())(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SSetAuxAudioProxyOffset(rOffset, m_pEntity->GetWorldTM()));
	}
}

//////////////////////////////////////////////////////////////////////////
SATLWorldPosition const& CEntityAudioProxy::GetAuxAudioProxyOffset(TAudioProxyID const nAudioProxyLocalID /* = DEFAULT_AUDIO_PROXY_ID */)
{
	TAuxAudioProxies::const_iterator const Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

	if (Iter != m_mapAuxAudioProxies.end())
	{
		return Iter->second.oOffset;
	}

	return s_oNULLOffset;
}

//////////////////////////////////////////////////////////////////////////
TAudioProxyID CEntityAudioProxy::CreateAuxAudioProxy()
{
	TAudioProxyID nAudioProxyLocalID = INVALID_AUDIO_PROXY_ID;
	IAudioProxy* const pIAudioProxy = gEnv->pAudioSystem->GetFreeAudioProxy();

	if (pIAudioProxy != NULL)
	{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
		if (m_nAudioProxyIDCounter == std::numeric_limits<TAudioProxyID>::max())
		{
			CryFatalError("<Audio> Exceeded numerical limits during CEntityAudioProxy::CreateAudioProxy!");
		}
		else if (m_pEntity == NULL)
		{
			CryFatalError("<Audio> NULL entity pointer during CEntityAudioProxy::CreateAudioProxy!");
		}

		CryFixedStringT<MAX_AUDIO_OBJECT_NAME_LENGTH> sFinalName(m_pEntity->GetName());
		size_t const nNumAuxAudioProxies = m_mapAuxAudioProxies.size();

		if (nNumAuxAudioProxies > 0)
		{
			// First AuxAudioProxy is not explicitly identified, it keeps the entity's name.
			// All additionally AuxaudioProxies however are being explicitly identified.
			sFinalName.Format("%s_auxaudioproxy_#%" PRISIZE_T, m_pEntity->GetName(), nNumAuxAudioProxies + 1);
		}

		pIAudioProxy->Initialize(sFinalName.c_str());
#else
		pIAudioProxy->Initialize(NULL);
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

		pIAudioProxy->SetPosition(m_pEntity->GetWorldPos());
		pIAudioProxy->SetObstructionCalcType(eAOOCT_IGNORE);
		pIAudioProxy->SetCurrentEnvironments(m_pEntity->GetId());

		m_mapAuxAudioProxies.insert(TAudioProxyPair(++m_nAudioProxyIDCounter, SAudioProxyWrapper(pIAudioProxy)));
		nAudioProxyLocalID = m_nAudioProxyIDCounter;
	}

	return nAudioProxyLocalID;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityAudioProxy::RemoveAuxAudioProxy(TAudioProxyID const nAudioProxyLocalID)
{
	bool bSuccess = false;

	if (nAudioProxyLocalID != DEFAULT_AUDIO_PROXY_ID)
	{
		TAuxAudioProxies::const_iterator const Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

		if (Iter != m_mapAuxAudioProxies.end())
		{
			m_mapAuxAudioProxies.erase(Iter);
			bSuccess = true;
		}
		else
		{
			gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> AudioProxy not found during CEntityAudioProxy::RemoveAudioProxy (%s)!", m_pEntity->GetEntityTextDescription());
			assert(false);
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to remove the default AudioProxy during CEntityAudioProxy::RemoveAudioProxy (%s)!", m_pEntity->GetEntityTextDescription());
		assert(false);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
CEntityAudioProxy::TAudioProxyPair& CEntityAudioProxy::GetAuxAudioProxyPair(TAudioProxyID const nAudioProxyLocalID)
{
	TAuxAudioProxies::iterator const Iter(m_mapAuxAudioProxies.find(nAudioProxyLocalID));

	if (Iter != m_mapAuxAudioProxies.end())
	{
		return *Iter;
	}

	return s_oNULLAudioProxyPair;
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::ExecuteTriggerInternal(TAudioControlID const nTriggerID, ELipSyncMethod const eLipSyncMethod, TTriggerFinishedCallback const pCallback, void* const pCallbackCookie, TAudioProxyID const nAudioProxyLocalID)
{
	if (m_pEntity != NULL)
	{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
		Matrix34 const& tm = m_pEntity->GetWorldTM();

		if (tm.GetTranslation() == Vec3Constants<float>::fVec3_Zero)
		{
			gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger at (0,0,0) position in the entity %s. Entity may not be initialized correctly!", m_pEntity->GetEntityTextDescription());
		}
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

		if (!m_bHide || (m_pEntity->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN) != 0)
		{
			REINST("lip sync on EntityAudioProxy in executetrigger")
			//if (lipSyncMethod != eLSM_None)
			//{
			//	// If voice is playing inform provider (if present) about it to apply lip-sync.
			//	if (m_pLipSyncProvider)
			//	{
			//		m_currentLipSyncId = pSound->GetId();
			//		m_currentLipSyncMethod = lipSyncMethod;
			//		m_pLipSyncProvider->RequestLipSync(this, m_currentLipSyncId, m_currentLipSyncMethod);
			//	}
			//}
		}
		else
		{
			gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger on %s which is hidden!", m_pEntity->GetEntityTextDescription());
		}
	}
	else
	{
		gEnv->pSystem->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "<Audio> Trying to execute an audio trigger on a sound proxy without a valid entity!");
	}

	if (nAudioProxyLocalID != INVALID_AUDIO_PROXY_ID)
	{
		TAudioProxyPair const& rAudioProxyPair = GetAuxAudioProxyPair(nAudioProxyLocalID);

		if (rAudioProxyPair.first != INVALID_AUDIO_PROXY_ID)
		{
			(SExecuteTrigger(nTriggerID, eLipSyncMethod, pCallback, pCallbackCookie))(rAudioProxyPair);
		}
	}
	else
	{
		std::for_each(m_mapAuxAudioProxies.begin(), m_mapAuxAudioProxies.end(), SExecuteTrigger(nTriggerID, eLipSyncMethod, pCallback, pCallbackCookie));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::SetEnvironmentAmountInternal(IEntity const* const pEntity, float const fAmount) const
{
	IEntityAudioProxy* const pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));

	if ((pIEntityAudioProxy != NULL) && (m_nAudioEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID))
	{
		// Only set the audio-environment-amount on the entities that already have an AudioProxy.
		// Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pEntityAudioProxy.
		pIEntityAudioProxy->SetEnvironmentAmount(m_nAudioEnvironmentID, fAmount, INVALID_AUDIO_PROXY_ID);
	}
}

///////////////////////////////////////////////////////////////////////////
void CEntityAudioProxy::Done()
{
	m_pEntity = NULL;
}

#include UNIQUE_VIRTUAL_WRAPPER(IEntityAudioProxy)
