////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>

#include "IActorSystem.h"
#include "IAnimatedCharacter.h"

/*
void DebugDrawLocation(const QuatT& location, ColorB colorPos, ColorB colorX, ColorB colorY, ColorB colorZ)
{
	IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

	const float thickness = 7.0f;
	const Vec3 pushUp(0.0f, 0.03f, 0.0f);

	pAuxGeom->DrawLine(location.t + pushUp, colorX, location.t + pushUp + location.q.GetColumn0(), colorX, thickness);
	pAuxGeom->DrawLine(location.t + pushUp, colorY, location.t + pushUp + location.q.GetColumn1(), colorY, thickness);
	pAuxGeom->DrawLine(location.t + pushUp, colorZ, location.t + pushUp + location.q.GetColumn2(), colorZ, thickness);

	const float radius = 0.06f;
	pAuxGeom->DrawSphere(location.t + pushUp, radius, colorPos);
}
*/


struct SPositionAdjustParams : public SProceduralParams
{
	Vec3  idealOffset;
	float idealYaw;
};

struct SPositionAdjuster
{
	void Init(IEntity &entity, const float blendTime)
	{
		m_targetTime = blendTime;
		m_lastTime  = 0.0f;
		m_invalid = false;

		m_delta.zero();
		m_deltaRot.SetIdentity();

		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity.GetId());
		m_pAnimatedCharacter = pActor ? pActor->GetAnimatedCharacter() : NULL;
	}

	void Update(IEntity &entity, float timePassed)
	{
		if(m_invalid)
			return;

		QuatT applyingDelta(IDENTITY);

		const float newTime = min(m_lastTime + timePassed, m_targetTime);
		const float deltaTime = newTime - m_lastTime;
		if (deltaTime > 0.0f)
		{
			Quat totalDeltaRot, lastTotalDeltaRot;

			const float targetTimeInverse = (float)__fres(m_targetTime);

			const float dt = deltaTime * targetTimeInverse;

			const float t		  = newTime * targetTimeInverse;
			const float lastT = m_lastTime * targetTimeInverse;

			totalDeltaRot.SetSlerp(Quat(IDENTITY), m_deltaRot, t);
			lastTotalDeltaRot.SetSlerp(Quat(IDENTITY), m_deltaRot, lastT);

			applyingDelta.q = (!lastTotalDeltaRot * totalDeltaRot);
			applyingDelta.t = (m_delta * dt);

			m_lastTime = newTime;
		}
		else if(m_targetTime==0.f)
		{
			applyingDelta.t = m_delta;
			applyingDelta.q = m_deltaRot;
			m_invalid = true;
		}

		//CryLog("Moving %s from (%f, %f, %f) to (%f, %f, %f) delta (%f, %f, %f) timeDelta: %f time: %f timeTgt: %f", 
		//			entity.GetName(),
		//			entity.GetPos().x, entity.GetPos().y, entity.GetPos().z,
		//			(entity.GetPos()+applyingDelta.t).x, (entity.GetPos()+applyingDelta.t).y, (entity.GetPos()+applyingDelta.t).z,
		//			applyingDelta.t.x, applyingDelta.t.y, applyingDelta.t.z,
		//			deltaTime, newTime, m_targetTime);

		if (m_pAnimatedCharacter)
		{
			m_pAnimatedCharacter->ForceMovement(applyingDelta);
		}
		else
		{
			if(IEntity *pParent = entity.GetParent())
				applyingDelta.t = !pParent->GetRotation() * applyingDelta.t;

			entity.SetPosRotScale(entity.GetPos()+applyingDelta.t, entity.GetRotation()*applyingDelta.q, entity.GetScale(), ENTITY_XFORM_NO_EVENT);
		}
	}

	QuatT m_targetLoc;
	Vec3 m_delta;
	Quat m_deltaRot;
	float m_lastTime;
	float m_targetTime;
	bool m_invalid;
	IAnimatedCharacter *m_pAnimatedCharacter;
};

