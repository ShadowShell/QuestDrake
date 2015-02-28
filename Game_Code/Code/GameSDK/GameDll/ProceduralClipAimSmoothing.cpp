////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "ICryMannequin.h"
#include <ICryAnimation.h>
#include <IAnimationPoseModifier.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "ProceduralContextAim.h"


struct SAimSmoothingParams 
	: public SProceduralParams
{
	float smoothTimeSeconds;
	float maxYawDegreesPerSecond;
	float maxPitchDegreesPerSecond;
};

class CProceduralClipAimSmoothing
	: public TProceduralContextualClip< SAimSmoothingParams, CProceduralContextAim >
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()

	CRYGENERATE_CLASS( CProceduralClipAimSmoothing, "AimSmoothing", 0xbb39e86e529743fc, 0xa734d6bec9a2158b )

	virtual void OnEnter( float blendTime, float duration, const SAimSmoothingParams& params )
	{
		m_requestId = m_context->RequestPolarCoordinatesSmoothingParameters( Vec2( DEG2RAD( params.maxYawDegreesPerSecond ), DEG2RAD( params.maxPitchDegreesPerSecond ) ), params.smoothTimeSeconds );
	}

	virtual void OnExit( float blendTime )
	{
		m_context->CancelPolarCoordinatesSmoothingParameters( m_requestId );
	}

	virtual void Update( float timePassed )
	{
	}

private:
	uint32 m_requestId;
};


CProceduralClipAimSmoothing::CProceduralClipAimSmoothing()
	: m_requestId( 0 )
{
}

CProceduralClipAimSmoothing::~CProceduralClipAimSmoothing()
{
}

CRYREGISTER_CLASS( CProceduralClipAimSmoothing )


