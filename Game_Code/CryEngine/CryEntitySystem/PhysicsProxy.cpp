////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PhysicsProxy.cpp
//  Version:     v1.00
//  Created:     25/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PhysicsProxy.h"
#include "Entity.h"
#include "EntityObject.h"
#include "EntitySystem.h"
#include "ISerialize.h"
#include "AffineParts.h"
#include "CryHeaders.h"

#include "TimeValue.h"
#include <ICryAnimation.h>
#include <IIndexedMesh.h>
#include <INetwork.h>
#include <IGeomCache.h>

#include <ICodeCheckpointMgr.h>

#define MAX_MASS_TO_RESTORE_VELOCITY 500

#define PHYS_ENTITY_DISABLE 1
#define PHYS_ENTITY_ENABLE  2

#define PHYS_DEFAULT_DENSITY 1000.0f

#if !defined(_RELEASE)
namespace
{
  static int ValidateSetParams(const EventPhys *pEvent)
  {
		const EventPhysStateChange* event = static_cast<const EventPhysStateChange*>(pEvent);
		if ((event->iSimClass[1] == SC_SLEEPING_RIGID || event->iSimClass[1] == SC_ACTIVE_RIGID) && event->pEntity->GetType() == PE_ARTICULATED)
    {
			pe_status_dynamics dynamics;
			pe_params_articulated_body articulated_body;
			if( event->pEntity->GetParams(&articulated_body) && event->pEntity->GetStatus(&dynamics) && articulated_body.pHost && dynamics.mass!=0.f )
      {
				CryWarning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_ERROR_DBGBRK, "articulated entity simulated as rigid while still attached to a host");
      }
		}
    return 1; 
  }
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::EnableValidation()
{
  if (IPhysicalWorld* pPhysWorld = gEnv->pPhysicalWorld)
  {
    pPhysWorld->AddEventClient( EventPhysStateChange::id, ValidateSetParams, 0, 1e14f);
  }
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::DisableValidation()
{
  if (IPhysicalWorld* pPhysWorld = gEnv->pPhysicalWorld)
  {
    pPhysWorld->RemoveEventClient( EventPhysStateChange::id, ValidateSetParams, 0 );
  }
}
#endif 


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
CPhysicalProxy::CPhysicalProxy()
	: m_pEntity(NULL)
	, m_pPhysicalEntity(NULL)
	, m_pColliders()
	, m_partId0(0)
	, m_nFlags(0)
	, m_timeLastSync(-100)
{
}

void CPhysicalProxy::Initialize( const SComponentInitializer& init )
{
	m_pEntity = (CEntity*)init.m_pEntity;
}

void CPhysicalProxy::Reload( IEntity *pEntity,SEntitySpawnParams &params )
{
	assert(pEntity == m_pEntity);

	m_partId0 = 0;
	m_nFlags = 0;
	m_timeLastSync = -100;

	ReleasePhysicalEntity();
	ReleaseColliders();
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::ReleasePhysicalEntity()
{
	// Delete physical entity from physical world.
	if (m_pPhysicalEntity)
	{
		m_pPhysicalEntity->Release();
		PhysicalWorld()->DestroyPhysicalEntity(m_pPhysicalEntity);
		m_pPhysicalEntity = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::ReleaseColliders()
{
	// Delete physical box entity from physical world.
	if (m_pColliders && m_pColliders->m_pPhysicalBBox)
	{
		PhysicalWorld()->DestroyPhysicalEntity(m_pColliders->m_pPhysicalBBox);
		delete  m_pColliders;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::Release()
{
	ReleasePhysicalEntity();
	ReleaseColliders();
	
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::Done()
{
	// Disabling the physics on 'done' prevents any outstanding physics CBs being executed.
	SetFlags( FLAG_PHYSICS_DISABLED );
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::Update( SEntityUpdateContext &ctx )
{
	if (m_pColliders)
		CheckColliders();
}

//////////////////////////////////////////////////////////////////////////

// Build a bit flag of used PARTID_LINKED ranges
uint64 GetUsedLinkedRanges(CEntity *pEntity, uint64 used)
{
	if (CPhysicalProxy* pProxy = pEntity->GetPhysicalProxy())
	{
		int range = pProxy->GetPartId0()/PARTID_LINKED;
		assert(range<PARTID_MAX_NUM_ATTACHMENTS);
		used |= 1ULL<<range;
		for(int i = pEntity->GetChildCount()-1; i>=0; i--)
			used |= GetUsedLinkedRanges((CEntity*)pEntity->GetChild(i),used);
	}
	return used;
}

#ifdef DEBUG_ENTITY_LINKING
void DumpPhysicsIds(IPhysicalEntity* pent)
{
	pe_status_nparts snp;
	int partcount = pent->GetStatus(&snp);
	for (int i=0; i<partcount; i++)
	{
		pe_params_part ppp;
		ppp.ipart = i;
		pent->GetParams(&ppp);
		CryLogAlways("##    partid: %d, (linked: %d)", ppp.partid, ppp.partid/PARTID_LINKED);
	}
}
#endif


int AllocFreePartIdRange(uint64& usedRanges)
{
	int offset = 0; uint64 mask = 0xff;
	while ((mask&usedRanges)==mask && offset<PARTID_MAX_NUM_ATTACHMENTS) {offset+=8; mask<<=8;}
	while ((1ULL<<(++offset) & usedRanges) && offset<PARTID_MAX_NUM_ATTACHMENTS);
	if (offset<PARTID_MAX_NUM_ATTACHMENTS)
	{
		usedRanges |= 1ULL<<offset;
		return offset;
	}
	return -1;
}

CEntity* GetAdam(CEntity* pAdam, Matrix34& mtx)
{ 
	CEntity *pParent;
	mtx.SetIdentity();
	while(pParent=(CEntity*)pAdam->GetParent())
	{
		mtx = Matrix34::CreateScale(pParent->GetScale()) * pAdam->GetLocalTM() * mtx;
		pAdam = pParent;
	}
	return pAdam;
}

void CPhysicalProxy::MoveChildPhysicsParts(IPhysicalEntity* pSrcAdam, CEntity* pChild, pe_action_move_parts& amp, uint64 usedRanges)
{
	CPhysicalProxy* pProxy = pChild->GetPhysicalProxy();
	if (pProxy && pProxy->m_pPhysicalEntity && pProxy->m_pPhysicalEntity->GetType()<=PE_RIGID)
	{
		int offset = AllocFreePartIdRange(usedRanges);
		if (offset>=0)
		{
			// Move from pSrcAdam to pDstAdam, find an available PARTID_LINKED range
			amp.idOffset = offset * PARTID_LINKED - pProxy->GetPartId0();
			amp.idStart = pProxy->GetPartId0();
			amp.idEnd = amp.idStart+PARTID_LINKED-1;
#ifdef DEBUG_ENTITY_LINKING
			CryLogAlways("## moving ids: %d to parent, will have new offset: %d", amp.idStart, amp.idEnd, offset);
#endif
			pSrcAdam->Action(&amp);
			pProxy->SetPartId0(offset * PARTID_LINKED);
		}
		else
		{
			IEntity* pAdamEntity = gEnv->pEntitySystem->GetEntityFromPhysics(amp.pTarget);
			gEnv->pSystem->Warning( VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, NULL,
				"Failed to attach %s to %s", pChild->GetName(), pAdamEntity?pAdamEntity->GetName() : "");
			return;
		}
	}
	const int n = pChild->GetChildCount();
	for(int i=0; i<n; i++)
		MoveChildPhysicsParts(pSrcAdam, (CEntity*)pChild->GetChild(i), amp, usedRanges);
}


//int GetMaxPartId0(CEntity *pent)
//{
//	int i,maxPartId0 = pent->GetPhysicalProxy() ? pent->GetPhysicalProxy()->GetPartId0():0;
//	for(i=pent->GetChildCount()-1;i>=0;i--)
//		maxPartId0 = max(maxPartId0,GetMaxPartId0((CEntity*)pent->GetChild(i)));
//	return maxPartId0;
//}

bool HasPartId(CEntity *pent,int partid)
{
	CPhysicalProxy* pProxy = pent->GetPhysicalProxy();
	if (pProxy && pProxy->GetPartId0()>=PARTID_LINKED && pProxy->GetPartId0()<PARTID_CGA && 
			(unsigned int)(partid-pProxy->GetPartId0())<(PARTID_LINKED-1))
		return true;
	//for(int i=pent->GetChildCount()-1;i>=0;i--)	if (HasPartId((CEntity*)pent->GetChild(i),partid))
	//	return true;
	return false;
}

void ReclaimPhysParts(CEntity *pentChild, IPhysicalEntity *pentSrc, pe_action_move_parts &amp)
{
	if (CPhysicalProxy* pProxy = pentChild->GetPhysicalProxy())
	{
		if ((amp.idStart=pProxy->GetPartId0())>0) 
		{
			amp.idEnd = amp.idStart+PARTID_LINKED-1;
#ifdef DEBUG_ENTITY_LINKING
			CryLogAlways("## reclaiming range %d-%d", amp.idStart, amp.idEnd);
#endif
			pentSrc->Action(&amp);
		}
#ifdef DEBUG_ENTITY_LINKING
		CryLogAlways("## SetPartId0(%d)", pProxy->GetPartId0() + amp.idOffset);
#endif
		pProxy->SetPartId0(pProxy->GetPartId0() + amp.idOffset);
	}
	for(int i=pentChild->GetChildCount()-1;i>=0;i--)
	{
		ReclaimPhysParts((CEntity*)pentChild->GetChild(i), pentSrc,amp);
	}
}

void CPhysicalProxy::ProcessEvent( SEntityEvent &event )
{
	switch (event.event)
	{
	case ENTITY_EVENT_XFORM:
		OnEntityXForm(event);
		if (m_pPhysicalEntity && m_pPhysicalEntity->GetType()<=PE_RIGID && GetPartId0()<PARTID_CGA && GetPartId0()>=PARTID_LINKED)
		{
			CEntity *pAdam;
			CPhysicalProxy *pAdamProxy;
			pe_status_pos sp;	sp.flags=status_local;
			pe_params_bbox pbb;
			Matrix34 mtxPart,mtxLoc;
			pe_params_part pp; pp.pMtx3x4 = &mtxPart;
			AABB bbox; bbox.Reset();
			if ((pAdam=(CEntity*)GetAdam(m_pEntity, mtxLoc))!=m_pEntity && (pAdamProxy=pAdam->GetPhysicalProxy()) && 
					pAdamProxy->m_pPhysicalEntity && pAdamProxy->m_pPhysicalEntity->GetType()<=PE_WHEELEDVEHICLE)
			{
				pe_status_nparts snp;
				for(sp.ipart=m_pPhysicalEntity->GetStatus(&snp)-1; sp.ipart>=0; sp.ipart--)
				{
					m_pPhysicalEntity->GetStatus(&sp);
					bbox.Add(sp.BBox[0]+sp.pos),bbox.Add(sp.BBox[1]+sp.pos);
					if (!CheckFlags(FLAG_IGNORE_XFORM_EVENT) && !pAdamProxy->CheckFlags(FLAG_IGNORE_XFORM_EVENT))
					{
						pp.partid = sp.partid+GetPartId0();
						mtxPart = mtxLoc*m_pEntity->GetSlotLocalTM(sp.partid,true);
						pAdamProxy->m_pPhysicalEntity->SetParams(&pp);
					}
				}
				for(sp.ipart=pAdamProxy->m_pPhysicalEntity->GetStatus(&snp)-1; sp.ipart>=0; sp.ipart--)
				{
					pAdamProxy->m_pPhysicalEntity->GetStatus(&sp);
					if (HasPartId(m_pEntity,sp.partid))
					{
						bbox.Add(sp.BBox[0]+sp.pos),bbox.Add(sp.BBox[1]+sp.pos);
						if (!CheckFlags(FLAG_IGNORE_XFORM_EVENT) && !pAdamProxy->CheckFlags(FLAG_IGNORE_XFORM_EVENT))
						{
							pp.partid = sp.partid;
							mtxPart = mtxLoc*m_pEntity->GetSlotLocalTM(sp.partid-GetPartId0(),true);
							pAdamProxy->m_pPhysicalEntity->SetParams(&pp);
						}
					}
				}
				if (bbox.max.x>bbox.min.x) 
				{
					pbb.BBox[0] = bbox.min; pbb.BBox[1] = bbox.max;
					m_pPhysicalEntity->SetParams(&pbb);
				}
			}
		} break;
	case ENTITY_EVENT_HIDE:
	case ENTITY_EVENT_INVISIBLE:
		if (!gEnv->IsEditor() && m_pPhysicalEntity && m_pPhysicalEntity->GetType() == PE_SOFT) 
		{
			ReleasePhysicalEntity();
			SetFlags( GetFlags() | (FLAG_PHYSICS_REMOVED) );
		}
		else
			EnablePhysics(false);
		break;
	case ENTITY_EVENT_UNHIDE:
		{
			// Force xform update to move physics proxy. Physics ignores updates while hidden.
			SEntityEvent event(ENTITY_EVENT_XFORM);
			event.nParam[0] = ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL;		
			OnEntityXForm(event);
		}
	case ENTITY_EVENT_VISIBLE:
		if (!gEnv->IsEditor() && !m_pPhysicalEntity && (GetFlags() & (FLAG_PHYSICS_REMOVED))) 
		{
			SEntityEvent e; 
			e.event = ENTITY_EVENT_RESET; 
			GetEntity()->GetScriptProxy()->ProcessEvent(e);
			SetFlags( GetFlags() & (~FLAG_PHYSICS_REMOVED) );
		}
		else
			EnablePhysics(true);
		break;
	case ENTITY_EVENT_DONE:
		{
			SEntityPhysicalizeParams epp; epp.type=PE_NONE;
			Physicalize(epp);
			break;
		}
	case ENTITY_EVENT_ATTACH:	
		{
			CEntity *pChild,*pAdam;
			CPhysicalProxy *pChildProxy,*pAdamProxy;
			pe_action_move_parts amp;
			pChild = (CEntity*)m_pEntity->GetEntitySystem()->GetEntity((EntityId)event.nParam[0]);
			pAdam = (CEntity*)GetAdam(pChild, amp.mtxRel);
			if (pChild)
			{
				int i;
				for(i=pChild->GetSlotCount()-1; i>=0 && (!pChild->GetCharacter(i) || pChild->GetCharacter(i)->GetOjectType()!=CGA); i--);

				if (i<0 &&
						(pChildProxy=pChild->GetPhysicalProxy()) && pChildProxy->m_pPhysicalEntity && pChildProxy->m_pPhysicalEntity->GetType()<=PE_RIGID && 
						pAdam && (pAdamProxy=pAdam->GetPhysicalProxy()) && pAdamProxy->m_pPhysicalEntity && 
						(i=pAdamProxy->m_pPhysicalEntity->GetType())<=PE_WHEELEDVEHICLE && i!=PE_STATIC)
				{

#ifdef DEBUG_ENTITY_LINKING
					{
						CryLogAlways("## Gonna attach %s to parent %s", pChild->GetName(), pAdam->GetName());
						CryLogAlways("## Child:");
						DumpPhysicsIds(pChildProxy->m_pPhysicalEntity);
						CryLogAlways("## parent:");
						DumpPhysicsIds(pAdamProxy->m_pPhysicalEntity);
					}
#endif

					// Search 'adam' for available PARTID_LINKED ranges that can be used
					uint64 usedRange = GetUsedLinkedRanges(pAdam,0);

					// Move pChild (and all of its children) to pAdam
					amp.pTarget = pAdamProxy->m_pPhysicalEntity;
					MoveChildPhysicsParts(pChildProxy->m_pPhysicalEntity, pChild, amp, usedRange);
					
#ifdef DEBUG_ENTITY_LINKING
					{
						CryLogAlways("## After attaching to parent %s", pAdam->GetName());
						DumpPhysicsIds(pAdamProxy->m_pPhysicalEntity);
					}
#endif
				}
			}
		} break;
	case ENTITY_EVENT_DETACH:
		{
			CEntity *pChild,*pAdam;
			CPhysicalProxy *pChildProxy,*pAdamProxy;
			pe_action_move_parts amp;
			pChild = (CEntity*)m_pEntity->GetEntitySystem()->GetEntity((EntityId)event.nParam[0]);
			pAdam = (CEntity*)m_pEntity->GetAdam();
			if (pChild)
			{
				int i;
				for(i=pChild->GetSlotCount()-1; i>=0 && (!pChild->GetCharacter(i) || pChild->GetCharacter(i)->GetOjectType()!=CGA); i--);

				if (i<0 && 
						(pChildProxy=pChild->GetPhysicalProxy()) && pChildProxy->m_pPhysicalEntity && pChildProxy->m_pPhysicalEntity->GetType()<=PE_RIGID && 
						pAdam && (pAdamProxy=pAdam->GetPhysicalProxy()) && pAdamProxy->m_pPhysicalEntity && 
						(i=pAdamProxy->m_pPhysicalEntity->GetType())<=PE_WHEELEDVEHICLE && i!=PE_STATIC &&  
						pChildProxy->GetPartId0()>=PARTID_LINKED)
				{
#ifdef DEBUG_ENTITY_LINKING
					{
						CryLogAlways("## Gonna DETACH %s from parent %s", pChild->GetName(), pAdam->GetName());
						DumpPhysicsIds(pAdamProxy->m_pPhysicalEntity);
					}
#endif
					Matrix34 childWorldTM = pChild->GetWorldTM();
					childWorldTM.OrthonormalizeFast();
					amp.mtxRel = childWorldTM.GetInverted()*pAdam->GetWorldTM();
					amp.pTarget = pChildProxy->m_pPhysicalEntity;
					amp.idOffset = -pChildProxy->GetPartId0();
					ReclaimPhysParts(pChild, pAdamProxy->m_pPhysicalEntity, amp);
					
#ifdef DEBUG_ENTITY_LINKING
					{
						CryLogAlways("## sfter ReclaimPhysParts child %s and parent %s", pChild->GetName(), pAdam->GetName());
						CryLogAlways("## Child:");
						DumpPhysicsIds(pChildProxy->m_pPhysicalEntity);
						CryLogAlways("## parent:");
						DumpPhysicsIds(pAdamProxy->m_pPhysicalEntity);
					}
#endif
				}
			}
		} break;
	case ENTITY_EVENT_COLLISION:
		if (event.nParam[1]==1)
		{
			EventPhysCollision *pColl = (EventPhysCollision*)event.nParam[0];
			int islot = pColl->partid[1];
			pe_params_bbox pbb;
			Vec3 sz; 
			float r,strength;
			Matrix34 mtx;
			IStatObj *pNewObj;
			CEntityObject *pSlot = m_pEntity->GetSlot(islot);
			if (!(pSlot=m_pEntity->GetSlot(islot)) || !pSlot->pStatObj)
				pSlot=m_pEntity->GetSlot(islot=0);
			pColl->pEntity[0]->GetParams(&pbb);
			sz = pbb.BBox[1]-pbb.BBox[0];
			r = min(0.5f,max(0.05f, (sz.x+sz.y+sz.z)*(1.0f/3)));
			strength = min(r, pColl->vloc[0].len()*(1.0f/50));
			mtx = m_pEntity->GetSlotWorldTM(islot).GetInverted();

			if (pSlot && pSlot->pStatObj)
			{
				if ((pNewObj=pSlot->pStatObj->DeformMorph(mtx*pColl->pt,r,strength))!=pSlot->pStatObj)
					m_pEntity->SetStatObj(pNewObj, islot,true);
			}
		}
		break;
	case ENTITY_EVENT_PHYS_BREAK:
		{
			EventPhysJointBroken *pepjb = (EventPhysJointBroken*)event.nParam[0];

			// Counter for feature test setup
			CODECHECKPOINT(physics_on_tree_joint_break);


			if (pepjb && !pepjb->bJoint && pepjb->idJoint==1000000)
			{	// special handling for splinters on breakable trees
				CEntity *pent,*pentNew;
				IStatObj *pStatObj;
				int i,j,iop;
				float dist,mindist;
				for(iop=0; iop<2; iop++) if (!is_unused(pepjb->pNewEntity[iop]) && (pent=(CEntity*)pepjb->pEntity[iop]->GetForeignData(PHYS_FOREIGN_ID_ENTITY)))
				{
					for(i=pent->GetSlotCount()-1,j=-1,mindist=1E10f; i>0; i--)
						if ((pStatObj=pent->GetStatObj(i|ENTITY_SLOT_ACTUAL)) && !(pStatObj->GetFlags() & STATIC_OBJECT_GENERATED) && 
								!pStatObj->GetPhysGeom() && (dist=(pent->GetSlotWorldTM(i).GetTranslation()-pepjb->pt).len2())<mindist)
							mindist=dist, j=i;
					if (j>=0)
					{
						pStatObj = pent->GetStatObj(j|ENTITY_SLOT_ACTUAL);
						if (pepjb->pNewEntity[iop] && (pentNew=(CEntity*)pepjb->pNewEntity[iop]->GetForeignData(PHYS_FOREIGN_ID_ENTITY)))
							pentNew->SetSlotLocalTM(pentNew->SetStatObj(pStatObj,-1,false), pent->GetSlotLocalTM(j|ENTITY_SLOT_ACTUAL,false));
						else 
						{
							IStatObj::SSubObject *pSubObj = pStatObj->FindSubObject("splinters_cloud");
							if (pSubObj) 
							{
								IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(pSubObj->properties);
								if (pEffect)
									pEffect->Spawn(true, IParticleEffect::ParticleLoc(pepjb->pt));
							}
						}
						pent->FreeSlot(j);
					}
				}
			}
		} break;
	case ENTITY_EVENT_RENDER:
		if (m_nFlags & FLAG_PHYS_AWAKE_WHEN_VISIBLE) 
		{
			IRenderNode *pRndNode = m_pEntity->GetRenderProxy();
			pe_action_awake aa;
			if (m_pPhysicalEntity && pRndNode && 
					(pRndNode->GetBBox().GetCenter()-GetISystem()->GetViewCamera().GetPosition()).len2()<sqr(CVar::es_MaxPhysDist))
			{
				m_pPhysicalEntity->Action(&aa);
				m_pEntity->SetFlags(m_pEntity->GetFlags() &~ ENTITY_FLAG_SEND_RENDER_EVENT);
				m_nFlags &= ~FLAG_PHYS_AWAKE_WHEN_VISIBLE;
			}
		} else if (m_nFlags & FLAG_ATTACH_CLOTH_WHEN_VISIBLE && m_pPhysicalEntity)
			for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
			{
				IRenderMesh *pRM;
				if (!m_pEntity->GetSlot(slot) || !m_pEntity->GetSlot(slot)->pStatObj || !(pRM=m_pEntity->GetSlot(slot)->pStatObj->GetRenderMesh()))
					continue;
				pe_params_foreign_data pfd;
				m_pPhysicalEntity->GetParams(&pfd);
				if (pfd.iForeignData==PHYS_FOREIGN_ID_USER)
				{
					IPhysicalEntity *pAttachTo = (IPhysicalEntity*)pfd.pForeignData;
					if (pAttachTo)
					{
						pe_status_pos sp; 
						pAttachTo->GetStatus(&sp);
						pAttachTo->Release();
						if (1<<sp.iSimClass > ent_independent)
							pAttachTo = 0;
					}
					AttachSoftVtx(pRM, pAttachTo,pfd.iForeignFlags);
				}
				pfd.pForeignData = m_pEntity;
				pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
				pfd.iForeignFlags = 0;
				m_pPhysicalEntity->SetParams( &pfd );
				m_pEntity->SetFlags(m_pEntity->GetFlags() &~ ENTITY_FLAG_SEND_RENDER_EVENT);
				m_nFlags &= ~FLAG_ATTACH_CLOTH_WHEN_VISIBLE;
				break;
			}
		break; 
	case ENTITY_EVENT_MATERIAL:
		if (IMaterial *pMtl = (IMaterial*)event.nParam[0]) 
		{
			pe_params_part ppart;
			int surfaceTypesId[MAX_SUB_MATERIALS];
			ppart.nMats = pMtl->FillSurfaceTypeIds(ppart.pMatMapping=surfaceTypesId);
			IEntity *pAdam = m_pEntity->GetAdam();
			if (m_partId0>=PARTID_LINKED && pAdam && pAdam!=m_pEntity && pAdam->GetPhysics())	{
				ppart.partid = m_partId0;	pAdam->GetPhysics()->SetParams(&ppart);
			}	else if (m_pPhysicalEntity)
				m_pPhysicalEntity->SetParams(&ppart);
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::NeedSerialize()
{
	if (m_pPhysicalEntity)
	{
		return true;

	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::GetSignature( TSerialize signature )
{
	signature.BeginGroup("PhysicalProxy");
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::Serialize( TSerialize ser )
{
	if (ser.BeginOptionalGroup("PhysicsProxy",(NeedSerialize()||ser.GetSerializationTarget()==eST_Network) ))
	{
		if (m_pPhysicalEntity)
		{
			if (ser.IsReading())
			{
				m_pPhysicalEntity->SetStateFromSnapshot( ser, 0 );
			}
			else
			{
				m_pPhysicalEntity->GetStateSnapshot( ser );
			}
		}

		if (ser.GetSerializationTarget()!=eST_Network) // no folieage over network for now.
		{
			CEntityObject *pSlot = m_pEntity->GetSlot(0);
			if (pSlot && pSlot->pStatObj && pSlot->pFoliage)
				pSlot->pFoliage->Serialize(ser);

			IPhysicalEntity *pCharPhys;
			// character physics is equal to the physproxy's physics only for ragdolls
			bool bSerializableRopes = 
				pSlot && pSlot->pCharacter && (!(pCharPhys=pSlot->pCharacter->GetISkeletonPose()->GetCharacterPhysics()) || pCharPhys==m_pPhysicalEntity);
			if (bSerializableRopes && ser.BeginOptionalGroup("SerializableRopes", bSerializableRopes))
			{
				for(int i=0;pSlot->pCharacter->GetISkeletonPose()->GetCharacterPhysics(i);i++)
				{
					if (ser.BeginOptionalGroup("CharRope",true))
					{
						if (ser.IsReading())
							pSlot->pCharacter->GetISkeletonPose()->GetCharacterPhysics(i)->SetStateFromSnapshot(ser);
						else
							pSlot->pCharacter->GetISkeletonPose()->GetCharacterPhysics(i)->GetStateSnapshot(ser);
						ser.EndGroup();
					}
				}
				ser.EndGroup();
			}
			if (bSerializableRopes && ser.IsReading())
				m_pEntity->ActivateForNumUpdates(2);
		}

		ser.EndGroup(); //PhysicsProxy
	}
}

void CPhysicalProxy::SerializeTyped( TSerialize ser, int type, int flags )
{
	NET_PROFILE_SCOPE("PhysicalProxy", ser.IsReading());
	ser.BeginGroup("PhysicsProxy");
	if (m_pPhysicalEntity)
	{
		if (ser.IsReading())
		{
			m_pPhysicalEntity->SetStateFromTypedSnapshot( ser, type, flags );
		}
		else
		{
			m_pPhysicalEntity->GetStateSnapshot( ser );
		}
	}
	ser.EndGroup();
}

void CPhysicalProxy::SerializeXML( XmlNodeRef &entityNode,bool bLoading )
{
	XmlNodeRef physicsState = entityNode->findChild("PhysicsState");
	if(physicsState)
		m_pEntity->SetPhysicsState(physicsState);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::OnEntityXForm( SEntityEvent &event )
{
	if (!CheckFlags(FLAG_IGNORE_XFORM_EVENT))
	{
		// This set flag prevents endless recursion of events recieved from physics.
		SetFlags( m_nFlags|FLAG_IGNORE_XFORM_EVENT );
		if (m_pPhysicalEntity)
		{
			pe_params_pos ppos;

			bool bAnySet = false;
			if (event.nParam[0] & ENTITY_XFORM_POS) // Position changes.
			{
				ppos.pos = m_pEntity->GetWorldTM().GetTranslation();
				bAnySet = true;
			}
			if (event.nParam[0] & ENTITY_XFORM_ROT) // Rotation changes.
			{
				bAnySet = true;
				if (!m_pEntity->GetParent())
					ppos.q = m_pEntity->GetRotation();
				else
					ppos.pMtx3x4 = const_cast<Matrix34*>(&m_pEntity->GetWorldTM());
			}
			if (event.nParam[0] & ENTITY_XFORM_SCL) // Scale changes.
			{
				bAnySet = true;
				Vec3 s = m_pEntity->GetScale();
				if (!m_pEntity->GetParent() && sqr(s.x-s.y)+sqr(s.y-s.z)==0.0f)
					ppos.scale = s.x;
				else
					ppos.pMtx3x4 = const_cast<Matrix34*>(&m_pEntity->GetWorldTM());
			}
			if (!bAnySet)
				ppos.pMtx3x4 = const_cast<Matrix34*>(&m_pEntity->GetWorldTM());

#ifdef SEG_WORLD
			if(event.nParam[1])
				ppos.bRecalcBounds |= 2;
#endif

			m_pPhysicalEntity->SetParams(&ppos);
		}
		if (m_pColliders)
		{
			// Not affected by rotation or scale, just recalculate position.
			AABB bbox;
			GetTriggerBounds(bbox);
			SetTriggerBounds(bbox);
		}
		SetFlags( m_nFlags&(~FLAG_IGNORE_XFORM_EVENT) );
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::GetLocalBounds( AABB &bbox )
{
	bbox.min.Set(0,0,0);
	bbox.max.Set(0,0,0);

	pe_status_pos pstate;
	if (m_pPhysicalEntity && m_pPhysicalEntity->GetStatus(&pstate))
	{
		bbox.min = pstate.BBox[0] / pstate.scale;
		bbox.max = pstate.BBox[1] / pstate.scale;
	}
	else if (m_pColliders)
	{
		GetTriggerBounds(bbox);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::GetWorldBounds( AABB &bbox )
{
	pe_params_bbox pbb;
	if (m_pPhysicalEntity && m_pPhysicalEntity->GetParams(&pbb))
	{
		bbox.min = pbb.BBox[0];
		bbox.max = pbb.BBox[1];
	}
	else if (m_pColliders && m_pColliders->m_pPhysicalBBox)
	{
		if (m_pColliders->m_pPhysicalBBox->GetParams(&pbb))
		{
			bbox.min = pbb.BBox[0];
			bbox.max = pbb.BBox[1];
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::EnablePhysics( bool bEnable )
{
	if (bEnable == !CheckFlags(FLAG_PHYSICS_DISABLED))
		return;

	if (bEnable)
	{
		SetFlags( GetFlags() & (~FLAG_PHYSICS_DISABLED) );
	}
	else 
	{
		SetFlags( GetFlags()|FLAG_PHYSICS_DISABLED );
	}

	if (!m_pPhysicalEntity)
		return;

	CEntity *pAdam = (CEntity*)m_pEntity->GetAdam();
	IPhysicalEntity *pPhysParent;
	if (m_partId0>=PARTID_LINKED && pAdam && pAdam!=m_pEntity && (pPhysParent=pAdam->GetPhysics()))
	{
		pe_params_part pp;
		if (bEnable) 
			pp.flagsOR=geom_collides|geom_floats, pp.flagsColliderOR=geom_colltype0;
		else
			pp.flagsAND=pp.flagsColliderAND=0, pp.mass=0;
		for (pp.partid=m_partId0; pp.partid<m_partId0+m_pEntity->GetSlotCount(); pp.partid++)
			pPhysParent->SetParams(&pp);
	}	
	if (bEnable)
	{
		PhysicalWorld()->DestroyPhysicalEntity(m_pPhysicalEntity, PHYS_ENTITY_ENABLE);
	}
	else 
	{
		PhysicalWorld()->DestroyPhysicalEntity(m_pPhysicalEntity, PHYS_ENTITY_DISABLE);
	}

	// Enable/Disable character physics characters.
	if (m_nFlags & FLAG_PHYS_CHARACTER)
	{
		int nSlot = (m_nFlags & CHARACTER_SLOT_MASK);
		ICharacterInstance *pCharacter = m_pEntity->GetCharacter(nSlot);
		IPhysicalEntity *pCharPhys;
		if (pCharacter)
		{
			pCharacter->GetISkeletonPose()->DestroyCharacterPhysics( (bEnable)?PHYS_ENTITY_ENABLE:PHYS_ENTITY_DISABLE );
			if (bEnable && (pCharPhys=pCharacter->GetISkeletonPose()->GetCharacterPhysics()) && pCharPhys!=m_pPhysicalEntity)
			{
				pe_params_articulated_body pab;
				pab.pHost = m_pPhysicalEntity;
				pCharPhys->SetParams(&pab);
			}
		}
	}

	OnChangedPhysics(bEnable);
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::IsPhysicsEnabled() const
{
	return !CheckFlags(FLAG_PHYSICS_DISABLED);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::Physicalize( SEntityPhysicalizeParams &params )
{
	ENTITY_PROFILER

	SEntityEvent eed(ENTITY_EVENT_DETACH);
	int i;
	for(i=0; i<m_pEntity->GetChildCount() ;i++)
	{
		eed.nParam[0] = m_pEntity->GetChild(i)->GetId();
		ProcessEvent( eed );
	}
	if (m_pEntity->GetParent())
	{
		eed.nParam[0] = m_pEntity->GetId();
		if (m_pEntity->GetParent()->GetProxy(ENTITY_PROXY_PHYSICS))
			m_pEntity->GetParent()->GetProxy(ENTITY_PROXY_PHYSICS)->ProcessEvent(eed);
	}

	pe_type previousPhysType = PE_NONE;
	Vec3 v(ZERO);

	if (m_pPhysicalEntity)
	{
		previousPhysType = m_pPhysicalEntity->GetType();
		if (params.type==PE_ARTICULATED && previousPhysType==PE_LIVING)
		{
			pe_status_dynamics sd;
			m_pPhysicalEntity->GetStatus(&sd);
			m_pEntity->PrePhysicsActivate(true); // make sure we are completely in-sync with physics b4 ragdollizing
			v = sd.v;
//params.fStiffnessScale=1200.0f;
			DestroyPhysicalEntity(false, params.fStiffnessScale>0);
		}	else if(params.type != PE_NONE)
		{
			DestroyPhysicalEntity();
		}
	}

	// Reset special flags.
	m_nFlags &= ~(FLAG_SYNC_CHARACTER|FLAG_PHYS_CHARACTER);

	switch (params.type)
	{
	case PE_RIGID:
	case PE_STATIC:
		PhysicalizeSimple(params);
		break;
	case PE_LIVING:
		PhysicalizeLiving(params);
		break;
	case PE_ARTICULATED:
		// Check for Special case, converting from living characters to the rag-doll.
		if (previousPhysType != PE_LIVING)
		{
			CreatePhysicalEntity(params);
			PhysicalizeGeomCache(params);
			PhysicalizeCharacter(params);

			// Apply joint velocities if desired
			if (params.bCopyJointVelocities)
			{
				// [*DavidR | 6/Oct/2010] This call is bound to be 80% redundant with the previous two, but it will make sure
				// the latest joint velocity preservation code gets used even when the previous character is not a living entity
				ConvertCharacterToRagdoll(params,v);
			}
		}
		else
		{
			ConvertCharacterToRagdoll(params,v);
		}
		break;
	case PE_PARTICLE:
		PhysicalizeParticle(params);
		break;
	case PE_SOFT:
		PhysicalizeSoft(params);
		break;
	case PE_WHEELEDVEHICLE:
		CreatePhysicalEntity(params);
		break;
	case PE_ROPE:
		if (params.nSlot >= 0)
			PhysicalizeCharacter(params);
		break;	
	case PE_AREA:
		PhysicalizeArea(params);
		break;
	case PE_NONE:
		SetFlags(GetFlags() & ~FLAG_POS_EXTRAPOLATED);
		DestroyPhysicalEntity();
		break;	
	default:
		DestroyPhysicalEntity();
		break;
	}

	//////////////////////////////////////////////////////////////////////////
	// Set physical entity Buoyancy.
	//////////////////////////////////////////////////////////////////////////
	if (m_pPhysicalEntity)
	{
		pe_params_flags pf;
		pf.flagsOR = pef_log_state_changes|pef_log_poststep;
#   if !defined(RELEASE)
    if (m_pPhysicalEntity->GetType() == PE_ARTICULATED) pf.flagsOR |= pef_monitor_state_changes;
#   endif 
		m_pPhysicalEntity->SetParams(&pf);

		if (params.pBuoyancy)
			m_pPhysicalEntity->SetParams(params.pBuoyancy);

		//////////////////////////////////////////////////////////////////////////
		// Assign physical materials mapping table for this material.
		//////////////////////////////////////////////////////////////////////////
		CRenderProxy *pRenderProxy = m_pEntity->GetRenderProxy();
		if (pRenderProxy)
		{
			IMaterial *pMtl = pRenderProxy->GetMaterial();
			if (params.nSlot >= 0)
				pMtl = pRenderProxy->GetRenderMaterial(params.nSlot);

			if (pMtl)
			{
				// Assign custom material to physics.
				int surfaceTypesId[MAX_SUB_MATERIALS];
				memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
				int numIds = pMtl->FillSurfaceTypeIds(surfaceTypesId);

				pe_params_part ppart;
				ppart.nMats = numIds;
				ppart.pMatMapping = surfaceTypesId;
				m_pPhysicalEntity->SetParams( &ppart );
			}
		}
	}

	if (m_pPhysicalEntity)
	{
		pe_params_foreign_data pfd;
		pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
		pfd.pForeignData = m_pEntity;
		if (!(m_nFlags & FLAG_ATTACH_CLOTH_WHEN_VISIBLE))
			m_pPhysicalEntity->SetParams( &pfd );

		if (CheckFlags(FLAG_PHYSICS_DISABLED))
		{
			// If Physics was disabled disable physics on new physical object now.
			PhysicalWorld()->DestroyPhysicalEntity(m_pPhysicalEntity, 1);
		}
	}
	// Check if character physics disabled.
	if (CheckFlags(FLAG_PHYSICS_DISABLED|FLAG_PHYS_CHARACTER))
	{
		int nSlot = (m_nFlags & CHARACTER_SLOT_MASK);
		ICharacterInstance *pCharacter = m_pEntity->GetCharacter(nSlot);
		if (pCharacter && pCharacter->GetISkeletonPose()->GetCharacterPhysics())
			pCharacter->GetISkeletonPose()->DestroyCharacterPhysics( PHYS_ENTITY_DISABLE );
	}

	SEntityEvent eea(ENTITY_EVENT_ATTACH);
	for(i=0; i<m_pEntity->GetChildCount() ;i++)
	{
		eea.nParam[0] = m_pEntity->GetChild(i)->GetId();
		ProcessEvent( eea );
	}
	if (m_pEntity->GetParent())
	{
		eea.nParam[0] = m_pEntity->GetId();
		if (m_pEntity->GetParent()->GetProxy(ENTITY_PROXY_PHYSICS))
			m_pEntity->GetParent()->GetProxy(ENTITY_PROXY_PHYSICS)->ProcessEvent(eea);
	}

	if (params.type != PE_NONE) // are actually physicalising
	{
		if (m_pEntity->IsHidden())
		{
			EnablePhysics(false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::CreatePhysicalEntity( SEntityPhysicalizeParams &params )
{
	// Initialize Rigid Body.
	pe_params_pos ppos;
	ppos.pMtx3x4 = const_cast<Matrix34*>(&m_pEntity->GetWorldTM());
	if (params.type==PE_ARTICULATED)
		ppos.iSimClass = 2;
	m_pPhysicalEntity = PhysicalWorld()->CreatePhysicalEntity( 
		(pe_type)params.type,&ppos,m_pEntity,PHYS_FOREIGN_ID_ENTITY,CEntitySystem::IdToHandle(m_pEntity->GetId()).GetIndex() );
	m_pPhysicalEntity->AddRef();

	if (params.nFlagsOR != 0)
	{
		pe_params_flags pf;
		pf.flagsOR = params.nFlagsOR;
		m_pPhysicalEntity->SetParams(&pf);
	}

	if (params.pCar)
	{
		m_pPhysicalEntity->SetParams(params.pCar);
	}

	if (params.type==PE_ARTICULATED)
	{
		pe_params_articulated_body pab;
		pab.bGrounded = 0;
		pab.scaleBounceResponse = 1;
		pab.bCheckCollisions = 1;
		pab.bCollisionResp = 1;
		m_pPhysicalEntity->SetParams(&pab);
	}

	OnChangedPhysics(true);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::AssignPhysicalEntity( IPhysicalEntity *pPhysEntity, int nSlot )
{
	assert(pPhysEntity);

	// Get Position form physical entity.
	(m_pPhysicalEntity = pPhysEntity)->AddRef();

	// This set flag prevents endless recursion of events recieved from physics.
	SetFlags( m_nFlags|FLAG_IGNORE_XFORM_EVENT );
  //
  Matrix34* mt = const_cast<Matrix34*>(nSlot>=0 ? &m_pEntity->GetSlotWorldTM(nSlot) : &m_pEntity->GetWorldTM());
	pe_params_pos ppos;
  ppos.pos = mt->GetTranslation();

  Matrix34 normalised_mat = *mt;
  normalised_mat.OrthonormalizeFast();
  ppos.q = Quat(normalised_mat);
	//ppos.pMtx3x4 = const_cast<Matrix34*>(nSlot>=0 ? &m_pEntity->GetSlotWorldTM(nSlot) : &m_pEntity->GetWorldTM());
	m_pPhysicalEntity->SetParams(&ppos);

	//////////////////////////////////////////////////////////////////////////
	// Set user data of physical entity to be pointer to the entity.
	pe_params_foreign_data pfd;
	pfd.pForeignData = m_pEntity;
	pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
	m_pPhysicalEntity->SetParams(&pfd);

	//////////////////////////////////////////////////////////////////////////
	// Enable update callbacks.
	pe_params_flags pf;
	pf.flagsOR = pef_log_state_changes|pef_log_poststep;
	m_pPhysicalEntity->SetParams(&pf);

	SetFlags( m_nFlags&(~FLAG_IGNORE_XFORM_EVENT) );
	
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::PhysicalizeSimple( SEntityPhysicalizeParams &params )
{
	CreatePhysicalEntity( params );

	if (params.nSlot < 0)
	{
		// Use all slots.
		for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
		{
			AddSlotGeometry( slot,params );
		}
	}
	else
	{
		// Use only specified slot.
		AddSlotGeometry( params.nSlot,params );
	}

	if ((params.type == PE_RIGID || params.type==PE_LIVING) && m_pPhysicalEntity)
	{
		// Rigid bodies should report at least one collision per frame.
		pe_simulation_params sp;
		sp.maxLoggedCollisions = 1;
		m_pPhysicalEntity->SetParams(&sp);
		pe_params_flags pf;
		pf.flagsOR = pef_log_collisions;
		m_pPhysicalEntity->SetParams(&pf);
	}
}

//////////////////////////////////////////////////////////////////////////
int CPhysicalProxy::AddSlotGeometry( int nSlot,SEntityPhysicalizeParams &params, int bNoSubslots )
{
	CEntityObject *pSlot = m_pEntity->GetSlot(nSlot);
	if (pSlot && pSlot->pCharacter)
	{
		PhysicalizeCharacter( params );
		return -1;
	}
	else if (pSlot && pSlot->pChildRenderNode && pSlot->pChildRenderNode->GetRenderNodeType() == eERType_GeomCache)
	{
		PhysicalizeGeomCache(params);
		return -1;
	}

	IStatObj *pStatObj = m_pEntity->GetStatObj(nSlot | ENTITY_SLOT_ACTUAL & -bNoSubslots);
	if (!pStatObj)
		return -1;
	int physId = -1;

	pe_geomparams partpos;
	Matrix34 mtx;	mtx.SetIdentity();
	Vec3 scale = m_pEntity->GetScale();

	//if (pSlot->HaveLocalMatrix())
	{
		mtx = m_pEntity->GetSlotLocalTM(nSlot | ENTITY_SLOT_ACTUAL & -bNoSubslots, false);
		mtx.SetTranslation(Diag33(scale)*mtx.GetTranslation());
		//scale *= mtx.GetColumn(0).len();
	}
	partpos.pMtx3x4 = &mtx;

	// nType 0 is physical geometry, 1 - obstruct geometry. 
	// Obstruct geometry should not be used for collisions 
	// (geom_collides flag should not be set ). 
	// It could be that an object has only mat_obstruct geometry. 
	// GetPhysGeom() will return 0 - nothing to collide. 
	// GetPhysGeom(PHYS_GEOM_TYPE_NO_COLLIDE) will return obstruct geometry, 
	// it should be used for AI visibility check and newer for collisions. 
	// the flag geom_collides should not be present in obstruct geometry physicalization.
	CEntity *pAdam = (CEntity*)m_pEntity->GetAdam();
	CPhysicalProxy *pAdamProxy = pAdam->GetPhysicalProxy();
	if (!pAdamProxy || !pAdamProxy->m_pPhysicalEntity || m_partId0==0)
		pAdamProxy = this;
	if (pAdamProxy!=this)
	{
		mtx = m_pEntity->GetLocalTM()*mtx;
		//if (m_partId0==0)
		//	m_partId0 = (GetMaxPartId0(pAdam)/PARTID_LINKED+1)*PARTID_LINKED;
	} else if (max(max(fabs_tpl(scale.x-1.0f),fabs_tpl(scale.y-1.0f)),fabs_tpl(scale.z-1.0f))>0.0001f)
		mtx = mtx*Matrix33::CreateScale(scale);

	partpos.flags = geom_collides|geom_floats;
	partpos.flags &= params.nFlagsAND;
	partpos.flagsCollider &= params.nFlagsAND;
	partpos.density = params.density;
	partpos.mass = params.mass;
	pStatObj->Physicalize(pAdamProxy->m_pPhysicalEntity, &partpos, m_partId0+nSlot, params.szPropsOverride);

	return m_partId0+nSlot;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::RemoveSlotGeometry( int nSlot )
{
	CEntityObject *pSlot = m_pEntity->GetSlot(nSlot);
	if (!pSlot || !m_pPhysicalEntity || pSlot->pStatObj && pSlot->pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
		return;

	CPhysicalProxy *pAdamProxy = ((CEntity*)m_pEntity->GetAdam())->GetPhysicalProxy();
	if (!pAdamProxy || !pAdamProxy->m_pPhysicalEntity || m_partId0<PARTID_LINKED)
		pAdamProxy = this;
	if (pSlot->pStatObj	&& pSlot->pStatObj->GetPhysGeom())
		pAdamProxy->m_pPhysicalEntity->RemoveGeometry(m_partId0+nSlot);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::MovePhysics(CPhysicalProxy *dstPhysics)
{
	//--- Release any existing physics on the dest
	if (dstPhysics->m_pPhysicalEntity) 
	{ 
		dstPhysics->m_pPhysicalEntity->Release();
		PhysicalWorld()->DestroyPhysicalEntity(dstPhysics->m_pPhysicalEntity);
	}

	dstPhysics->m_nFlags = m_nFlags;
	dstPhysics->m_pPhysicalEntity = m_pPhysicalEntity;

	m_pPhysicalEntity = NULL;
	m_nFlags &= ~(CHARACTER_SLOT_MASK|FLAG_PHYS_CHARACTER|FLAG_SYNC_CHARACTER);

	//////////////////////////////////////////////////////////////////////////
	// Set user data of physical entity to be pointer to the entity.
	pe_params_foreign_data pfd;
	pfd.pForeignData = dstPhysics->m_pEntity;
	pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
	dstPhysics->m_pPhysicalEntity->SetParams(&pfd);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::UpdateSlotGeometry( int nSlot, IStatObj *pStatObjNew, float mass, int bNoSubslots )
{
	//CEntityObject *pSlot = m_pEntity->GetSlot(nSlot);
	//if (!pSlot || !m_pPhysicalEntity || pSlot->pStatObj && !pSlot->pStatObj->GetRenderMesh())
	//	return;
	IStatObj *pStatObj = m_pEntity->GetStatObj(nSlot);
	if (!m_pPhysicalEntity)
		return;
	if (pStatObjNew && pStatObjNew->GetFlags() & STATIC_OBJECT_COMPOUND)
	{
		Matrix34 mtx;	
		float scale = m_pEntity->GetScale().x;
		mtx = m_pEntity->GetSlotLocalTM(nSlot, false);
		mtx.SetTranslation(mtx.GetTranslation()*scale);
		scale *= mtx.GetColumn(0).len();
		mtx.SetScale(Vec3(scale,scale,scale));
		pe_action_remove_all_parts tmp;
		m_pPhysicalEntity->Action(&tmp);
		pStatObjNew->PhysicalizeSubobjects(m_pPhysicalEntity, &mtx, mass,-1);
		return;
	}	else if (pStatObj && pStatObj->GetPhysGeom())
	{
		pe_params_part pp;
		pp.partid = nSlot;
		pp.pPhysGeom = pStatObj->GetPhysGeom();
		if (mass>=0)
			pp.mass = mass;

		CRenderProxy *pRenderProxy = m_pEntity->GetRenderProxy();
		int surfaceTypesId[MAX_SUB_MATERIALS];
		memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
		if (pRenderProxy && pRenderProxy->GetRenderMaterial(nSlot))
		{ // Assign custom material to physics.
			pp.nMats = pRenderProxy->GetRenderMaterial(nSlot)->FillSurfaceTypeIds(surfaceTypesId);
			pp.pMatMapping = surfaceTypesId;
		} else {
			pp.nMats = pp.pPhysGeom->nMats;
			pp.pMatMapping = pp.pPhysGeom->pMatMapping;
		}

		pe_status_pos sp;	sp.partid = nSlot;
		if (m_pPhysicalEntity->GetStatus(&sp))
			m_pPhysicalEntity->SetParams(&pp);
		else
		{
			SEntityPhysicalizeParams epp;
			epp.mass = mass;
			AddSlotGeometry(nSlot, epp, bNoSubslots);
		}
	} else
		m_pPhysicalEntity->RemoveGeometry(nSlot);
}

//////////////////////////////////////////////////////////////////////////
phys_geometry* CPhysicalProxy::GetSlotGeometry( int nSlot )
{
	CEntityObject *pSlot = m_pEntity->GetSlot(nSlot);
	if (!pSlot || !pSlot->pStatObj)
		return 0;

	phys_geometry *pPhysGeom = pSlot->pStatObj->GetPhysGeom();
	return pPhysGeom;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::DestroyPhysicalEntity(bool bDestroyCharacters, int iMode)
{
	if (m_nFlags & FLAG_POS_EXTRAPOLATED)
	{
		SetFlags( m_nFlags | FLAG_IGNORE_XFORM_EVENT );
		pe_status_pos sp;
		if (m_pPhysicalEntity && m_pPhysicalEntity->GetStatus(&sp))
			m_pEntity->SetPos(sp.pos);
		SetFlags( m_nFlags & ~(FLAG_IGNORE_XFORM_EVENT | FLAG_POS_EXTRAPOLATED) );
	}

	if (m_partId0>=PARTID_LINKED)
	{
		CPhysicalProxy *pAdamProxy = ((CEntity*)m_pEntity->GetAdam())->GetPhysicalProxy();
		if (!pAdamProxy || !pAdamProxy->m_pPhysicalEntity)
			pAdamProxy = this;
		if (pAdamProxy!=this)
			for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
				pAdamProxy->m_pPhysicalEntity->RemoveGeometry(m_partId0+slot);
	}

	bool bCharDeleted = false;
	for(int i=0; i<m_pEntity->GetSlotCount(); i++) if (m_pEntity->GetCharacter(i))
	{
		bool bSameChar = m_pEntity->GetCharacter(i)->GetISkeletonPose()->GetCharacterPhysics()==m_pPhysicalEntity;
		if (bDestroyCharacters)
		{
			m_pEntity->GetCharacter(i)->GetISkeletonPose()->DestroyCharacterPhysics(iMode);
			bCharDeleted |= bSameChar;
		}	else if (bSameChar && iMode==0)
			m_pEntity->GetCharacter(i)->GetISkeletonPose()->SetCharacterPhysics(0);
	}

	if (m_pPhysicalEntity) 
	{ 
		m_pPhysicalEntity->Release();
		if (!bCharDeleted)
			PhysicalWorld()->DestroyPhysicalEntity(m_pPhysicalEntity,iMode);
	}
	m_pPhysicalEntity = NULL;

	IRenderNode *pRndNode = m_pEntity->GetRenderProxy();
	if (pRndNode)
		pRndNode->m_nInternalFlags &= ~(IRenderNode::WAS_INVISIBLE | IRenderNode::WAS_IN_VISAREA | IRenderNode::WAS_FARAWAY);

	OnChangedPhysics(false);
}

//////////////////////////////////////////////////////////////////////////
IPhysicalEntity* CPhysicalProxy::QueryPhyscalEntity( IEntity *pEntity ) const
{
	CEntity *pCEntity = (CEntity*)pEntity;
	if (pCEntity && pCEntity->GetPhysicalProxy())
	{
		CPhysicalProxy *pPhysProxy = pCEntity->GetPhysicalProxy();
		return pPhysProxy->m_pPhysicalEntity;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::PhysicalizeLiving( SEntityPhysicalizeParams &params )
{
	// Dimensions and dynamics must be specified.
	assert( params.pPlayerDimensions && "Player dimensions parameter must be specified." );
	assert( params.pPlayerDynamics && "Player dynamics parameter must be specified." );
	if (!params.pPlayerDimensions || !params.pPlayerDynamics)
		return;

	PhysicalizeSimple(params);

	if (m_pPhysicalEntity)
	{
		m_pPhysicalEntity->SetParams( params.pPlayerDimensions );
		m_pPhysicalEntity->SetParams( params.pPlayerDynamics );
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::PhysicalizeParticle( SEntityPhysicalizeParams &params )
{
	assert( params.pParticle && "Particle parameter must be specified." );
	if (!params.pParticle)
		return;

	CreatePhysicalEntity( params );
	m_pPhysicalEntity->SetParams( params.pParticle );
}


//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::ReattachSoftEntityVtx( IPhysicalEntity *pAttachToEntity, int nAttachToPart )
{
	for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
	{
		CEntityObject *pSlot = m_pEntity->GetSlot(slot);
		if (!pSlot || !pSlot->pStatObj)
			continue;

		phys_geometry *pPhysGeom = pSlot->pStatObj->GetPhysGeom();
		if (!pPhysGeom)
			continue;

		IRenderMesh *pRM = pSlot->pStatObj->GetRenderMesh();
		if (pRM)
		{
			AttachSoftVtx( pRM, pAttachToEntity, nAttachToPart );
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CPhysicalProxy::AttachSoftVtx(IRenderMesh *pRM, IPhysicalEntity *pAttachToEntity, int nAttachToPart)
{
	strided_pointer<ColorB> pColors(0);
	pRM->LockForThreadAccess();
	if ((pColors.data=(ColorB*)pRM->GetColorPtr(pColors.iStride, FSL_READ)))
	{
		pe_action_attach_points aap;
		aap.nPoints = 0;
		const int verticesCount = pRM->GetVerticesCount();
		aap.piVtx = new int[verticesCount];
		uint32 dummy=0,mask=0, *pVtxMap=pRM->GetPhysVertexMap();
		int i;
		if (!pVtxMap)
			pVtxMap=&dummy,mask=~0;

		for(i=0; i<pRM->GetVerticesCount(); i++)
		{
			if (pColors[i].g == 0)
			{
				PREFAST_ASSUME(aap.nPoints >= 0 && aap.nPoints < verticesCount);
				aap.piVtx[aap.nPoints++] = pVtxMap[i&~mask] | i&mask;
			}
		}
		if (aap.nPoints)
		{
			if (pAttachToEntity)
			{
				aap.pEntity = pAttachToEntity;
				if (nAttachToPart >= 0)
					aap.partid = nAttachToPart;
			}
			m_pPhysicalEntity->Action(&aap);
		}
		delete[] aap.piVtx;
	}
	pRM->UnLockForThreadAccess();
}

void CPhysicalProxy::PhysicalizeSoft( SEntityPhysicalizeParams &params )
{
	bool bReady = false;
	CreatePhysicalEntity( params );

	// Find first slot with static physical geometry.
	for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
	{
		CEntityObject *pSlot = m_pEntity->GetSlot(slot);
		IRenderMesh *pRM;
		if (!pSlot || !pSlot->pStatObj)
			continue;

		phys_geometry *pPhysGeom = pSlot->pStatObj->GetPhysGeom();
		if (!pPhysGeom)
			continue;

		const Vec3& scaleVec = m_pEntity->GetScale(); 
		float abs0 = (scaleVec.x - scaleVec.y) * (scaleVec.x - scaleVec.y);
		float abs1 = (scaleVec.x - scaleVec.z) * (scaleVec.x - scaleVec.z);
		float abs2 = (scaleVec.y - scaleVec.z) * (scaleVec.y - scaleVec.z);
		if ((abs0 + abs1 + abs2) > FLT_EPSILON) 
			EntityWarning( "<CPhysicalProxy::PhysicalizeSoft> '%s' non-uniform scaling not supported for softbodies",m_pEntity->GetEntityTextDescription());
		float scale = scaleVec.x; 

		pe_geomparams partpos;
		if (pSlot->HaveLocalMatrix())
		{
			partpos.pMtx3x4 = const_cast<Matrix34*>(&m_pEntity->GetSlotLocalTM(slot, false));
		}
		partpos.density = params.density;
		partpos.mass = params.mass;
		partpos.flags = 0;
		partpos.pLattice = pSlot->pStatObj->GetTetrLattice();
		partpos.scale = scale;
		pSlot->pStatObj->UpdateVertices(0,0,0,0,0); // with 0 input will just prep skin data
		if (pSlot->pStatObj->GetFlags() & STATIC_OBJECT_CLONE)
			partpos.flags |= geom_can_modify;
		m_pPhysicalEntity->AddGeometry( pPhysGeom,&partpos );
		if (partpos.pLattice)
		{
			pe_action_attach_points aap; 
			aap.nPoints = -1; // per-point auto-attachment
			if (params.nAttachToPart>=0)
				m_pPhysicalEntity->Action(&aap);
		} else
		{
			if (!(pRM = pSlot->pStatObj->GetRenderMesh()))
				continue;
			AttachSoftVtx(pRM, params.pAttachToEntity,params.nAttachToPart);
		}

		bReady = true;
		break;
	}

	if (!bReady && m_pPhysicalEntity)
	{
		pe_params_foreign_data pfd;
		pfd.iForeignData = PHYS_FOREIGN_ID_USER;
		pfd.pForeignData = params.pAttachToEntity;
		if (params.pAttachToEntity)
			params.pAttachToEntity->AddRef();
		pfd.iForeignFlags = params.nAttachToPart;
		m_pPhysicalEntity->SetParams(&pfd);
		m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_SEND_RENDER_EVENT);
		m_nFlags |= FLAG_ATTACH_CLOTH_WHEN_VISIBLE;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::PhysicalizeArea( SEntityPhysicalizeParams &params )
{
	// Create area physical entity.
	assert( params.pAreaDef );
	if (!params.pAreaDef)
		return;
	if (!params.pAreaDef->pGravityParams && !params.pBuoyancy)
		return;

	AffineParts affineParts;
	affineParts.SpectralDecompose( m_pEntity->GetWorldTM() );
	float fEntityScale = affineParts.scale.x;

	SEntityPhysicalizeParams::AreaDefinition *pAreaDef = params.pAreaDef;

	IPhysicalEntity *pPhysicalEntity = NULL;

	switch (params.pAreaDef->areaType)
	{
		case SEntityPhysicalizeParams::AreaDefinition::AREA_SPHERE:
			{
				primitives::sphere geomSphere;
				geomSphere.center.x = 0;
				geomSphere.center.y = 0;
				geomSphere.center.z = 0;
				geomSphere.r = pAreaDef->fRadius;
				IGeometry *pGeom = PhysicalWorld()->GetGeomManager()->CreatePrimitive( primitives::sphere::type,&geomSphere );
				pPhysicalEntity = PhysicalWorld()->AddArea( pGeom,affineParts.pos,affineParts.rot,fEntityScale );
			}
			break;

		case SEntityPhysicalizeParams::AreaDefinition::AREA_BOX:
			{
				primitives::box geomBox;
				geomBox.Basis.SetIdentity();
				geomBox.center = (pAreaDef->boxmax + pAreaDef->boxmin) * 0.5f;
				geomBox.size = (pAreaDef->boxmax - pAreaDef->boxmin) * 0.5f;
				geomBox.bOriented = 0;
				IGeometry *pGeom = PhysicalWorld()->GetGeomManager()->CreatePrimitive( primitives::box::type,&geomBox );
				pPhysicalEntity = PhysicalWorld()->AddArea( pGeom,affineParts.pos,affineParts.rot,fEntityScale );
			}
			break;

		case SEntityPhysicalizeParams::AreaDefinition::AREA_GEOMETRY:
			{
				// Will not physicalize if no slot geometry.
				phys_geometry *pPhysGeom = GetSlotGeometry(params.nSlot);
				if (!pPhysGeom || !pPhysGeom->pGeom)
				{
					EntityWarning( "<CPhysicalProxy::PhysicalizeArea> No Physical Geometry in Slot %d of Entity %s",params.nSlot,m_pEntity->GetEntityTextDescription() );
					return;
				}
				pPhysicalEntity = PhysicalWorld()->AddArea( pPhysGeom->pGeom,affineParts.pos,affineParts.rot,fEntityScale );
			}
			break;

		case SEntityPhysicalizeParams::AreaDefinition::AREA_SHAPE:
			{
				assert( pAreaDef->pPoints );
				if (!pAreaDef->pPoints)
					return;

				pPhysicalEntity = PhysicalWorld()->AddArea( pAreaDef->pPoints,pAreaDef->nNumPoints,pAreaDef->zmin,pAreaDef->zmax );
			}
			break;

		case SEntityPhysicalizeParams::AreaDefinition::AREA_CYLINDER:
			{
				primitives::cylinder geomCylinder;
				geomCylinder.center = pAreaDef->center - affineParts.pos;
				if (pAreaDef->axis.len2())
					geomCylinder.axis = pAreaDef->axis;
				else
					geomCylinder.axis = Vec3(0,0,1);
				geomCylinder.r = pAreaDef->fRadius;
				geomCylinder.hh = pAreaDef->zmax - pAreaDef->zmin;
				/*
				CryLogAlways( "CYLINDER: center:(%f %f %f) axis:(%f %f %f) radius:%f height:%f",
					geomCylinder.center.x, geomCylinder.center.y, geomCylinder.center.z,
					geomCylinder.axis.x, geomCylinder.axis.y, geomCylinder.axis.z,
					geomCylinder.r, geomCylinder.hh );
					*/
				IGeometry *pGeom = PhysicalWorld()->GetGeomManager()->CreatePrimitive( primitives::cylinder::type, &geomCylinder );
				pPhysicalEntity = PhysicalWorld()->AddArea( pGeom, affineParts.pos,affineParts.rot,fEntityScale );
			}
			break;

		case SEntityPhysicalizeParams::AreaDefinition::AREA_SPLINE:
			{
				if (!pAreaDef->pPoints)
					return;
				Matrix34 rmtx = m_pEntity->GetWorldTM().GetInverted();
				for(int i=0;i<pAreaDef->nNumPoints;i++)
					pAreaDef->pPoints[i] = rmtx*pAreaDef->pPoints[i];
				pPhysicalEntity = PhysicalWorld()->AddArea( pAreaDef->pPoints, pAreaDef->nNumPoints, pAreaDef->fRadius,
					affineParts.pos,affineParts.rot,fEntityScale );
				if (params.pAreaDef->pGravityParams)
					params.pAreaDef->pGravityParams->bUniform = 0;
			}
			break;
	}

	if (!pPhysicalEntity)
		return;

	AssignPhysicalEntity( pPhysicalEntity );

	if (params.nFlagsOR != 0)
	{
		pe_params_flags pf;
		pf.flagsOR = params.nFlagsOR;
		m_pPhysicalEntity->SetParams(&pf);
	}

	if (params.pAreaDef->pGravityParams)
	{
		if (!is_unused(pAreaDef->pGravityParams->falloff0))
		{
			if (pAreaDef->pGravityParams->falloff0 < 0.f)
				MARK_UNUSED pAreaDef->pGravityParams->falloff0;
			else
			{
				pe_status_pos pos;
				pPhysicalEntity->GetStatus(&pos);
			}
		}
		m_pPhysicalEntity->SetParams( params.pAreaDef->pGravityParams );
	}
	if (params.pBuoyancy)
	{
		m_pPhysicalEntity->SetParams( params.pBuoyancy );
	}

	if (m_pEntity->GetFlags() & ENTITY_FLAG_OUTDOORONLY)
	{
		pe_params_foreign_data fd;
		fd.iForeignFlags = PFF_OUTDOOR_AREA;
		m_pPhysicalEntity->SetParams(&fd);
	}
}

/////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::PhysicalizeGeomCache(SEntityPhysicalizeParams &params)
{
	if (params.nSlot < 0)
	{
		return false;
	}

	IGeomCacheRenderNode *pGeomCacheRenderNode = m_pEntity->GetGeomCacheRenderNode(params.nSlot);
	if (!pGeomCacheRenderNode)
	{
		return false;
	}

	IGeomCache* pGeomCache = pGeomCacheRenderNode->GetGeomCache();
	if (!pGeomCache)
	{
		return false;
	}

	pe_articgeomparams geomParams;
	geomParams.flags = geom_collides|geom_floats;
	geomParams.density = -1;
	geomParams.mass = -1;

	pGeomCacheRenderNode->InitPhysicalEntity(m_pPhysicalEntity, geomParams);
	m_pEntity->InvalidateTM(ENTITY_XFORM_POS | ENTITY_XFORM_ROT | ENTITY_XFORM_SCL);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::PhysicalizeCharacter( SEntityPhysicalizeParams &params )
{
	if (params.nSlot < 0)
		return false;

	ICharacterInstance *pCharacter = m_pEntity->GetCharacter(params.nSlot);
	if (!pCharacter)
		return false;
	int partid0 = PARTID_CGA*(params.nSlot+1);
	Matrix34 mtxloc = m_pEntity->GetSlotLocalTM(params.nSlot,false); mtxloc.ScaleColumn(m_pEntity->GetScale());

	m_nFlags |= FLAG_PHYS_CHARACTER;
	m_nFlags &= ~CHARACTER_SLOT_MASK;
	m_nFlags |= (params.nSlot & CHARACTER_SLOT_MASK);
	
	if (pCharacter->GetOjectType() != CGA)
		m_nFlags |= FLAG_SYNC_CHARACTER, partid0=0;

  if (params.type == PE_LIVING)
	{
		pe_params_part pp;
		pp.ipart = 0;
		pp.flagsAND = ~(geom_colltype_ray|geom_colltype13);
		pp.bRecalcBBox = 0;
		m_pPhysicalEntity->SetParams(&pp);
	}

	int i,iAux;
	pe_params_foreign_data pfd;
	pfd.pForeignData = m_pEntity;
	pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
	pfd.iForeignFlagsOR = PFF_UNIMPORTANT;

	if (!m_pPhysicalEntity || params.type == PE_LIVING)
	{
		if (params.fStiffnessScale < 0)
			params.fStiffnessScale = 1.0f;
		IPhysicalEntity *pCharPhys = pCharacter->GetISkeletonPose()->CreateCharacterPhysics( m_pPhysicalEntity,params.mass,-1,params.fStiffnessScale, 
			params.nLod, mtxloc);
		pCharacter->GetISkeletonPose()->CreateAuxilaryPhysics( pCharPhys, m_pEntity->GetSlotWorldTM(params.nSlot),params.nLod );

		for (iAux=0;pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux);iAux++)
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux)->SetParams(&pfd);
		if (!m_pPhysicalEntity && iAux==1) 
		{ // we have a single physicalized rope
			pe_params_flags pf;
			pf.flagsOR = pef_log_poststep;
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(0)->SetParams(&pf);
		}

		IAttachmentManager *pAttman = pCharacter->GetIAttachmentManager();
		IAttachmentObject *pAttachmentObject;
		if (pAttman) for(i=pAttman->GetAttachmentCount()-1; i>=0; i--)
			if ((pAttachmentObject=pAttman->GetInterfaceByIndex(i)->GetIAttachmentObject()) && 
					pAttman->GetInterfaceByIndex(i)->GetType()==CA_BONE && (pCharacter=pAttachmentObject->GetICharacterInstance()))
				for (iAux=0;pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux);iAux++)
					pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux)->SetParams(&pfd);

		if (pCharPhys)
		{	// make sure the skeleton goes before the ropes in the list
			pe_params_pos pp;
			pp.iSimClass = 6;
			pCharPhys->SetParams(&pp);
			pp.iSimClass = 4;
			pCharPhys->SetParams(&pp);
		}
	}
	else
	{
		pCharacter->GetISkeletonPose()->BuildPhysicalEntity(m_pPhysicalEntity,params.mass,-1,params.fStiffnessScale,params.nLod,partid0, mtxloc);
		pCharacter->GetISkeletonPose()->CreateAuxilaryPhysics(m_pPhysicalEntity, m_pEntity->GetSlotWorldTM(params.nSlot),params.nLod);
		pCharacter->GetISkeletonPose()->SetCharacterPhysics(m_pPhysicalEntity);
		pe_params_pos pp;
		pp.q = m_pEntity->m_qRotation;
		for(IEntity *pent=m_pEntity->GetParent(); pent; pent=pent->GetParent())
			pp.q = pent->GetRotation()*pp.q;
		m_pPhysicalEntity->SetParams(&pp);
		for (iAux=0;pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux);iAux++)
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(iAux)->SetParams(&pfd);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::ConvertCharacterToRagdoll( SEntityPhysicalizeParams &params, const Vec3 &velInitial )
{
	if (params.nSlot < 0)
		return false;

	ICharacterInstance *pCharacter = m_pEntity->GetCharacter(params.nSlot);
	if (!pCharacter)
		return false;

	m_nFlags |= FLAG_PHYS_CHARACTER;
	m_nFlags |= FLAG_SYNC_CHARACTER;
	m_nFlags &= ~CHARACTER_SLOT_MASK;
	m_nFlags |= (params.nSlot & CHARACTER_SLOT_MASK);

	// This is special case when converting living character into the rag-doll	
	IPhysicalEntity *pPhysEntity = pCharacter->GetISkeletonPose()->RelinquishCharacterPhysics(//m_pEntity->GetSlotWorldTM(params.nSlot),
		m_pEntity->GetWorldTM(), params.fStiffnessScale, params.bCopyJointVelocities, velInitial);
	if (pPhysEntity)
	{
		// Store current velocity.
		//pe_status_dynamics sd;
		//sd.v.Set(0,0,0);
		if (m_pPhysicalEntity)
		{
			//m_pPhysicalEntity->GetStatus(&sd);
			DestroyPhysicalEntity(false);
		}

		AssignPhysicalEntity( pPhysEntity, -1 );//params.nSlot );

		int i,nAux;// = pCharacter->CreateAuxilaryPhysics( pPhysEntity, m_pEntity->GetSlotWorldTM(params.nSlot) );
		pe_params_foreign_data pfd;
		pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
		if (pPhysEntity)
			pPhysEntity->SetParams(&pfd);
		pfd.pForeignData = m_pEntity;
		pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
		pe_params_flags pf;
		pf.flagsOR = pef_log_collisions|params.nFlagsOR; // without nMaxLoggedCollisions it will only allow breaking thing on contact
		pf.flagsAND = params.nFlagsAND;
		pPhysEntity->SetParams(&pf);
		pe_simulation_params sp;
		sp.maxLoggedCollisions = 3;
		pPhysEntity->SetParams(&sp);
		pf.flagsOR = pef_traceable|rope_collides|rope_collides_with_terrain;//|rope_ignore_attachments;
		pe_params_rope pr; pr.flagsCollider = geom_colltype0;
		for (nAux=0; pCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux); nAux++)
		{
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux)->SetParams(&pfd);
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux)->SetParams(&pf);
			pCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux)->SetParams(&pr);
		}

		//////////////////////////////////////////////////////////////////////////
		// Iterate thru character attachments, and set rope parameters.
		IAttachmentManager *pAttman = pCharacter->GetIAttachmentManager();
		if (pAttman)
		{
			for(i=pAttman->GetAttachmentCount()-1; i>=0; i--)
			{
				IAttachmentObject *pAttachmentObject = pAttman->GetInterfaceByIndex(i)->GetIAttachmentObject();
				if (pAttachmentObject && pAttman->GetInterfaceByIndex(i)->GetType()==CA_BONE)
				{
					ICharacterInstance *pAttachedCharacter = pAttachmentObject->GetICharacterInstance();
					if (pAttachedCharacter)
						for(nAux=0; pAttachedCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux); nAux++)
						{
							pAttachedCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux)->SetParams(&pfd);
							pAttachedCharacter->GetISkeletonPose()->GetCharacterPhysics(nAux)->SetParams(&pf);
						}
				}
			}
		}

		if (!params.bCopyJointVelocities)
		{
			//////////////////////////////////////////////////////////////////////////
			// Restore previous velocity
			pe_action_set_velocity asv;
			asv.v = velInitial;
			pPhysEntity->Action(&asv);
		}
	}

	// Force character bones to be updated from physics bones.
	//if (m_pPhysicalEntity)
	//	pCharacter->SynchronizeWithPhysicalEntity( m_pPhysicalEntity );
	/*SEntityUpdateContext ctx;
	if (m_pEntity->GetRenderProxy())
		m_pEntity->GetRenderProxy()->Update(ctx);*/
	SetFlags( m_nFlags | FLAG_IGNORE_XFORM_EVENT );
	m_pEntity->SetRotation(Quat(IDENTITY));
	SetFlags( m_nFlags & ~FLAG_IGNORE_XFORM_EVENT );

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::SyncCharacterWithPhysics()
{
	 // Find characters.
	int nSlot = (m_nFlags & CHARACTER_SLOT_MASK);
	ICharacterInstance *pCharacter = m_pEntity->GetCharacter(nSlot);
	if (pCharacter)
	{
		pe_type type = m_pPhysicalEntity->GetType();
		switch (type)
		{
		case PE_ARTICULATED:
			pCharacter->GetISkeletonPose()->SynchronizeWithPhysicalEntity( m_pPhysicalEntity );
			break;
		/*case PE_LIVING:
			pCharacter->UpdatePhysics(1.0f);
			break;*/
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CPhysicalProxy::PhysicalizeFoliage( int iSlot )
{
	if (m_pEntity->GetRenderProxy())
		m_pEntity->GetRenderProxy()->PhysicalizeFoliage(true, 0, iSlot);
	CEntityObject *pSlot = m_pEntity->GetSlot(iSlot);

	if (pSlot && pSlot->pStatObj)
	{
		pSlot->pStatObj->PhysicalizeFoliage(m_pPhysicalEntity, m_pEntity->GetSlotWorldTM(iSlot), pSlot->pFoliage);
		return pSlot->pFoliage!=0;
	}
	return false;
}

void CPhysicalProxy::DephysicalizeFoliage( int iSlot )
{
	CEntityObject *pSlot = m_pEntity->GetSlot(iSlot);
	if (pSlot && pSlot->pFoliage)
	{
		pSlot->pFoliage->Release();
		pSlot->pFoliage = 0;
	}
}

IFoliage *CPhysicalProxy::GetFoliage( int iSlot )
{
	CEntityObject *pSlot = m_pEntity->GetSlot(iSlot);
	if (pSlot)
		return pSlot->pFoliage;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::AddImpulse( int ipart, const Vec3 &pos,const Vec3 &impulse,bool bPos,float fAuxScale, float fPushScale/* = 1.0f */)
{
	ENTITY_PROFILER

	IPhysicalEntity *pPhysicalEntity = m_pPhysicalEntity;
	CPhysicalProxy *pAdamProxy = ((CEntity*)m_pEntity->GetAdam())->GetPhysicalProxy();
	if (pAdamProxy && pAdamProxy->m_pPhysicalEntity)
		pPhysicalEntity = pAdamProxy->m_pPhysicalEntity;

	if (!pPhysicalEntity)
		return;

	const pe_type physicalEntityType = pPhysicalEntity->GetType();
	const bool bNotLiving = physicalEntityType != PE_LIVING;

	//if (m_pPhysicalEntity && (!m_bIsADeadBody || (m_pEntitySystem->m_pHitDeadBodies->GetIVal() )))
	if (pPhysicalEntity && CVar::pHitDeadBodies->GetIVal() && (fPushScale > 0.0f))
	{
		// Ignore the pushScale on not living entities
		const float fImpulseScale = bNotLiving ? fAuxScale : fPushScale;

		Vec3 mod_impulse = impulse;
		pe_status_nparts statusTmp;
		if (!(pPhysicalEntity->GetStatus(&statusTmp)>5 && physicalEntityType==PE_ARTICULATED))
		{	// don't scale impulse for complex articulated entities
			pe_status_dynamics sd;
			float minVel = CVar::pMinImpulseVel->GetFVal();
			if (minVel>0 && pPhysicalEntity->GetStatus(&sd) && mod_impulse*mod_impulse<sqr(sd.mass*minVel))
			{
				float fScale = CVar::pImpulseScale->GetFVal();
				if (fScale>0)
					mod_impulse *= fScale;
				else if (sd.mass<CVar::pMaxImpulseAdjMass->GetFVal())
					mod_impulse = mod_impulse.normalized()*(minVel*sd.mass);
			}
		}
		pe_action_impulse ai;
		ai.partid = ipart;
		if(bPos)
			ai.point = pos;
		ai.impulse = mod_impulse * fImpulseScale;
		pPhysicalEntity->Action(&ai);
	}

//	if (bPos && m_bVisible && (!m_physic || bNotLiving || (CVar::pHitCharacters->GetIVal())))
	if (bPos && (bNotLiving || (CVar::pHitCharacters->GetIVal())))
	{
		// Use all slots.
		for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
		{
			ICharacterInstance *pCharacter = m_pEntity->GetCharacter(slot);
			if (pCharacter)
				pCharacter->GetISkeletonPose()->AddImpact(ipart, pos, impulse*fAuxScale);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::SetTriggerBounds( const AABB &bbox )
{
	ENTITY_PROFILER

	if (bbox.min == bbox.max)
	{
		// Destroy trigger bounds.
		if (m_pColliders && m_pColliders->m_pPhysicalBBox)
		{
			PhysicalWorld()->DestroyPhysicalEntity(m_pColliders->m_pPhysicalBBox);
			delete m_pColliders;
			m_pColliders = 0;
		}
		return;
	}
	if (!m_pColliders)
	{
		m_pColliders = new Colliders;
		m_pColliders->m_pPhysicalBBox = 0;
	}

	if (!m_pColliders->m_pPhysicalBBox)
	{	
		// Create a physics bbox to get the callback from physics when a move event happens.
		m_pColliders->m_pPhysicalBBox = PhysicalWorld()->CreatePhysicalEntity(PE_STATIC);
		pe_params_pos parSim;
		parSim.iSimClass = SC_TRIGGER;
		m_pColliders->m_pPhysicalBBox->SetParams(&parSim);
		pe_params_foreign_data fd;
		fd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
		fd.pForeignData = m_pEntity;
		fd.iForeignFlags = ent_rigid | ent_living | ent_triggers;
		m_pColliders->m_pPhysicalBBox->SetParams(&fd);

		//pe_params_flags pf;
		//pf.flagsOR = pef_log_state_changes|pef_log_poststep;
		//m_pColliders->m_pPhysicalBBox->SetParams(&pf);
	}

	if (Vec3(bbox.max-bbox.min).GetLengthSquared() > 10000*10000)
	{
		EntityWarning( "Too big physical bounding box set for entity %s",m_pEntity->GetEntityTextDescription() );
	}

	Vec3 pos = m_pEntity->GetWorldTM().GetTranslation();
	pe_params_pos ppos;
	ppos.pos = pos;
	m_pColliders->m_pPhysicalBBox->SetParams(&ppos);

	pe_params_bbox parBBox;
	parBBox.BBox[0] = pos + bbox.min;
	parBBox.BBox[1] = pos + bbox.max;
	m_pColliders->m_pPhysicalBBox->SetParams(&parBBox);
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::GetTriggerBounds( AABB &bbox )
{
	pe_params_bbox pbb;
	if (m_pColliders && m_pColliders->m_pPhysicalBBox && m_pColliders->m_pPhysicalBBox->GetParams(&pbb))
	{
		bbox.min = pbb.BBox[0];
		bbox.max = pbb.BBox[1];

		// Move to local space.
		pe_status_pos ppos;
		m_pColliders->m_pPhysicalBBox->GetStatus(&ppos);
		bbox.min -= ppos.pos;
		bbox.max -= ppos.pos;
	}
	else
	{
		bbox.min = bbox.max = Vec3(0,0,0);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::OnPhysicsPostStep(EventPhysPostStep *pEvent)
{
	if (CheckFlags(FLAG_PHYSICS_DISABLED))
		return;

	if (!m_pPhysicalEntity)
	{
		if (m_nFlags & FLAG_SYNC_CHARACTER && !m_pEntity->IsActive())
			m_pEntity->ActivateForNumUpdates(4);
		return;
	}

	// Guards to not let physical proxy move physical entity in response to XFORM event.
	SetFlags( m_nFlags|FLAG_IGNORE_XFORM_EVENT );

	// Set transformation on the entity from transformation of the physical entity.
	pe_status_pos ppos;
	// If Entity is attached do not accept entity position from physics (or assume its world space coords)
	if (!(m_pEntity->m_pBinds && m_pEntity->m_pBinds->pParent))
	{
		if (pEvent)
			ppos.pos=pEvent->pos, ppos.q=pEvent->q;
		else
			m_pPhysicalEntity->GetStatus(&ppos);

		m_timeLastSync = gEnv->pTimer->GetCurrTime();

		// NOTE: Living entity orientations are not controlled by physics, and should thus be ignored here.
		// We simply set the current entitiy orientation, which will be automatically culled/ignored inside SetPosRotScale.
		if (m_pPhysicalEntity->GetType() == PE_LIVING)
			ppos.q = m_pEntity->GetRotation();

		m_pEntity->SetPosRotScale(ppos.pos,ppos.q,m_pEntity->GetScale(),ENTITY_XFORM_PHYSICS_STEP |
			((m_nFlags & FLAG_DISABLE_ENT_SERIALIZATION) ? ENTITY_XFORM_NO_PROPOGATE : 0) |
			ENTITY_XFORM_NO_EVENT & gEnv->pPhysicalWorld->GetPhysVars()->bLogStructureChanges-1);
		SetFlags( m_nFlags & ~FLAG_POS_EXTRAPOLATED );
	}

	pe_type physType = m_pPhysicalEntity->GetType();

	if (physType != PE_RIGID) // In Rigid body sub-parts are not controlled by physics.
	{
		if (m_nFlags & FLAG_SYNC_CHARACTER)
		{
			//SyncCharacterWithPhysics();
			if (!m_pEntity->IsActive())
				m_pEntity->ActivateForNumUpdates(4);
		}
		else
		{
			// Use all slots.
			ppos.flags = status_local;
			for (int slot = 0; slot < m_pEntity->GetSlotCount(); slot++)
			{
				int nSlotFlags = m_pEntity->GetSlotFlags(slot);
				if (!(nSlotFlags&ENTITY_SLOT_IGNORE_PHYSICS))
				{
					ppos.partid = slot+m_partId0;
					if (m_pPhysicalEntity->GetStatus(&ppos)) 
					{
						Matrix34 tm = Matrix34::Create( Vec3(ppos.scale,ppos.scale,ppos.scale),ppos.q,ppos.pos );
						m_pEntity->SetSlotLocalTM(slot,tm);
					}
				}
			}
		}

		if (physType==PE_SOFT && !(m_pEntity->GetRenderProxy()->m_nInternalFlags & IRenderNode::WAS_INVISIBLE))
		{
			pe_status_softvtx ssv;
			const Vec3& entityScale = m_pEntity->GetScale(); 
			const float rscale = 1.f/entityScale.x;

			m_pPhysicalEntity->GetStatus(&ssv);
			m_pEntity->SetPosRotScale(ssv.pos,ssv.q,entityScale,ENTITY_XFORM_PHYSICS_STEP );

			IStatObj *pStatObj = m_pEntity->GetStatObj(0);
			IStatObj *pStatObjNew = pStatObj->UpdateVertices(ssv.pVtx,ssv.pNormals, 0,ssv.nVtx, ssv.pVtxMap, rscale);
			if (pStatObjNew!=pStatObj)
			{
				ssv.pMesh->SetForeignData(pStatObjNew,0);
        m_pEntity->GetRenderProxy()->SetSlotGeometry(0, pStatObjNew);
			}
			m_pEntity->GetRenderProxy()->InvalidateLocalBounds();	
		}
	}

	SetFlags( m_nFlags&(~FLAG_IGNORE_XFORM_EVENT) );
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::AttachToPhysicalEntity( IPhysicalEntity *pPhysEntity )
{
	// Get Position from physical entity.
	if (m_pPhysicalEntity = pPhysEntity)
	{
		m_pPhysicalEntity->AddRef();
		pe_params_foreign_data pfd;
		pfd.pForeignData = m_pEntity;
		pfd.iForeignData = PHYS_FOREIGN_ID_ENTITY;
		pPhysEntity->SetParams(&pfd,1);
		gEnv->pPhysicalWorld->SetPhysicalEntityId(pPhysEntity, CEntitySystem::IdToHandle(m_pEntity->GetId()).GetIndex());

		pe_params_flags pf;
		pf.flagsOR = pef_log_state_changes|pef_log_poststep;
		pPhysEntity->SetParams(&pf);

		OnPhysicsPostStep();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::CreateRenderGeometry( int nSlot,IGeometry *pFromGeom, bop_meshupdate *pLastUpdate )
{
	m_pEntity->GetRenderProxy()->SetSlotGeometry( 0,gEnv->p3DEngine->UpdateDeformableStatObj(pFromGeom,pLastUpdate) );
	m_pEntity->GetRenderProxy()->InvalidateBounds( true,true ); // since SetSlotGeometry doesn't do it if pStatObj is the same
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::AddCollider( EntityId id )
{
	// Try to insert new colliding entity to our colliders set.
	std::pair<ColliderSet::iterator,bool> result = m_pColliders->colliders.insert(id);
	if (result.second == true)
	{
		// New collider was successfully added.
		IEntity *pEntity = m_pEntity->GetEntitySystem()->GetEntity(id);
		SEntityEvent event;
		event.event = ENTITY_EVENT_ENTERAREA;
		event.nParam[0] = id;
		event.nParam[1] = 0;
		m_pEntity->SendEvent(event);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::RemoveCollider( EntityId id )
{
	if (!m_pColliders)
		return;

	// Try to remove collider from set, then compare size of set before and after erase.
	int prevSize = m_pColliders->colliders.size();
	m_pColliders->colliders.erase(id);
	if (m_pColliders->colliders.size() != prevSize)
	{
		// If anything was erased.
		SEntityEvent event;
		event.event = ENTITY_EVENT_LEAVEAREA;
		event.nParam[0] = id;
		event.nParam[1] = 0;
		m_pEntity->SendEvent(event);
	}
}

//////////////////////////////////////////////////////////////////////////
CEntity* CPhysicalProxy::GetCEntity( IPhysicalEntity* pPhysEntity )
{
	assert(pPhysEntity);
	CEntity *pEntity = (CEntity*)pPhysEntity->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
	return pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::CheckColliders()
{
	ENTITY_PROFILER

	if (!m_pColliders)
		return;

	AABB bbox;
	GetTriggerBounds(bbox);
	if (bbox.max.x <= bbox.min.x || bbox.max.y <= bbox.min.y || bbox.max.z <= bbox.min.z)
		return; // something wrong

	Vec3 pos = m_pEntity->GetWorldTM().GetTranslation();
	bbox.min += pos;
	bbox.max += pos;

	// resolve collisions
	IPhysicalEntity **ppColliders;
	int nCount = 0;

	// Prepare temporary set of colliders.
	// Entities which will be reported as colliders will be erased from this set.
	// So that all the entities which are left in the set are not colliders anymore.
	ColliderSet tempSet;
	if (m_pColliders)
		tempSet = m_pColliders->colliders;

	if (nCount = PhysicalWorld()->GetEntitiesInBox( bbox.min,bbox.max, ppColliders, 14 ))
	{
		static DynArray<IPhysicalEntity*> s_colliders;
		s_colliders.resize(nCount);
		memcpy( &s_colliders[0],ppColliders,nCount*sizeof(IPhysicalEntity*) );

		// Check if colliding entities are in list.
		for (int i = 0; i < nCount; i++)
		{
			CEntity *pEntity = GetCEntity(s_colliders[i]);
			if (!pEntity)
				continue;
			int id = pEntity->GetId();
			int prevSize = tempSet.size();
			tempSet.erase( id );
			if (tempSet.size() == prevSize)
			{
				// pEntity is a new entity not in list.
				AddCollider( id );
			}
		}
	}
	// If any entity left in this set, then they are not colliders anymore.
	if (!tempSet.empty())
	{
		for (ColliderSet::iterator it = tempSet.begin(); it != tempSet.end(); ++it)
		{
			RemoveCollider( *it );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhysicalProxy::OnContactWithEntity( CEntity *pEntity )
{
	if (m_pColliders)
	{
		// Activate entity for 4 frames.
		m_pEntity->ActivateForNumUpdates(4);
	}
}

void CPhysicalProxy::OnCollision(CEntity *pTarget, int matId, const Vec3 &pt, const Vec3 &n, const Vec3 &vel, const Vec3 &targetVel, int partId, float mass)
{
	CScriptProxy *pProxy = m_pEntity->GetScriptProxy();

	if (pProxy)
	{
		if (CVar::pLogCollisions->GetIVal() != 0)
		{
			string s1 = m_pEntity->GetEntityTextDescription();
			string s2;
			if (pTarget)
				s2 = pTarget->GetEntityTextDescription();
			else
				s2 = "<Unknown>";
			CryLogAlways( "OnCollision %s (Target: %s)",s1.c_str(),s2.c_str() );
		}
		pProxy->OnCollision(pTarget, matId, pt, n, vel, targetVel, partId, mass);
	}
}

void CPhysicalProxy::OnChangedPhysics( bool bEnabled )
{
	SEntityEvent evt(ENTITY_EVENT_ENABLE_PHYSICS);
	evt.nParam[0] = bEnabled;
	m_pEntity->SendEvent( evt );
}

void CPhysicalProxy::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	//pSizer->AddObject(m_pPhysicalEntity);	
}
  
void CPhysicalProxy::EnableNetworkSerialization(bool enable)
{
	if (enable==false)
	{
		m_nFlags |= FLAG_DISABLE_ENT_SERIALIZATION;
		// Pause
		if (m_pPhysicalEntity)
			m_pPhysicalEntity->SetNetworkAuthority(-1,1);
	}
	else
	{
		m_nFlags &= ~FLAG_DISABLE_ENT_SERIALIZATION;
		// Unpause
		if (m_pPhysicalEntity)
			m_pPhysicalEntity->SetNetworkAuthority(-1,0);
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IEntityPhysicalProxy)
