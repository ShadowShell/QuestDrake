////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>

struct SLookIKParams : public SProceduralParams
{
	float	blendTime;
	float	layer;
};

class CProceduralClipLookPose : public TProceduralClip<SLookIKParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipLookPose, "LookPose", 0xB2240E0AED87413D, 0x8B7730C63C890FB9)

	virtual void OnEnter(float blendTime, float duration, const SLookIKParams &params)
	{
		m_paramTargetCrc = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase( "LookTarget" );
		m_token = kInvalidToken;

		Vec3 lookAtTarget;
		if(!GetParam(m_paramTargetCrc, lookAtTarget))
		{
			lookAtTarget = m_entity->GetWorldPos();
			lookAtTarget += m_entity->GetForwardDir() * 10.0f;
		}

		const float smoothTime = params.blendTime;
		const uint32 ikLayer = m_scope->GetBaseLayer() + (uint32)params.layer;
		IAnimationPoseBlenderDir *poseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();
		if (poseBlenderLook)
		{
			poseBlenderLook->SetState(true);
			const bool wasPoseBlenderActive = (0.01f < poseBlenderLook->GetBlend());
			if (!wasPoseBlenderActive)
			{
				poseBlenderLook->SetTarget(lookAtTarget);
			}
			poseBlenderLook->SetPolarCoordinatesSmoothTimeSeconds(smoothTime);
			poseBlenderLook->SetLayer(ikLayer);
			poseBlenderLook->SetFadeInSpeed(blendTime);
		}

		m_IKLayer = ikLayer;

		StartLookAnimation(blendTime);
	}

	virtual void OnExit(float blendTime)
	{
		IAnimationPoseBlenderDir *poseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();

		m_charInstance->GetISkeletonAnim()->StopAnimationInLayer(m_IKLayer, blendTime);

		StopLookAnimation(blendTime);

		if (poseBlenderLook)
		{
			poseBlenderLook->SetState(false);
			poseBlenderLook->SetFadeOutSpeed(blendTime);
		}

	}

	virtual void Update(float timePassed)
	{
		Vec3 lookAtTarget;
		if(GetParam(m_paramTargetCrc, lookAtTarget))
		{
			IAnimationPoseBlenderDir *poseBlenderLook = m_charInstance->GetISkeletonPose()->GetIPoseBlenderLook();
			if(poseBlenderLook)
			{
				poseBlenderLook->SetTarget(lookAtTarget);
			}
		}
	}

private:

	void StartLookAnimation(const float blendTime)
	{
		const SLookIKParams& params = GetParams();
		if (params.animRef.IsEmpty())
		{
			return;
		}

		const int animID = m_charInstance->GetIAnimationSet()->GetAnimIDByCRC(params.animRef.crc);
		if (animID >= 0)
		{
			assert( animID <= 65535 );

			m_token = GetNextToken( (uint16)animID );

			CryCharAnimationParams animParams;
			animParams.m_fTransTime = blendTime;
			animParams.m_nLayerID = m_IKLayer;
			animParams.m_nUserToken = m_token;
			animParams.m_nFlags = CA_LOOP_ANIMATION|CA_ALLOW_ANIM_RESTART;
			m_charInstance->GetISkeletonAnim()->StartAnimationById(animID, animParams);
		}
	}


	void StopLookAnimation(const float blendTime)
	{
		const SLookIKParams& params = GetParams();
		if (params.animRef.IsEmpty())
		{
			return;
		}

		CRY_ASSERT(m_token != kInvalidToken);

		ISkeletonAnim* pSkeletonAnim = m_charInstance->GetISkeletonAnim();
		assert(pSkeletonAnim);

		const int animationCount = pSkeletonAnim->GetNumAnimsInFIFO(m_IKLayer);
		for (int i = 0; i < animationCount; ++i)
		{
			const CAnimation& animation = pSkeletonAnim->GetAnimFromFIFO(m_IKLayer, i);
			const uint32 animationToken = animation.GetUserToken();
			if (animationToken == m_token)
			{
				const bool isTopAnimation = (i == (animationCount - 1));
				if (isTopAnimation)
				{
					pSkeletonAnim->StopAnimationInLayer(m_IKLayer, blendTime);
				}
				// If we found an proc clip look animation not in the top it might be indicative that we're reusing this layer for non look animations,
				// but we can assume that in 99% of cases we're already blending out, since there's a top animation that should be blending in.
				return;
			}
		}
	}

	static uint32 GetNextToken( const uint16 animId )
	{
		static uint16 s_currentToken = 0;
		s_currentToken++;

		return ((animId << 16) | s_currentToken);
	}

	const static uint32 kInvalidToken = 0xFFFFFFFF;

	uint32 m_IKLayer;
	uint32 m_token;

	uint32 m_paramTargetCrc;
};

CProceduralClipLookPose::CProceduralClipLookPose()
{
}

CProceduralClipLookPose::~CProceduralClipLookPose()
{
}

CRYREGISTER_CLASS(CProceduralClipLookPose)
