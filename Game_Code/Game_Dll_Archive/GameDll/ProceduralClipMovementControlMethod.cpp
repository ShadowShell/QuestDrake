// (c) 2001 - 2012 Crytek GmbH.
#include "StdAfx.h"

#include "ICryMannequin.h"
#include <ICryAnimation.h>
#include <IAnimationPoseModifier.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "ProceduralContextMovementControlMethod.h"




struct SProceduralClipMovementControlMethodParams
	: public SProceduralParams
{
	float horizontal;
	float vertical;
};


namespace MovementControlMethodRemapping
{
	enum { SupportedMCM_Count = 4 };

	const EMovementControlMethod g_remappingTable[ SupportedMCM_Count ] =
	{
		eMCM_Undefined,
		eMCM_Entity,
		eMCM_Animation,
		eMCM_AnimationHCollision,
	};

	EMovementControlMethod GetFromFloatParam( const float param )
	{
		const int index = static_cast< int >( param );
		const EMovementControlMethod supportedMcm = ( 0 <= index && index < SupportedMCM_Count ) ? g_remappingTable[ index ] : eMCM_Undefined;
		return supportedMcm;
	}
}


class CProceduralClipMovementControlMethod
	: public TProceduralContextualClip< SProceduralClipMovementControlMethodParams, CProceduralContextMovementControlMethod >
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()

	CRYGENERATE_CLASS( CProceduralClipMovementControlMethod, "MovementControlMethod", 0x5583f04ebd654a89, 0x9679909839adad90 )


	virtual void OnEnter( float blendTime, float duration, const SProceduralClipMovementControlMethodParams& params )
	{
		const EMovementControlMethod horizontal = MovementControlMethodRemapping::GetFromFloatParam( params.horizontal );

		const EMovementControlMethod vertical = MovementControlMethodRemapping::GetFromFloatParam( params.vertical );
		const EMovementControlMethod validVertical = ( vertical == eMCM_AnimationHCollision ) ? eMCM_Animation : vertical;

		m_requestId = m_context->RequestMovementControlMethod( horizontal, validVertical );
	}

	virtual void OnExit( float blendTime )
	{
		m_context->CancelRequest( m_requestId );
	}

	virtual void Update( float timePassed )
	{
	}

private:
	uint32 m_requestId;
};


CProceduralClipMovementControlMethod::CProceduralClipMovementControlMethod()
	: m_requestId( 0 )
{
}

CProceduralClipMovementControlMethod::~CProceduralClipMovementControlMethod()
{
}

CRYREGISTER_CLASS( CProceduralClipMovementControlMethod )
