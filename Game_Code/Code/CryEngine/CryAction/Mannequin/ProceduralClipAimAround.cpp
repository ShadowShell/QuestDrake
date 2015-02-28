////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>


struct SAimAroundParams : public SProceduralParams
{
	float	blendTime;
	float	layer;
	float yawMin;
	float yawMax;
	float pitchMin;
	float pitchMax;
	float timeMin;
	float timeMax;
};

class CProceduralClipAimAround : public TProceduralClip<SAimAroundParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipAimAround, "RandomAimAround", 0xA42A812473484536, 0x9C1D647C8DEAC662)

	virtual void OnEnter(float blendTime, float duration, const SAimAroundParams &params)
	{
		m_params = &params;

		UpdateLookTarget();
		Vec3 lookPos = m_entity->GetWorldPos();
		lookPos += m_entity->GetRotation() * (m_lookOffset*10.0f);

		const float smoothTime = params.blendTime;
		const uint32 ikLayer = m_scope->GetBaseLayer() + (uint32)params.layer;
		if (!params.animRef.IsEmpty())
		{
			CryCharAnimationParams animParams;
			animParams.m_fTransTime = blendTime;
			animParams.m_nLayerID = ikLayer;
			animParams.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
			int animID = m_charInstance->GetIAnimationSet()->GetAnimIDByCRC(params.animRef.crc);
			m_charInstance->GetISkeletonAnim()->StartAnimationById(animID, animParams);
		}

		IAnimationPoseBlenderDir* poseBlenderAim = m_charInstance->GetISkeletonPose()->GetIPoseBlenderAim();
		if (poseBlenderAim)
		{
			poseBlenderAim->SetState(true);
			poseBlenderAim->SetTarget(lookPos);
			poseBlenderAim->SetPolarCoordinatesSmoothTimeSeconds(smoothTime);
			poseBlenderAim->SetLayer(ikLayer);
			poseBlenderAim->SetFadeInSpeed(blendTime);
		}
	}


	virtual void OnExit(float blendTime)
	{
		IAnimationPoseBlenderDir *poseBlenderAim = m_charInstance->GetISkeletonPose()->GetIPoseBlenderAim();
		if (poseBlenderAim)
		{
			poseBlenderAim->SetState(false);
			poseBlenderAim->SetFadeOutSpeed(blendTime);
		}
	}

	virtual void Update(float timePassed)
	{
		m_lookAroundTime -= timePassed;

		if (m_lookAroundTime < 0.0f)
		{
			UpdateLookTarget();
		}

		Vec3 lookPos = m_entity->GetWorldPos();
		lookPos += m_entity->GetRotation() * (m_lookOffset*10.0f);

		IAnimationPoseBlenderDir* poseBlenderAim = m_charInstance->GetISkeletonPose()->GetIPoseBlenderAim();
		if (poseBlenderAim)
		{
			poseBlenderAim->SetState(true);
			poseBlenderAim->SetTarget(lookPos);
		}
	}

	void UpdateLookTarget()
	{
		const float timeRange  = m_params->timeMax-m_params->timeMin;
		const float yawRange   = m_params->yawMax-m_params->yawMin;
		const float pitchRange = m_params->pitchMax-m_params->pitchMin;

		//--- TODO! Context use of random number generator!
		float yaw   = m_params->yawMin + (Random() * yawRange);
		float pitch = m_params->pitchMin + (Random() * pitchRange);
		m_lookOffset.Set(sin_tpl(yaw), cos_tpl(yaw), 0.0f);
		m_lookAroundTime = m_params->timeMin + (Random() * timeRange);
	}

public:
	float m_lookAroundTime;
	Vec3  m_lookOffset;
	const SAimAroundParams *m_params;
};

CProceduralClipAimAround::CProceduralClipAimAround()
{
}
CProceduralClipAimAround::~CProceduralClipAimAround()
{
}

CRYREGISTER_CLASS(CProceduralClipAimAround)
