////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include <IAgent.h>

#include "Player.h"
#include "PlayerAI.h"

class CProceduralClipCompromiseCover : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()
	
	CRYGENERATE_CLASS( CProceduralClipCompromiseCover, "CompromiseCover", 0x74D0CE3E8C8B4D56, 0xAACDBD49508B6186 )

protected:
	virtual void OnEnter( float blendTime, float duration, const SProceduralParams &params )
	{
		const EntityId actorEntityID = m_scope->GetEntityId();
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity( actorEntityID );
		IF_UNLIKELY( !pEntity )
			return;

		IAIObject* pAI = pEntity->GetAI();
		IF_UNLIKELY( !pAI )
			return;

		IPipeUser* pPipeUser = pAI->CastToIPipeUser();
		IF_UNLIKELY( !pPipeUser )
			return;

		CPlayer* pPlayer = static_cast<CPlayer*>( g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( actorEntityID ) );
		IF_UNLIKELY( !pPlayer )
			return;

		CAIAnimationComponent* pAIAnimationComponent = pPlayer->GetAIAnimationComponent();
		IF_UNLIKELY( !pAIAnimationComponent )
			return;

		pPipeUser->SetCoverCompromised();

		DoExplicitStanceChange( *pPlayer, *pAIAnimationComponent, STANCE_STAND );
	}

	virtual void Update( float timePassed )
	{
	}

	virtual void OnExit( float blendTime )
	{
	}

	void DoExplicitStanceChange( CPlayer &player, CAIAnimationComponent& pAIAnimationComponent, EStance targetStance )
	{
		if ( targetStance != STANCE_NULL )
		{
			player.SetStance( targetStance );

			CAIAnimationState& aiAnimationState = pAIAnimationComponent.GetAnimationState();
			aiAnimationState.SetRequestedStance( targetStance );
			aiAnimationState.SetStance( targetStance );
		}
	}
};


CProceduralClipCompromiseCover::CProceduralClipCompromiseCover()
{
}

CProceduralClipCompromiseCover::~CProceduralClipCompromiseCover()
{
}

CRYREGISTER_CLASS( CProceduralClipCompromiseCover )
