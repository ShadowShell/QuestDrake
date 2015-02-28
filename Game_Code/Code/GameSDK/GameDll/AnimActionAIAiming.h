////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ANIM_ACTION_AI_AIMING_H__
#define __ANIM_ACTION_AI_AIMING_H__

#include "ICryMannequin.h"


class CAnimActionAIAiming
: public TAction< SAnimationContext >
{
public:
	typedef TAction< SAnimationContext > TBase;

	DEFINE_ACTION( "AnimActionAIAiming" );

	CAnimActionAIAiming();

	// IAction
	virtual void OnInitialise();
	virtual EStatus Update( float timePassed );
	// ~IAction

	static bool IsSupported( const SAnimationContext& context );

private:
	static FragmentID FindFragmentId( const SAnimationContext& context );
};


#endif