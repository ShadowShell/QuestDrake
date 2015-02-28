////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>


struct SAnimLayerParams : public SProceduralParams
{
	float layer;
	float invert;
};

class CProceduralClipLayerWeight : public TProceduralClip<SAnimLayerParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipLayerWeight, "LayerWeight", 0x914C932663744A54, 0x9F17EA656315FB23)

	virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		m_dataStringCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(params.dataString.c_str());
		m_invert = (params.invert == 1.0f);
		int layer = (int)params.layer;

		CRY_ASSERT_MESSAGE(layer >= 0 && layer < m_scope->GetTotalLayers(), string().Format("CProceduralClipLayerWeight::OnEnter -> Invalid layer passed in: '%d'", layer));
			
		m_animLayer = m_scope->GetBaseLayer() + layer;

		UpdateLayerWeight();
	}

	virtual void OnExit(float blendTime)
	{
		m_charInstance->GetISkeletonAnim()->SetLayerBlendWeight(m_animLayer, 1.f);
	}

	virtual void Update(float timePassed)
	{
		UpdateLayerWeight();
	}

private:

	void UpdateLayerWeight()
	{
		float paramValue = 0.f;

		if(GetParam(m_dataStringCRC, paramValue))
		{
			paramValue = m_invert ? 1.0f - paramValue : paramValue;
			m_charInstance->GetISkeletonAnim()->SetLayerBlendWeight(m_animLayer, paramValue);
		}
	}
	
	uint32 m_dataStringCRC;
	uint8 m_animLayer;
	bool	m_invert;
};

CProceduralClipLayerWeight::CProceduralClipLayerWeight() : m_dataStringCRC(0), m_animLayer(0), m_invert(false)
{
}

CProceduralClipLayerWeight::~CProceduralClipLayerWeight()
{
}

CRYREGISTER_CLASS(CProceduralClipLayerWeight)

class CProceduralClipIKControlledLayerWeight : public TProceduralClip<SAnimLayerParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipIKControlledLayerWeight, "IKLayerWeight", 0x1F2E7D1CC43A42DF, 0xA6FBF8FE59F2FDD1)

		virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		IDefaultSkeleton& rIDefaultSkeleton = m_charInstance->GetIDefaultSkeleton();
		m_jointID = rIDefaultSkeleton.GetJointIDByName(params.dataString.c_str());

		m_invert = (params.invert == 1.0f);
		int layer = (int)params.layer;

		CRY_ASSERT_MESSAGE(layer >= 0 && layer < m_scope->GetTotalLayers(), string().Format("CProceduralClipIKControlledLayerWeight::OnEnter -> Invalid layer passed in: '%d'", layer));

		m_animLayer = m_scope->GetBaseLayer() + layer;

		UpdateLayerWeight();
	}

	virtual void OnExit(float blendTime)
	{
		m_charInstance->GetISkeletonAnim()->SetLayerBlendWeight(m_animLayer, 1.f);
	}

	virtual void Update(float timePassed)
	{
		UpdateLayerWeight();
	}

private:

	void UpdateLayerWeight()
	{
		if(m_jointID >= 0)
		{
			if(ISkeletonPose* pPose = m_charInstance->GetISkeletonPose())
			{
				QuatT joint = pPose->GetRelJointByID(m_jointID);

				float paramValue = clamp_tpl(joint.t.x, 0.f, 1.f);

				if(m_invert)
				{
					paramValue = 1.0f - paramValue;
				}
				
				m_charInstance->GetISkeletonAnim()->SetLayerBlendWeight(m_animLayer, paramValue);
			}
		}
	}

	int16 m_jointID;
	uint8 m_animLayer;
	bool	m_invert;
};

CProceduralClipIKControlledLayerWeight::CProceduralClipIKControlledLayerWeight() : m_jointID(-1), m_animLayer(0), m_invert(false)
{
}

CProceduralClipIKControlledLayerWeight::~CProceduralClipIKControlledLayerWeight()
{
}

CRYREGISTER_CLASS(CProceduralClipIKControlledLayerWeight)


