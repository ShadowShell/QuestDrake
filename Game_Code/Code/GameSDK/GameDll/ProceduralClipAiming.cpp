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

class CProceduralClipAiming
: public TProceduralContextualClip< SProceduralParams, CProceduralContextAim >
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()

	CRYGENERATE_CLASS( CProceduralClipAiming, "Aiming", 0xb66393b6c9824242, 0xbb8281092dd0d9a0 )

	virtual void OnEnter( float blendTime, float duration, const SProceduralParams& params )
	{
		m_context->SetBlendInTime( blendTime );
		m_context->UpdateProcClipAimingRequest( true );
	}

	virtual void OnExit( float blendTime )
	{
		m_context->SetBlendOutTime( blendTime );
		m_context->UpdateProcClipAimingRequest( false );
	}

	virtual void Update( float timePassed )
	{
		m_context->UpdateProcClipAimingRequest( true );
	}
};


CProceduralClipAiming::CProceduralClipAiming()
{
}

CProceduralClipAiming::~CProceduralClipAiming()
{
}

CRYREGISTER_CLASS( CProceduralClipAiming )
