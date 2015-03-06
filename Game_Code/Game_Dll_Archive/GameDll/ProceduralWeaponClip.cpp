#include "StdAfx.h"
#include "Item.h"
#include "ProceduralWeaponContext.h"
#include "Utility/Wiggle.h"





template<typename WeaponOffsetParams>
struct SWeaponProceduralClipParams : public SProceduralParams
{
	WeaponOffsetParams m_offsetParams;
};



struct SStaticWeaponPoseParams
{
	enum EPoseType
	{
		EPT_RightHand=0,
		EPT_LeftHand=2,
		EPT_Zoom=1,
	};

	float	pose_type;						// should be EPoseType
	float	zoom_transition_angle;
	SWeaponOffset m_offset;

	EPoseType GetPoseType() const {return EPoseType(int(pose_type));}
};





class CWeaponPoseOffset : TProceduralContextualClip<SWeaponProceduralClipParams<SStaticWeaponPoseParams>, CProceduralWeaponAnimationContext>
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(CWeaponPoseOffset, "WeaponPose", 0x2851700C9EF348CE, 0xBD8F27D1E71C1272)

public:
	virtual void OnEnter(float blendTime, float duration, const SWeaponProceduralClipParams<SStaticWeaponPoseParams>& staticParams)
	{
		m_context->Initialize(m_scope);

		CWeaponOffsetStack& shoulderOffset = m_context->GetWeaponSway().GetZoomOffset().GetShoulderOffset();
		CWeaponOffsetStack& leftHandOffset = m_context->GetWeaponSway().GetZoomOffset().GetLeftHandOffset();
		SWeaponOffset& zoomOffset = m_context->GetWeaponSway().GetZoomOffset().GetZommOffset();
		m_poseType = staticParams.m_offsetParams.GetPoseType();

		if (m_poseType == SStaticWeaponPoseParams::EPT_RightHand)
		{
			m_offsetId = shoulderOffset.PushOffset(
				ToRadians(staticParams.m_offsetParams.m_offset),
				m_scope->GetBaseLayer(),
				blendTime);
		}
		else if (m_poseType == SStaticWeaponPoseParams::EPT_LeftHand)
		{
			m_offsetId = leftHandOffset.PushOffset(
				ToRadians(staticParams.m_offsetParams.m_offset),
				m_scope->GetBaseLayer(),
				blendTime);
		}
		else if (m_poseType == SStaticWeaponPoseParams::EPT_Zoom)
		{
			zoomOffset = ToRadians(staticParams.m_offsetParams.m_offset);
			m_context->GetWeaponSway().GetZoomOffset().SetZoomTransitionRotation(
				DEG2RAD(staticParams.m_offsetParams.zoom_transition_angle));
		}
	}

	virtual void OnExit(float blendTime)
	{
		CWeaponOffsetStack& shoulderOffset = m_context->GetWeaponSway().GetZoomOffset().GetShoulderOffset();
		CWeaponOffsetStack& leftHandOffset = m_context->GetWeaponSway().GetZoomOffset().GetLeftHandOffset();

		if (m_poseType == SStaticWeaponPoseParams::EPT_RightHand)
			shoulderOffset.PopOffset(m_offsetId, blendTime);
		else if (m_poseType == SStaticWeaponPoseParams::EPT_LeftHand)
			leftHandOffset.PopOffset(m_offsetId, blendTime);

		m_context->Finalize();
	}

	virtual void Update(float timePassed)
	{
		Vec3 aimDirection = Vec3(0.0f, 1.0f, 0.0f);
		if (GetParam(CItem::sActionParamCRCs.aimDirection, aimDirection))
			m_context->SetAimDirection(aimDirection);
		float zoomTransition;
		if (GetParam(CItem::sActionParamCRCs.zoomTransition, zoomTransition))
			m_context->GetWeaponSway().GetZoomOffset().SetZoomTransition(zoomTransition);
	}