class CProceduralClipPosAdjust : public TProceduralClip<SPositionAdjustParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipPosAdjust, "PositionAdjust", 0x24DAC26243B446a1, 0xA7CF278C3B3B9669)

	virtual void OnEnter(float blendTime, float duration, const SPositionAdjustParams &params)
	{
		m_posAdjuster.Init(m_scope->GetEntity(), blendTime);

		const QuatT  entityLoc(m_entity->GetWorldPos(), m_entity->GetWorldRotation());

		CAnimation *anim = m_scope->GetTopAnim(0);
		if (anim)
		{
			anim->SetStaticFlag(CA_FULL_ROOT_PRIORITY);
		}

		const bool isRootEntity = IsRootEntity();
		const bool hasParam		  = GetParam("TargetPos", m_posAdjuster.m_targetLoc);

		if (isRootEntity && hasParam)
		{
			Vec3 actualOffset = m_posAdjuster.m_targetLoc.t - entityLoc.t;
			Vec3 idealOffset = m_posAdjuster.m_targetLoc.q * params.idealOffset;
			m_posAdjuster.m_delta = actualOffset - idealOffset;
			Quat idealRotOffset;
			idealRotOffset.SetRotationAA(DEG2RAD(params.idealYaw), Vec3Constants<f32>::fVec3_OneZ);
			Quat actualRotOffset = m_posAdjuster.m_targetLoc.q * !entityLoc.q;
			m_posAdjuster.m_deltaRot = actualRotOffset * !idealRotOffset;

			//CryLog("Init (%f, %f, %f) Target  (%f, %f, %f) Delta: (%f, %f, %f)", entityLoc.t.x, entityLoc.t.y, entityLoc.t.z, m_posAdjuster.m_targetLoc.t.x, m_posAdjuster.m_targetLoc.t.y, m_posAdjuster.m_targetLoc.t.z, m_posAdjuster.m_delta.x, m_posAdjuster.m_delta.y, m_posAdjuster.m_delta.z);
		}
		else
		{
			m_posAdjuster.m_targetLoc.t = entityLoc.t;
			m_posAdjuster.m_targetLoc.q = entityLoc.q;
			m_posAdjuster.m_invalid = true;

			if (!hasParam)
			{
				CryLog("Failed to init PositionAdjust due to missing parameter: TargetPos");
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		m_posAdjuster.Update(*m_entity, timePassed);
	}

protected:
	SPositionAdjuster m_posAdjuster;
};

CProceduralClipPosAdjust::CProceduralClipPosAdjust()
{
}
CProceduralClipPosAdjust::~CProceduralClipPosAdjust()
{
}

CRYREGISTER_CLASS(CProceduralClipPosAdjust)

struct SPositionAdjustAnimParams : public SProceduralParams
{
	float ignoreRotation;
	float ignorePosition;
};

class CProceduralClipPosAdjustAnimPos : public TProceduralClip<SPositionAdjustAnimParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipPosAdjustAnimPos, "PositionAdjustAnimPos", 0x30A9E31C321C4FC1, 0xACD15BE48E2ABC4F)

	virtual void OnEnter(float blendTime, float duration, const SPositionAdjustAnimParams &params)
	{
		m_posAdjuster.Init(m_scope->GetEntity(), blendTime);

		CAnimation *anim = m_scope->GetTopAnim(0);
		if (anim)
		{
			anim->SetStaticFlag(CA_FULL_ROOT_PRIORITY);
		}

		QuatT animStartLoc(IDENTITY);
		uint32 nControllerID = m_charInstance->GetIDefaultSkeleton().GetControllerIDByID(0);
		const bool success = (anim && m_charInstance->GetIAnimationSet()->GetAnimationDCCWorldSpaceLocation(anim,animStartLoc,nControllerID));

		QuatT  entityLoc(m_entity->GetWorldPos(), m_entity->GetWorldRotation());

		if(IPhysicalEntity* pe = m_entity->GetPhysics())
		{
			pe_status_dynamics dyn;
			if (pe->GetStatus(&dyn))
			{
				Vec3 vel = dyn.v;
				
				entityLoc.t += vel * gEnv->pTimer->GetFrameTime();
			}	
			
		}

		const bool isRootEntity = IsRootEntity();

		const char* pParamName = params.dataString.empty() ? "TargetPos" : params.dataString.c_str();
		const bool hasParam		  = GetParam(pParamName, m_posAdjuster.m_targetLoc);

		if (isRootEntity && hasParam && success)
		{
			if(!params.ignorePosition)
			{
				const Vec3 actualOffset = m_posAdjuster.m_targetLoc.t - entityLoc.t;
				const Vec3 idealOffset = m_posAdjuster.m_targetLoc.q * animStartLoc.t;
				m_posAdjuster.m_delta = actualOffset + idealOffset;
			}

			if(!params.ignoreRotation)
			{
				const Quat actualRotOffset = !entityLoc.q * m_posAdjuster.m_targetLoc.q;
				m_posAdjuster.m_deltaRot = actualRotOffset * animStartLoc.q;
			}
			//CryLog("Init %s (%f, %f, %f) Target  (%f, %f, %f) Delta: (%f, %f, %f)", m_scope->GetEntity().GetName(), entityLoc.t.x, entityLoc.t.y, entityLoc.t.z, m_posAdjuster.m_targetLoc.t.x, m_posAdjuster.m_targetLoc.t.y, m_posAdjuster.m_targetLoc.t.z, m_posAdjuster.m_delta.x, m_posAdjuster.m_delta.y, m_posAdjuster.m_delta.z);
		}
		else
		{
			m_posAdjuster.m_targetLoc.t = m_entity->GetPos();
			m_posAdjuster.m_targetLoc.q = m_entity->GetRotation();
			m_posAdjuster.m_invalid = true;

			if (!hasParam)
			{
				CryLog("Failed to init PositionAdjust due to missing parameter: %s", pParamName);
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		m_posAdjuster.Update(*m_entity, timePassed);
	}

protected:
	SPositionAdjuster m_posAdjuster;
};

CProceduralClipPosAdjustAnimPos::CProceduralClipPosAdjustAnimPos()
{
}
CProceduralClipPosAdjustAnimPos::~CProceduralClipPosAdjustAnimPos()
{
}

CRYREGISTER_CLASS(CProceduralClipPosAdjustAnimPos)


class CProceduralClipPosAdjustAnimPosContinuously : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipPosAdjustAnimPosContinuously, "PositionAdjustAnimPosContinuously", 0x82537C5657E34E7A, 0xA9442ED71B9F5E20)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		m_posAdjuster.Init(m_scope->GetEntity(), blendTime);

		CAnimation *anim = m_scope->GetTopAnim(0);
		if (anim)
		{
			anim->SetStaticFlag(CA_FULL_ROOT_PRIORITY);
		}

		QuatT animStartLoc(IDENTITY);
		uint32 nControllerID = m_charInstance->GetIDefaultSkeleton().GetControllerIDByID(0);
		const bool hasStartLocation = (anim && m_charInstance->GetIAnimationSet()->GetAnimationDCCWorldSpaceLocation(anim,animStartLoc,nControllerID ));
		const bool isRootEntity = IsRootEntity();
		const bool hasParam		  = GetParam("TargetPos", m_posAdjuster.m_targetLoc);

		if (isRootEntity && hasParam && hasStartLocation)
		{
			UpdateDeltas(animStartLoc);
		}
		else
		{
			m_posAdjuster.m_targetLoc.t = m_entity->GetPos();
			m_posAdjuster.m_targetLoc.q = m_entity->GetRotation();
			m_posAdjuster.m_invalid = true;

			if (!hasParam)
			{
				CryLog("Failed to init PositionAdjust due to missing parameter: TargetPos");
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		if (m_posAdjuster.m_lastTime > 0)
		{
			CAnimation *anim = m_scope->GetTopAnim(0);
			QuatT animStartLoc(IDENTITY);
			uint32 nControllerID = m_charInstance->GetIDefaultSkeleton().GetControllerIDByID(0);
			const bool hasStartLocation = (anim && m_charInstance->GetIAnimationSet()->GetAnimationDCCWorldSpaceLocation(anim,animStartLoc,nControllerID));

			if (hasStartLocation)
			{
				UpdateDeltas(animStartLoc);

				m_posAdjuster.m_targetTime -= m_posAdjuster.m_lastTime;
				m_posAdjuster.m_lastTime = 0;
			}
		}

		m_posAdjuster.Update(*m_entity, timePassed);
	}

	void UpdateDeltas(QuatT &animStartLoc)
	{
		IActor *pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_entity->GetId());
		IAnimatedCharacter* pAnimatedCharacter = pActor ? pActor->GetAnimatedCharacter() : NULL;
		const Vec3 vExpectedMovement = pAnimatedCharacter ? pAnimatedCharacter->GetExpectedEntMovement() : Vec3Constants<float>::fVec3_Zero;

		const QuatT entityLoc(m_entity->GetPos() + vExpectedMovement, m_entity->GetRotation());

		Vec3 actualOffset = m_posAdjuster.m_targetLoc.t - entityLoc.t;
		Vec3 idealOffset = m_posAdjuster.m_targetLoc.q * -animStartLoc.t;
		m_posAdjuster.m_delta = actualOffset - idealOffset;
		Quat actualRotOffset = !entityLoc.q * m_posAdjuster.m_targetLoc.q;
		m_posAdjuster.m_deltaRot = actualRotOffset * animStartLoc.q;

		//CryLog("Init %s (%f, %f, %f) Target  (%f, %f, %f) Delta: (%f, %f, %f)", m_scope->GetEntity().GetName(), entityLoc.t.x, entityLoc.t.y, entityLoc.t.z, m_posAdjuster.m_targetLoc.t.x, m_posAdjuster.m_targetLoc.t.y, m_posAdjuster.m_targetLoc.t.z, m_posAdjuster.m_delta.x, m_posAdjuster.m_delta.y, m_posAdjuster.m_delta.z);

		/*DebugDrawLocation(
		entityLoc,
		RGBA8(0x30,0x30,0x30,0xff),
		RGBA8(0x50,0x30,0x30,0xff),
		RGBA8(0x30,0x50,0x30,0xff),
		RGBA8(0x30,0x30,0x50,0xff));

		const QuatT worldLoc = entityLoc * animStartLoc.GetInverted();

		DebugDrawLocation(
			worldLoc,
			RGBA8(0x80,0x80,0x80,0xff),
			RGBA8(0xb0,0x80,0x80,0xff),
			RGBA8(0x80,0xb0,0x80,0xff),
			RGBA8(0x80,0x80,0xb0,0xff));
		*/
	}
