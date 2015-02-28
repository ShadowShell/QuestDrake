// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"

#include "ParticleParams.h"
#ifndef _LIB
#include "ParticleParams_TypeInfo.h"
#endif
#include "MFXParticleEffect.h"
#include "MaterialEffectsCVars.h"
#include "IActorSystem.h"

CMFXParticleEffect::CMFXParticleEffect()
{
}

CMFXParticleEffect::~CMFXParticleEffect()
{
}

void CMFXParticleEffect::Execute(SMFXRunTimeEffectParams& params)
{
  FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	if (!(params.playflags & MFX_PLAY_PARTICLES))
		return;
		
  Vec3 pos = params.pos;
	Vec3 dir = ZERO;
	Vec3 inDir = params.dir[0];
	Vec3 reverso = inDir * -1.0f;
	switch (m_particleParams.directionType)
	{
	case MFX_PART_DIRECTION_NORMAL:
		dir = params.normal;
		break;
	case MFX_PART_DIRECTION_RICOCHET:
		dir = reverso.GetRotated(params.normal, gf_PI).normalize();
		break;
	default:
		dir = params.normal;
		break;
	}
 
  bool tryToAttachEffect = (CMaterialEffectsCVars::Get().mfx_EnableAttachedEffects != 0);
  float distToPlayer = 0.f;
  IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
  if (pClientActor)
  {
    distToPlayer = (pClientActor->GetEntity()->GetWorldPos() - params.pos).GetLength();
	tryToAttachEffect = tryToAttachEffect && (pClientActor->GetEntityId() != params.trg);
  }
  
  SMFXParticleEntries::const_iterator end = m_particleParams.m_entries.end();
  for (SMFXParticleEntries::const_iterator it = m_particleParams.m_entries.begin(); it!=end; ++it)
  {
    // choose effect based on distance
    if ((it->maxdist == 0.f) || (distToPlayer <= it->maxdist) && !it->name.empty() )
    { 
      IParticleEffect *pParticle = gEnv->pParticleManager->FindEffect(it->name.c_str());

      if (pParticle)
      {
        SMFXParticleEffectParams& pa = params.particleParams;

        const float pfx_minscale = (it->minscale != 0.f) ? it->minscale : (pa.minscale != 0.f) ? pa.minscale : CMaterialEffectsCVars::Get().mfx_pfx_minScale;
        const float pfx_maxscale = (it->maxscale != 0.f) ? it->maxscale : (pa.maxscale != 0.f) ? pa.maxscale : CMaterialEffectsCVars::Get().mfx_pfx_maxScale; 
        const float pfx_maxdist = (it->maxscaledist != 0.f) ? it->maxscaledist : (pa.maxscaledist != 0.f) ? pa.maxscaledist : CMaterialEffectsCVars::Get().mfx_pfx_maxDist; 
                
        const float truscale = pfx_minscale + ((pfx_maxscale - pfx_minscale) * (distToPlayer!=0.f ? min(1.0f, distToPlayer/pfx_maxdist) : 1.f));  

		bool particleSpawnedAndAttached = tryToAttachEffect ? AttachToTarget(*it, params, pParticle, dir, truscale) : false;

        // If not attached, just spawn the particle
		if (particleSpawnedAndAttached == false)
		{
			pParticle->Spawn( true, IParticleEffect::ParticleLoc(pos, dir, truscale) );
		}
      }
      
      break;
    }
  }
}

bool CMFXParticleEffect::AttachToTarget( const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale )
{
	bool shouldTryToAttach = particleParams.attachToTarget && (params.trg != 0);
	if (!shouldTryToAttach)
	{
		return false;
	}

	IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntity(params.trg);
	if (pTargetEntity)
	{
		//Try to figure out if it's a character using physics type
		IPhysicalEntity* pTargetPhysics = pTargetEntity->GetPhysics();
		int physicsType = pTargetPhysics ? pTargetPhysics->GetType() : PE_NONE;

		bool isCharacter = (physicsType == PE_LIVING) || (physicsType == PE_ARTICULATED);

		if (isCharacter)
		{
			return AttachToCharacter(*pTargetEntity, particleParams, params, dir, scale);
		}
		//else
		//{
			//return AttachToEntity(*pTargetEntity, particleParams, params, pParticleEffect, dir, scale);
		//}
	}

	return false;
}

bool CMFXParticleEffect::AttachToCharacter( IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, const Vec3& dir, float scale )
{
	if (params.partID >= 0)
	{
		//Assume character is loaded in first slot 
		//We could iterate through all available slots, but first one should be good enough
		ICharacterInstance* pCharacterInstace = targetEntity.GetCharacter(0);
		ISkeletonPose* pSkeletonPose = pCharacterInstace ? pCharacterInstace->GetISkeletonPose() : NULL;
		if (pSkeletonPose)
		{
			IDefaultSkeleton& rIDefaultSkeleton	= pCharacterInstace->GetIDefaultSkeleton();
			//It hit the character, but probably in a physicalized attached part, like armor plates, etc
			if (params.partID >= rIDefaultSkeleton.GetJointCount())
			{
				return false;
			}

			//It hit some valid joint, create an attachment
			const char* boneName = rIDefaultSkeleton.GetJointNameByID(params.partID);
			TAttachmentName attachmentName;
			GetNextCharacterAttachmentName(attachmentName);

			IAttachmentManager* pAttachmentManager = pCharacterInstace->GetIAttachmentManager();
			CRY_ASSERT(pAttachmentManager);
			
			//Remove the attachment first (in case was created before)
			pAttachmentManager->RemoveAttachmentByName(attachmentName.c_str());

			//Create attachment on nearest hit bone
			IAttachment* pAttachment = pAttachmentManager->CreateAttachment(attachmentName.c_str(), CA_BONE, boneName, false);
			if (pAttachment)
			{
				//Apply relative offsets
				const QuatT boneLocation = pSkeletonPose->GetAbsJointByID(params.partID);
				Matrix34 inverseJointTM = targetEntity.GetWorldTM() * Matrix34(boneLocation);
				inverseJointTM.Invert();
				Vec3 attachmentOffsetPosition = inverseJointTM * params.pos;
				Quat attachmentOffsetRotation = Quat(inverseJointTM) * targetEntity.GetRotation();

				CRY_ASSERT(attachmentOffsetPosition.IsValid());
				//CRY_ASSERT(attachmentOffsetRotation.IsUnit());

				pAttachment->SetAttRelativeDefault(QuatT(attachmentOffsetRotation, attachmentOffsetPosition));

				//Finally attach the effect
				CEffectAttachment* pEffectAttachment = new CEffectAttachment(particleParams.name.c_str(), Vec3(0,0,0), dir, scale);
				pAttachment->AddBinding(pEffectAttachment);
				
				return true;
			}
		}
	}

	return false;
}

