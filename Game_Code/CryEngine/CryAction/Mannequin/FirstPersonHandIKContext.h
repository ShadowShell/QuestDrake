////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FIRST_PERSON_HAND_IK_CONTEXT_H__
#define __FIRST_PERSON_HAND_IK_CONTEXT_H__

#include <CryExtension/Impl/ClassWeaver.h>
#include "ICryAnimation.h"
#include "ICryMannequin.h"



class CFirstPersonHandIKContext : public IProceduralContext
{
private:
	struct SParams
	{
		SParams();
		SParams(IDefaultSkeleton& rIDefaultSkeleton);

		int m_weaponTargetIdx;
		int m_leftHandTargetIdx;
		int m_rightBlendIkIdx;
	};

public:
	PROCEDURAL_CONTEXT(CFirstPersonHandIKContext, "FirstPersonHandIK", 0xd8a55b349caa4b53, 0x89bcf1708d565bc3);

	virtual void Initialize(ICharacterInstance* pCharacterInstance);
	virtual void Finalize();
	virtual void Update(float timePassed);

	virtual void SetAimDirection(Vec3 aimDirection);
	virtual void AddRightOffset(QuatT offset);
	virtual void AddLeftOffset(QuatT offset);

private:
	SParams m_params;
	IAnimationOperatorQueuePtr m_pPoseModifier;
	ICharacterInstance* m_pCharacterInstance;

	QuatT m_rightOffset;
	QuatT m_leftOffset;
	Vec3 m_aimDirection;
	int m_instanceCount;
};


#endif