private:
	CWeaponOffsetStack::TOffsetId m_offsetId;
	SStaticWeaponPoseParams::EPoseType m_poseType;
};
CRYREGISTER_CLASS(CWeaponPoseOffset);



CWeaponPoseOffset::CWeaponPoseOffset()
	:	m_offsetId(-1)
	,	m_poseType(SStaticWeaponPoseParams::EPT_RightHand)
{
}


CWeaponPoseOffset::~CWeaponPoseOffset()
{
}





class CWeaponSwayOffset : TProceduralContextualClip<SWeaponProceduralClipParams<SStaticWeaponSwayParams>, CProceduralWeaponAnimationContext>
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(CWeaponSwayOffset, "WeaponSway", 0x99C2D1973ADB4ED2, 0x933C570EEEEC4FEB)

public:
	virtual void OnEnter(float blendTime, float duration, const SWeaponProceduralClipParams<SStaticWeaponSwayParams>& staticParams)
	{
		m_context->Initialize(m_scope);
		m_lastSwayParams = m_context->GetWeaponSway().GetLookOffset().GetCurrentStaticParams();
		m_context->GetWeaponSway().GetLookOffset().SetStaticParams(staticParams.m_offsetParams);
		m_context->GetWeaponSway().GetStrafeOffset().SetStaticParams(staticParams.m_offsetParams);
	}

	virtual void OnExit(float blendTime)
	{
		m_context->GetWeaponSway().GetLookOffset().SetStaticParams(m_lastSwayParams);
		m_context->GetWeaponSway().GetStrafeOffset().SetStaticParams(m_lastSwayParams);
		m_context->Finalize();
	}

	virtual void Update(float timePassed)
	{
		SGameWeaponSwayParams gameParams;
		GetParam(CItem::sActionParamCRCs.aimDirection, gameParams.aimDirection);
		GetParam(CItem::sActionParamCRCs.inputMove, gameParams.inputMove);
		GetParam(CItem::sActionParamCRCs.inputRot, gameParams.inputRot);
		GetParam(CItem::sActionParamCRCs.velocity, gameParams.velocity);
		m_context->GetWeaponSway().GetLookOffset().SetGameParams(gameParams);
		m_context->GetWeaponSway().GetStrafeOffset().SetGameParams(gameParams);
	}

private:
	SStaticWeaponSwayParams m_lastSwayParams;
};


CRYREGISTER_CLASS(CWeaponSwayOffset);



CWeaponSwayOffset::CWeaponSwayOffset()
{
}


CWeaponSwayOffset::~CWeaponSwayOffset()
{
}



class CWeaponRecoilOffset : TProceduralContextualClip<SWeaponProceduralClipParams<SStaticWeaponRecoilParams>, CProceduralWeaponAnimationContext>
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(CWeaponRecoilOffset, "WeaponRecoil", 0x687E47B2390B439B, 0xAE790B654097807F)

public:
	virtual void OnEnter(float blendTime, float duration, const SWeaponProceduralClipParams<SStaticWeaponRecoilParams>& staticParams)
	{
		m_context->Initialize(m_scope);
		m_context->GetWeaponSway().GetRecoilOffset().SetStaticParams(staticParams.m_offsetParams);

		const bool firstFire = true;
		m_context->GetWeaponSway().GetRecoilOffset().TriggerRecoil(firstFire);
	}

	virtual void OnExit(float blendTime)
	{
		m_context->Finalize();
	}

	virtual void Update(float timePassed)
	{
		bool fired = false;
		bool firstFire = false;
		GetParam(CItem::sActionParamCRCs.fired, fired);
		GetParam(CItem::sActionParamCRCs.firstFire, firstFire);
		if (fired)
			m_context->GetWeaponSway().GetRecoilOffset().TriggerRecoil(firstFire);
	}
};

CRYREGISTER_CLASS(CWeaponRecoilOffset);



CWeaponRecoilOffset::CWeaponRecoilOffset()
{
}


CWeaponRecoilOffset::~CWeaponRecoilOffset()
{
}