bool CMFXParticleEffect::AttachToEntity( IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale )
{
	if (pParticleEffect)
	{
		int effectSlot = targetEntity.LoadParticleEmitter(-1, pParticleEffect);
		if (effectSlot >= 0)
		{
			Matrix34 hitTM;
			hitTM.Set(Vec3(1.0f, 1.0f, 1.0f), Quat::CreateRotationVDir(dir), params.pos);

			Matrix34 localEffectTM = targetEntity.GetWorldTM().GetInverted() * hitTM;
			localEffectTM.ScaleColumn(Vec3(scale, scale, scale));

			CRY_ASSERT(localEffectTM.IsValid());

			targetEntity.SetSlotLocalTM(effectSlot, localEffectTM);

			return true;
		}
	}

	return false;
}

void CMFXParticleEffect::GetNextCharacterAttachmentName( TAttachmentName& attachmentName )
{
	static int nextId = 0;
	const int maxAttachmentIds = 6;

	attachmentName.Format("Mfx_Particle_Attachment%d", nextId);

	nextId = (nextId < maxAttachmentIds) ? (nextId+1) : 0;
}

void CMFXParticleEffect::ReadXMLNode(XmlNodeRef& node)
{
	IMFXEffect::ReadXMLNode(node);
	  
  for (int i=0; i<node->getChildCount(); ++i)
  {
    XmlNodeRef child = node->getChild(i);
    if (!strcmp(child->getTag(), "Name"))
    {
      SMFXParticleEntry entry;            
      entry.name = child->getContent();

			if (child->haveAttr("userdata"))
				entry.userdata = child->getAttr("userdata");
	      
			if (child->haveAttr("scale"))
				child->getAttr("scale", entry.scale);

			if (child->haveAttr("maxdist"))
				child->getAttr("maxdist", entry.maxdist);

			if (child->haveAttr("minscale"))
				child->getAttr("minscale", entry.minscale);

			if (child->haveAttr("maxscale"))
				child->getAttr("maxscale", entry.maxscale);

			if (child->haveAttr("maxscaledist"))
				child->getAttr("maxscaledist", entry.maxscaledist);

			if (child->haveAttr("attach"))
				child->getAttr("attach", entry.attachToTarget);
	      
			m_particleParams.m_entries.push_back(entry);
    }
  }
  
  MFXParticleDirection dir = MFX_PART_DIRECTION_NORMAL;
	XmlNodeRef dirType = node->findChild("Direction");
	if (dirType)
	{
		const char *val = dirType->getContent();
		if (!strcmp(val, "Normal"))
		{
			dir = MFX_PART_DIRECTION_NORMAL;
		}
		else if (!strcmp(val, "Ricochet"))
		{
			dir = MFX_PART_DIRECTION_RICOCHET;
		}
	}
	m_particleParams.directionType = dir;
	
}

IMFXEffectPtr CMFXParticleEffect::Clone()
{
	CMFXParticleEffect *clone = new CMFXParticleEffect();
	clone->m_effectParams = m_effectParams;
	clone->m_particleParams = m_particleParams;
	return clone;
}

void CMFXParticleEffect::GetResources(SMFXResourceList& rlist)
{
	SMFXParticleListNode *listNode = SMFXParticleListNode::Create();
	
  if (!m_particleParams.m_entries.empty())
  {
    const SMFXParticleEntry& entry = m_particleParams.m_entries.back();    
    listNode->m_particleParams.name = entry.name.c_str();
    listNode->m_particleParams.userdata = entry.userdata.c_str();
    listNode->m_particleParams.scale = entry.scale;
  }  
  listNode->m_particleParams.directionType = m_particleParams.directionType;

  SMFXParticleListNode* next = rlist.m_particleList;
  
  if (!next)
    rlist.m_particleList = listNode;
  else
  { 
    while (next->pNext)
      next = next->pNext;

    next->pNext = listNode;
  }  
}

void CMFXParticleEffect::PreLoadAssets()
{
	LOADING_TIME_PROFILE_SECTION;

	IMFXEffect::PreLoadAssets();
	for (size_t i=0; i<m_particleParams.m_entries.size(); i++)
	{
		SMFXParticleEntry& entry = m_particleParams.m_entries[i];
		if( gEnv->pParticleManager->FindEffect( entry.name.c_str() ) == NULL )
		{
			CryLog( "MFXParticleEffect: Unable to find effect <%s>; Removing from list", entry.name.c_str() );
			entry.name.clear();
		}

		SLICE_AND_SLEEP();
	}
}



