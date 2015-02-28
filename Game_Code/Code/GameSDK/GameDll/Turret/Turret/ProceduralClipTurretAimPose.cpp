#include "StdAfx.h"

#include <ICryAnimation.h>
#include <IAnimationPoseModifier.h>

#include <ICryMannequin.h>

#include <CryExtension/CryCreateClassInstance.h>

#include <Cry_Geo.h>
#include <Cry_GeoDistance.h>

#include "ProceduralContextTurretAimPose.h"


struct STurretAimIKParams 
	: public SProceduralParams
{
	float blendTime;
	float layer;

	float horizontalAimSmoothTime;
	float verticalAimSmoothTime;

	float maxYawDegreesSecond;
	float maxPitchDegreesSecond;
};

struct SProceduralClipWeightHelper
{
public:
	SProceduralClipWeightHelper()
		: m_weight( 0 )
		, m_weightChangeRate( 0 )
	{
	}

	void OnEnter( const float blendInSeconds )
	{
		assert( 0 <= blendInSeconds );
		if ( blendInSeconds == 0 )
		{
			m_weight = 1;
			m_weightChangeRate = 0;
		}
		else
		{
			m_weightChangeRate = ( 1 - m_weight ) / blendInSeconds;
		}
	}

	void Update( const float elapsedSeconds )
	{
		const float weightDelta = m_weightChangeRate * elapsedSeconds;
		const float newWeight = m_weight + weightDelta;
		m_weight = clamp_tpl( newWeight, 0.f, 1.f );
	}

	void OnExit( const float blendOutSeconds )
	{
		assert( 0 <= blendOutSeconds );
		if ( blendOutSeconds == 0 )
		{
			m_weight = 0;
			m_weightChangeRate = 0;
		}
		else
		{
			m_weightChangeRate = -m_weight / blendOutSeconds;
		}
	}

	float GetWeight() const
	{
		return m_weight;
	}

private:
	float m_weight;
	float m_weightChangeRate;
};


class CProceduralClipTurretAimPose
	: TProceduralContextualClip< STurretAimIKParams, CProceduralContextTurretAimPose >
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()

	CRYGENERATE_CLASS( CProceduralClipTurretAimPose, "TurretAimPose", 0x664f9a1cf5004f25, 0x901bf8427f048eb2 )

	virtual void OnEnter( float blendTimeSeconds, float duration, const STurretAimIKParams& params )
	{
		m_weightHelper.OnEnter( blendTimeSeconds );

		const IAnimationSet* pAnimationSet = m_charInstance->GetIAnimationSet();
		assert( pAnimationSet != NULL );

		const AnimCRC aimAnimationCrc = params.animRef.crc;
		const int aimAnimationId = pAnimationSet->GetAnimIDByCRC( aimAnimationCrc );

		if ( aimAnimationId < 0 && aimAnimationCrc != ANIM_CRC_INVALID )
		{
			CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "ProceduralClipTurretAimPose: Requested an aim pose with name '%s' that doesn't exist for entity '%s'. Will use fallback vertical aiming behaviour, and it will look ugly!", GetParams().animRef.GetString(), m_entity->GetName() );
			return;
		}

		CryCharAnimationParams animParams;
		animParams.m_fTransTime = blendTimeSeconds;
		animParams.m_nLayerID = m_context->GetVerticalAimLayer();
		animParams.m_nFlags |= CA_LOOP_ANIMATION;
		animParams.m_nFlags |= CA_ALLOW_ANIM_RESTART;

		ISkeletonAnim* pSkeletonAnim = m_charInstance->GetISkeletonAnim();
		assert( pSkeletonAnim != NULL );

		pSkeletonAnim->StartAnimationById( aimAnimationId, animParams );
	}

	virtual void OnExit( float blendTimeSeconds )
	{
		m_weightHelper.OnExit( blendTimeSeconds );

		ISkeletonAnim* pSkeletonAnim = m_charInstance->GetISkeletonAnim();
		assert( pSkeletonAnim != NULL );

		const int32 verticalAimLayer = m_context->GetVerticalAimLayer();
		pSkeletonAnim->StopAnimationInLayer( verticalAimLayer, blendTimeSeconds );

		const float weight = m_weightHelper.GetWeight();

		const Vec3 targetWorldPosition = CalculateTargetWorldPosition();
		m_context->SetRequestedTargetWorldPosition( targetWorldPosition, weight, blendTimeSeconds );

		const STurretAimIKParams &params = GetParams();
		m_context->SetVerticalSmoothTime( params.verticalAimSmoothTime, weight, blendTimeSeconds );
		m_context->SetMaxYawDegreesPerSecond( params.maxYawDegreesSecond, weight, blendTimeSeconds );
		m_context->SetMaxPitchDegreesPerSecond( params.maxPitchDegreesSecond, weight, blendTimeSeconds );
	}

	virtual void Update( float timePassed )
	{
		m_weightHelper.Update( timePassed );
		const float weight = m_weightHelper.GetWeight();

		const Vec3 targetWorldPosition = CalculateTargetWorldPosition();
		m_context->SetRequestedTargetWorldPosition( targetWorldPosition, weight );

		const STurretAimIKParams &params = GetParams();
		m_context->SetVerticalSmoothTime( params.verticalAimSmoothTime, weight );
		m_context->SetMaxYawDegreesPerSecond( params.maxYawDegreesSecond, weight );
		m_context->SetMaxPitchDegreesPerSecond( params.maxPitchDegreesSecond, weight );
	}

	Vec3 CalculateTargetWorldPosition() const
	{
		QuatT targetWorldLocation;
		const bool hasTargetWorldLocationParam = GetParam( "TargetPos", targetWorldLocation );
		if ( hasTargetWorldLocationParam )
		{
			return targetWorldLocation.t;
		}

		const Vec3 targetWorldPosition = Vec3( 0, 0, 0 );
		return targetWorldPosition;
	}

private:
	SProceduralClipWeightHelper m_weightHelper;
};

CProceduralClipTurretAimPose::CProceduralClipTurretAimPose()
{

}

CProceduralClipTurretAimPose::~CProceduralClipTurretAimPose()
{

}


CRYREGISTER_CLASS( CProceduralClipTurretAimPose )