class CProceduralClipLayerManualUpdate : public TProceduralClip<SAnimLayerParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipLayerManualUpdate, "LayerManualUpdate", 0x581A337CBD9A427D, 0xBEECBF15535583DF)

	virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		m_dataStringCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(params.dataString.c_str());
		int layer = (int)params.layer;

		CRY_ASSERT_MESSAGE(layer >= 0 && layer < m_scope->GetTotalLayers(), string().Format("CProceduralClipLayerManualUpdate::OnEnter -> Invalid layer passed in: '%d'", layer));

		m_animLayer = layer + m_scope->GetBaseLayer();

		CAnimation* pAnimation = NULL;
		ISkeletonAnim* pSkeletonAnim = NULL;

		if(GetSkeletonAndAnimation(pSkeletonAnim, pAnimation))
		{
			pAnimation->SetStaticFlag(CA_MANUAL_UPDATE);

			float paramValue = 0.f;

			if(GetParam(m_dataStringCRC, paramValue))
			{
				pSkeletonAnim->SetAnimationNormalizedTime(pAnimation, paramValue);
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
		CAnimation* pAnimation = NULL;
		ISkeletonAnim* pSkeletonAnim = NULL;
		
		if (GetSkeletonAndAnimation(pSkeletonAnim, pAnimation))
		{
			pAnimation->ClearStaticFlag(CA_MANUAL_UPDATE);
		}
	}

	virtual void Update(float timePassed)
	{
		float paramValue = 0.f;

		if(GetParam(m_dataStringCRC, paramValue))
		{
			CAnimation* pAnimation = NULL;
			ISkeletonAnim* pSkeletonAnim = NULL;

			if(GetSkeletonAndAnimation(pSkeletonAnim, pAnimation))
			{
				pSkeletonAnim->SetAnimationNormalizedTime(pAnimation, paramValue);
			}
		}
	}

private:

	bool GetSkeletonAndAnimation(ISkeletonAnim*& pSkeletonAnim, CAnimation*& pAnimation)
	{
		pSkeletonAnim = m_charInstance->GetISkeletonAnim();
		if (pSkeletonAnim->GetNumAnimsInFIFO(m_animLayer) != 0)
		{
			pAnimation = &(pSkeletonAnim->GetAnimFromFIFO(m_animLayer, 0));
			return true;
		}

		return false;
	}

	uint32 m_dataStringCRC;
	uint8 m_animLayer;
};

CProceduralClipLayerManualUpdate::CProceduralClipLayerManualUpdate() : m_dataStringCRC(0), m_animLayer(0)
{
}

CProceduralClipLayerManualUpdate::~CProceduralClipLayerManualUpdate()
{
}

CRYREGISTER_CLASS(CProceduralClipLayerManualUpdate)

class CProceduralClipWeightedList : public TProceduralClip<SAnimLayerParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipWeightedList, "WeightedList", 0x2EEC1D59A87249BE, 0x80B7A02BB9977DB5)

	virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		m_baseLayer = (uint32) params.layer;
		UpdateLayerWeight();
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		UpdateLayerWeight();
	}

private:

	void UpdateLayerWeight()
	{
		SWeightData data;
		if(GetParam(GetParams().dataCRC.crc, data))
		{
			const uint32 numLayers = min(m_scope->GetTotalLayers()-m_baseLayer, static_cast<uint32>(SWeightData::MAX_WEIGHTS));

			uint32 layer = m_baseLayer;
			for (uint32 i=0; i<numLayers; i++, layer++)
			{
				float factor = clamp_tpl(data.weights[i], 0.f, 1.f);

				m_scope->ApplyAnimWeight(layer, factor);
			}
		}
	}

	uint32 m_baseLayer;
};

CProceduralClipWeightedList::CProceduralClipWeightedList()
{
}

CProceduralClipWeightedList::~CProceduralClipWeightedList()
{
}

CRYREGISTER_CLASS(CProceduralClipWeightedList)


class CProceduralClipManualUpdateList : public TProceduralClip<SAnimLayerParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipManualUpdateList, "ManualUpdateList", 0x0778A906A7F1431E, 0x985243A9DD7F07BA)

	virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		m_baseLayer = (uint32) params.layer;
		UpdateLayerTimes();
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		UpdateLayerTimes();
	}

private:

	void UpdateLayerTimes()
	{
		SWeightData data;
		if(GetParam(GetParams().dataCRC.crc, data))
		{
			const uint32 numLayers = min(m_scope->GetTotalLayers()-m_baseLayer, static_cast<uint32>(SWeightData::MAX_WEIGHTS) * 2);

			uint32 layer = m_scope->GetBaseLayer()+m_baseLayer;
			for (uint32 i=0; i<numLayers; i++, layer++)
			{
				uint32 factorID = i>>1;
				float factor = data.weights[factorID];

				const bool bIsNegative = (i%2) == 0;
				if (bIsNegative)
				{
					factor = -factor;
				}

				ISkeletonAnim *pSkeletonAnim = m_charInstance->GetISkeletonAnim();
				int numAnims = pSkeletonAnim->GetNumAnimsInFIFO(layer);
				if (numAnims > 0)
				{
					CAnimation &animation = pSkeletonAnim->GetAnimFromFIFO(layer, numAnims-1);
					animation.SetStaticFlag(CA_MANUAL_UPDATE);
					animation.SetCurrentSegmentNormalizedTime(max(0.0f, factor));
				}
			}
		}
	}

	uint32 m_baseLayer;
};

CProceduralClipManualUpdateList::CProceduralClipManualUpdateList()
{
}

CProceduralClipManualUpdateList::~CProceduralClipManualUpdateList()
{
}

CRYREGISTER_CLASS(CProceduralClipManualUpdateList)

