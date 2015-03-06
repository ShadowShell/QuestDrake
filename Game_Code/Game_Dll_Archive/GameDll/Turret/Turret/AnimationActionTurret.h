#ifndef __ANIMATION_ACTION_TURRET__H__
#define __ANIMATION_ACTION_TURRET__H__

#include <ICryMannequin.h>

class CAnimationActionTurret
	: public TAction< SAnimationContext >
{
public:
	typedef TAction< SAnimationContext > BaseClass;

	DEFINE_ACTION( "AnimationActionTurret" );

	CAnimationActionTurret( FragmentID fragmentId )
	 : TAction< SAnimationContext >( 0, fragmentId, TAG_STATE_EMPTY, IAction::NoAutoBlendOut )
	{
	}

	~CAnimationActionTurret()
	{
	}

	virtual EPriorityComparison ComparePriority( const IAction& currentAction ) const
	{
		assert( &currentAction != this );
		return Higher;
	}

	virtual EStatus Update( float elapsedSeconds )
	{
		const IScope& rootScope = GetRootScope();
		const bool foundBetterMatchingFragment = rootScope.IsDifferent( m_fragmentID, m_fragTags );
		if ( foundBetterMatchingFragment )
		{
			SetFragment( m_fragmentID, m_fragTags );
		}

		return BaseClass::Update( elapsedSeconds );
	}
};

#endif