struct SStaticBumpParams
{
	float	time;
	float	shift;
	float	rotation;
};



class CWeaponBumpOffset : TProceduralContextualClip<SWeaponProceduralClipParams<SStaticBumpParams>, CProceduralWeaponAnimationContext>
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(CWeaponBumpOffset, "WeaponBump", 0x7727E5B8AD3E4BF0, 0x908A1C310D2A14CB)

public:
	virtual void OnEnter(float blendTime, float duration, const SWeaponProceduralClipParams<SStaticBumpParams>& staticParams)
	{
		m_context->Initialize(m_scope);

		float fallFactor = 0.0;
		GetParam(CItem::sActionParamCRCs.fallFactor, fallFactor);

		QuatT bump(IDENTITY);
		bump.t.z = -staticParams.m_offsetParams.shift * fallFactor;
		bump.q = Quat::CreateRotationX(DEG2RAD(-staticParams.m_offsetParams.rotation));
		float attack = staticParams.m_offsetParams.time * 0.35f;
		float release = staticParams.m_offsetParams.time * 0.65f;
		float rebounce = 0.25f;

		m_context->GetWeaponSway().GetBumpOffset().AddBump(bump, attack, release, rebounce);
	}

	virtual void OnExit(float blendTime)
	{
		m_context->Finalize();
	}

	virtual void Update(float timePassed)
	{
	}
};

CRYREGISTER_CLASS(CWeaponBumpOffset);


CWeaponBumpOffset::CWeaponBumpOffset()
{
}


CWeaponBumpOffset::~CWeaponBumpOffset()
{
}





struct SStaticWiggleParams
{
	float frequency;
	float intensity;
};



class CWeaponWiggleOffset : TProceduralContextualClip<SWeaponProceduralClipParams<SStaticWiggleParams>, CProceduralWeaponAnimationContext>
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(CWeaponWiggleOffset, "WeaponWiggle", 0x60C85C63C42842D3, 0x8FDE84DC256E41B1)

public:
	virtual void OnEnter(float blendTime, float duration, const SWeaponProceduralClipParams<SStaticWiggleParams>& staticParams)
	{
		m_context->Initialize(m_scope);
		m_blendingIn = true;
		m_timePassed = 0.0f;
		m_blendTime = blendTime;
	}

	virtual void OnExit(float blendTime)
	{
		m_context->Finalize();
		m_blendingIn = false;
		m_timePassed = 0.0f;
		m_blendTime = blendTime;
	}

	virtual void Update(float timePassed)
	{
		m_timePassed += timePassed;

		QuatT shakeOffset(IDENTITY);

		m_wiggler.SetParams(GetParams().m_offsetParams.frequency);
		const float intensity = BlendIntensity() * GetParams().m_offsetParams.intensity;

		Vec3 wiggle = m_wiggler.Update(timePassed) * 2.0f - Vec3(1.0f, 1.0f, 1.0f);
		wiggle.Normalize();
		shakeOffset.t.x = wiggle.x * intensity;
		shakeOffset.t.z = wiggle.y * intensity;
		shakeOffset.q = Quat::CreateRotationY(wiggle.z * intensity);

		m_context->GetWeaponSway().AddCustomOffset(shakeOffset);
	}

private:
	float BlendIntensity() const
	{
		float result = 1.0f;

		if (m_blendTime >= 0.0f)
			result = m_timePassed / m_blendTime;

		if (!m_blendingIn)
			result = 1.0f - result;

		return SATURATE(result);
	}

	CWiggleVec3 m_wiggler;
	float m_blendTime;
	float m_timePassed;
	bool m_blendingIn;
};

CRYREGISTER_CLASS(CWeaponWiggleOffset);


CWeaponWiggleOffset::CWeaponWiggleOffset()
	:	m_blendTime(0.0f)
	,	m_timePassed(0.0f)
	,	m_blendingIn(false)
{
}


CWeaponWiggleOffset::~CWeaponWiggleOffset()
{
}
