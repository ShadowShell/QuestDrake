////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>


struct SLookAroundParams : public SProceduralParams
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

class CProceduralClipLookAround : public TProceduralClip<SLookAroundParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipLookAround, "RandomLookAround", 0x58B38D3E2344489e, 0xAA9283E63E5742EB)

	virtual void OnEnter(float blendTime, float duration, const SLookAroundParams &params)
	{
		m_params = &params;
		const float smoothTime = params.blendTime;
		const uint32 ikLayer = m_scope->GetBaseLayer() + (uint32)params.layer;

		UpdateLookTarget();
		Vec3 lookPos = m_entity->GetWorldPos();
		lookPos += m_entity->GetRotation() * (m_lookOffset*10.0f);

		if (!params.animRef.IsEmpty())
		{
			CryCharAnimationParams animParams;
			animParams.m_fTransTime = blendTime;
			animParams.m_nLayerID = ikLayer;
			animParams.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
			int animID = m_charInstance->GetIAnimationSet()->GetAnimIDByCRC(params.animRef.crc);
			m_charInstance->GetISkeletonAnim()->StartAnimationById(animID, animParams);
		}

		IAnimationPoseBlenderDir* poseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();
		if (poseBlenderLook)
		{
			poseBlenderLook->SetState(true);
			poseBlenderLook->SetTarget(lookPos);
			poseBlenderLook->SetFadeoutAngle(DEG2RAD(180.0f));
			poseBlenderLook->SetPolarCoordinatesSmoothTimeSeconds(smoothTime);
			poseBlenderLook->SetLayer(ikLayer);
			poseBlenderLook->SetFadeInSpeed(blendTime);
		}
	}

	virtual void OnExit(float blendTime)
	{
		IAnimationPoseBlenderDir *poseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();
		if (poseBlenderLook)
		{
			poseBlenderLook->SetState(false);
			poseBlenderLook->SetFadeOutSpeed(blendTime);
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


		IAnimationPoseBlenderDir* pIPoseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();
		if (pIPoseBlenderLook)
		{
			pIPoseBlenderLook->SetState(true);
			pIPoseBlenderLook->SetTarget(lookPos);
			pIPoseBlenderLook->SetFadeoutAngle(DEG2RAD(180.0f));
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
	const SLookAroundParams *m_params;
};

CProceduralClipLookAround::CProceduralClipLookAround()
{
}
CProceduralClipLookAround::~CProceduralClipLookAround()
{
}

CRYREGISTER_CLASS(CProceduralClipLookAround)

