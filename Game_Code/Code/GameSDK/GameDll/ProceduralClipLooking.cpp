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

#include "ProceduralContextLook.h"

class CProceduralClipLooking
	: public TProceduralContextualClip< SProceduralParams, CProceduralContextLook >
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
		CRYINTERFACE_END()

		CRYGENERATE_CLASS( CProceduralClipLooking, "Looking", 0xB234B14696144D20, 0xB030369EAE5F179B )

		virtual void OnEnter( float blendTime, float duration, const SProceduralParams& params )
	{
		m_context->SetBlendInTime( blendTime );
		m_context->UpdateProcClipLookingRequest( true );
	}

	virtual void OnExit( float blendTime )
	{
		m_context->SetBlendOutTime( blendTime );
		m_context->UpdateProcClipLookingRequest( false );
	}

	virtual void Update( float timePassed )
	{
		m_context->UpdateProcClipLookingRequest( true );
	}
};


CProceduralClipLooking::CProceduralClipLooking()
{
}

CProceduralClipLooking::~CProceduralClipLooking()
{
}

CRYREGISTER_CLASS( CProceduralClipLooking )