protected:
	SPositionAdjuster m_posAdjuster;
};

CProceduralClipPosAdjustAnimPosContinuously::CProceduralClipPosAdjustAnimPosContinuously()
{
}
CProceduralClipPosAdjustAnimPosContinuously::~CProceduralClipPosAdjustAnimPosContinuously()
{
}

CRYREGISTER_CLASS(CProceduralClipPosAdjustAnimPosContinuously)


class CProceduralClipPosAdjustTargetLocator : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipPosAdjustTargetLocator, "PositionAdjustTargetLocator", 0xD288D1F85B6B4AF0, 0xA5A4B3CBBE1AD976)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		m_posAdjuster.Init(m_scope->GetEntity(), blendTime);

		const SControllerDef &contDef = m_scope->GetActionController().GetContext().controllerDef;
		int scopeID = contDef.m_scopeIDs.Find(params.dataCRC.crc);
		IScope *scope = (scopeID >= 0) ? m_scope->GetActionController().GetScope(scopeID) : NULL;

		QuatT targetPos(IDENTITY);
		bool hasParam = false;
		if (scope && scope->GetCharInst())
		{
			targetPos.t = scope->GetEntity().GetPos();
			targetPos.q = scope->GetEntity().GetRotation();

			IDefaultSkeleton& rIDefaultSkeleton = scope->GetCharInst()->GetIDefaultSkeleton();
			int jointID = rIDefaultSkeleton.GetJointIDByName(params.dataString.c_str());
			if (jointID >= 0)
			{
				ISkeletonPose* skelPose = scope->GetCharInst()->GetISkeletonPose();
				targetPos = targetPos * skelPose->GetAbsJointByID(jointID);
				hasParam = true;
			}
		}

		m_posAdjuster.m_targetLoc = targetPos;

		CAnimation *anim = m_scope->GetTopAnim(0);
		if (anim)
		{
			anim->SetStaticFlag(CA_FULL_ROOT_PRIORITY);
		}

		const QuatT  entityLoc(m_entity->GetPos(), m_entity->GetRotation());

		const bool isRootEntity = IsRootEntity();

		if (isRootEntity && hasParam)
		{
			Vec3 actualOffset = m_posAdjuster.m_targetLoc.t - entityLoc.t;
			m_posAdjuster.m_delta = actualOffset;
			Quat actualRotOffset = m_posAdjuster.m_targetLoc.q * !entityLoc.q;
			m_posAdjuster.m_deltaRot = actualRotOffset;

			//CryLog("Init %s (%f, %f, %f) Target  (%f, %f, %f) Delta: (%f, %f, %f)", m_scope->GetEntity().GetName(), entityLoc.t.x, entityLoc.t.y, entityLoc.t.z, m_posAdjuster.m_targetLoc.t.x, m_posAdjuster.m_targetLoc.t.y, m_posAdjuster.m_targetLoc.t.z, m_posAdjuster.m_delta.x, m_posAdjuster.m_delta.y, m_posAdjuster.m_delta.z);
		}
		else
		{
			m_posAdjuster.m_targetLoc.t = m_entity->GetPos();
			m_posAdjuster.m_targetLoc.q = m_entity->GetRotation();
			m_posAdjuster.m_invalid = true;

			if (!hasParam)
			{
				CryLog("Failed to init PositionAdjust due to missing target object or joint: %s", params.dataString.c_str());
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		m_posAdjuster.Update(*m_entity, timePassed);
	}

protected:
	SPositionAdjuster m_posAdjuster;
};

CProceduralClipPosAdjustTargetLocator::CProceduralClipPosAdjustTargetLocator()
{
}
CProceduralClipPosAdjustTargetLocator::~CProceduralClipPosAdjustTargetLocator()
{
}

CRYREGISTER_CLASS(CProceduralClipPosAdjustTargetLocator)