struct SJointAdjustParams : public SProceduralParams
{
	float layer;
	float additive;
	float relative;
	Vec3 position;
};

class CProceduralClipPostProcessAdjust : public TProceduralClip<SJointAdjustParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipPostProcessAdjust, "JointAdjust", 0xB02A7426DB664E2B, 0xAC47F147E57DFC95)

	virtual void OnEnter(float blendTime, float duration, const SJointAdjustParams &params)
	{
		m_layer = (uint32) params.layer;
		m_position = params.position;
		
		if(params.additive == 0.f)
		{
			m_operation = (params.relative == 0.f) ? IAnimationOperatorQueue::eOp_Override : IAnimationOperatorQueue::eOp_OverrideRelative;
		}
		else
		{
			m_operation = (params.relative == 0.f) ? IAnimationOperatorQueue::eOp_Additive : IAnimationOperatorQueue::eOp_AdditiveRelative;
		}

		IDefaultSkeleton& rIDefaultSkeleton = m_charInstance->GetIDefaultSkeleton();
		{
			int16 jointID = rIDefaultSkeleton.GetJointIDByName(params.dataString.c_str());

			if(jointID >= 0)
			{
				m_jointID = jointID;
				CryCreateClassInstance("AnimationPoseModifier_OperatorQueue", m_pPoseModifier);
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "CProceduralClipPostProcessAdjust::OnEnter joint '%s' doesn't exist in skeleton '%s'", params.dataString.c_str(), m_charInstance->GetFilePath());
			}
		}
	}

	virtual void OnExit(float blendTime)
	{
	}

	virtual void Update(float timePassed)
	{
		if(m_jointID >= 0)
		{
			if (ICharacterInstance* pCharacter = m_scope->GetCharInst())
			{
				pCharacter->GetISkeletonAnim()->PushPoseModifier(m_scope->GetBaseLayer() + m_layer, cryinterface_cast<IAnimationPoseModifier>(m_pPoseModifier), "ProceduralClipPostProcessAdjust");

				m_pPoseModifier->Clear();

				ISkeletonPose* pPose = pCharacter->GetISkeletonPose();
				m_pPoseModifier->PushPosition(m_jointID, m_operation, m_position);
			}
		}
	}

private:

	IAnimationOperatorQueue::EOp m_operation;
	int16 m_jointID;
	uint32 m_layer;
	Vec3 m_position;
	IAnimationOperatorQueuePtr m_pPoseModifier;
};

CProceduralClipPostProcessAdjust::CProceduralClipPostProcessAdjust() 
	: m_operation(IAnimationOperatorQueue::eOp_AdditiveRelative)
	, m_jointID(-1)
	, m_layer(0)
	, m_position(ZERO)
{
}

CProceduralClipPostProcessAdjust::~CProceduralClipPostProcessAdjust()
{
}

CRYREGISTER_CLASS(CProceduralClipPostProcessAdjust)





class CProceduralClipLayerAnimSpeed : public TProceduralClip<SAnimLayerParams>
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipLayerAnimSpeed, "LayerAnimSpeed", 0x9EAF88A4F5E5499B, 0x9CEEECDE2B88D53B)


	virtual void OnEnter(float blendTime, float duration, const SAnimLayerParams &params)
	{
		m_dataStringCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(params.dataString.c_str());
		m_invert = (params.invert == 1.0f);
		int layer = (int)params.layer;

		CRY_ASSERT_MESSAGE(layer >= 0 && layer < m_scope->GetTotalLayers(), string().Format("CProceduralClipLayerAnimSpeed::OnEnter -> Invalid layer passed in: '%d'", layer));

		m_animLayer = m_scope->GetBaseLayer() + layer;

		UpdateLayerAnimSpeed();
	}


	virtual void OnExit(float blendTime)
	{
		m_charInstance->GetISkeletonAnim()->SetLayerPlaybackScale(m_animLayer, 1.0f);
	}


	virtual void Update(float timePassed)
	{
		UpdateLayerAnimSpeed();
	}


private:

	void UpdateLayerAnimSpeed()
	{
		float paramValue = 0.f;

		if (GetParam(m_dataStringCRC, paramValue))
		{
			paramValue = m_invert ? 1.0f - paramValue : paramValue;
			m_charInstance->GetISkeletonAnim()->SetLayerPlaybackScale(m_animLayer, paramValue);
		}
	}

	uint32 m_dataStringCRC;
	uint8 m_animLayer;
	bool	m_invert;
};



CProceduralClipLayerAnimSpeed::CProceduralClipLayerAnimSpeed()
	:	m_dataStringCRC(0)
	,	m_animLayer(0)
	,	m_invert(false)
{
}



CProceduralClipLayerAnimSpeed::~CProceduralClipLayerAnimSpeed()
{
}


CRYREGISTER_CLASS(CProceduralClipLayerAnimSpeed)
