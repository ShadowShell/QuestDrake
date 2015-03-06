#ifndef __ANIM_ACTION_AI_STANCE__H__
#define __ANIM_ACTION_AI_STANCE__H__

#include "ICryMannequin.h"
#include "IAgent.h"

class CPlayer;

class CAnimActionAIStance
	: public TAction< SAnimationContext >
{
public:
	typedef TAction< SAnimationContext > TBase;

	DEFINE_ACTION( "AIStance" );

	CAnimActionAIStance( int priority, CPlayer* pPlayer, const EStance targetStance );

	virtual void Enter();
	virtual void Exit();
	virtual EStatus Update( float elapsedSeconds );
	virtual EStatus UpdatePending( float timePassed );


	virtual void OnInitialise();

	EStance GetTargetStance() const { return m_targetStance; }
	bool IsPlayerAnimationStanceSet() const { return m_isPlayerAnimationStanceSet; }

	ActionScopes FindStanceActionScopeMask( const SAnimationContext& context ) const;
	bool FragmentExistsInDatabase( const SAnimationContext& context, const IAnimationDatabase& database ) const;
private:
	void SetMovementParameters();
	void RestoreMovementParameters();

	void SetPlayerAnimationStanceOnce();
	EStance GetPlayerAnimationStance() const;

	bool FindFragmentInfo( const SAnimationContext& context, FragmentID& fragmentIdOut, SFragTagState& fragTagStateOut ) const;

private:
	CPlayer* const m_pPlayer;
	const EStance m_targetStance;
	bool m_isPlayerAnimationStanceSet;
};


#endif
