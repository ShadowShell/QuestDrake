////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ANIM_ACTION_AI_AIM_POSE_H__
#define __ANIM_ACTION_AI_AIM_POSE_H__

#include "ICryMannequin.h"


class CAnimActionAIAimPose
: public TAction< SAnimationContext >
{
public:
	typedef TAction< SAnimationContext > TBase;

	DEFINE_ACTION( "AnimActionAIAimPose" );

	CAnimActionAIAimPose();

	// IAction
	virtual void OnInitialise();
	virtual EStatus Update( float timePassed );
	virtual void Install();
	// ~IAction

	static bool IsSupported( const SAnimationContext& context );

private:
	void InitialiseAimPoseBlender();
	static FragmentID FindFragmentId( const SAnimationContext& context );
};


#endif