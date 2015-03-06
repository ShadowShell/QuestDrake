////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ANIM_ACTION_AI_LOOKING_H__
#define __ANIM_ACTION_AI_LOOKING_H__

#include "ICryMannequin.h"


class CAnimActionAILooking
	: public TAction< SAnimationContext >
{
public:
	typedef TAction< SAnimationContext > TBase;

	DEFINE_ACTION( "AnimActionAILooking" );

	CAnimActionAILooking();

	// IAction
	virtual void OnInitialise();
	virtual EStatus Update( float timePassed );
	// ~IAction

	static bool IsSupported( const SAnimationContext& context );

private:
	static FragmentID FindFragmentId( const SAnimationContext& context );
};


#endif