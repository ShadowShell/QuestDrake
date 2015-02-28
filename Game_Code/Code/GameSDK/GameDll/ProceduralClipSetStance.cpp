#include "StdAfx.h"
#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include <IAgent.h>
#include "Player.h"

class CProceduralClipSetStance : public TProceduralClip<SProceduralParams>
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD( IProceduralClip )
	CRYINTERFACE_END()
	
	CRYGENERATE_CLASS( CProceduralClipSetStance, "SetStance", 0x73B51332E8864D61, 0x80BBB244A6ECD268 )

protected:
	virtual void OnEnter( float blendTime, float duration, const SProceduralParams& params )
	{
		const EntityId entityId = m_scope->GetEntityId();
		IEntity* entity = gEnv->pEntitySystem->GetEntity( entityId );
		IF_UNLIKELY( !entity )
			return;

		IAIObject* aiObject = entity->GetAI();
		IF_UNLIKELY( !aiObject )
			return;

		CPlayer* player = static_cast<CPlayer*>( g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor( entityId ) );
		IF_UNLIKELY( !player )
			return;

		CAIAnimationComponent* aiAnimationComponent = player->GetAIAnimationComponent();
		IF_UNLIKELY( !aiAnimationComponent )
			return;

		const char *stanceName = params.dataString.c_str();
		IF_UNLIKELY( !stanceName )
			return;

		int stance = STANCE_NULL;
		for(; stance < STANCE_LAST; ++stance)
		{
			if ( strcmpi( stanceName, GetStanceName( (EStance)stance) ) == 0 )
				break;
		}

		IF_UNLIKELY( (EStance)stance == STANCE_LAST )
			return;

		aiAnimationComponent->ForceStanceTo( *player, (EStance)stance );
		aiAnimationComponent->ForceStanceInAIActorTo( *player, (EStance)stance );
	}

	virtual void Update( float timePassed )
	{
	}

	virtual void OnExit( float blendTime )
	{
	}
};

CProceduralClipSetStance::CProceduralClipSetStance()
{
}

CProceduralClipSetStance::~CProceduralClipSetStance()
{
}

CRYREGISTER_CLASS( CProceduralClipSetStance )
