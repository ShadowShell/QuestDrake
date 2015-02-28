// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "MannequinDebug.h"
#include "ActionController.h"

#include "IAIObject.h"


//////////////////////////////////////////////////////////////////////////
static void MN_ReloadAll( IConsoleCmdArgs* pArgs )
{
	CCryAction::GetCryAction()->GetMannequinInterface().ReloadAll();
}

struct SAnimAssetContext
{
	XmlNodeRef								xmlRoot;
	const IAnimationDatabase *pDatabase;
};

void AssetCallback(const SAnimAssetReport &assetReport, void *_context)
{
	SAnimAssetContext *pContext = (SAnimAssetContext *)_context;
	XmlNodeRef xmlAnim = GetISystem()->CreateXmlNode("Anim");
	if (assetReport.pAnimName != NULL)
	{
		xmlAnim->setAttr("AnimID", assetReport.pAnimName);
	}
	if (assetReport.pAnimPath != NULL)
	{
		xmlAnim->setAttr("AnimPath", assetReport.pAnimPath);
	}
	pContext->xmlRoot->addChild(xmlAnim);
}

//////////////////////////////////////////////////////////////////////////
static void MN_ListAssets( IConsoleCmdArgs* pArgs )
{
	const char *filename = "mannequinAnimationAssetUsage.xml";

	if (pArgs->GetArgCount() >= 2)
	{
		filename = pArgs->GetArg(1);
	}

	SAnimAssetContext context;
	context.xmlRoot = GetISystem()->CreateXmlNode("UsedAnims");
	IAnimationDatabaseManager &dbManager = CCryAction::GetCryAction()->GetMannequinInterface().GetAnimationDatabaseManager();
	const int numDBAs = dbManager.GetTotalDatabases();
	for (int i=0; i<numDBAs; i++)
	{
		const IAnimationDatabase *pDatabase = dbManager.GetDatabase(i);
		context.pDatabase = pDatabase;
		pDatabase->EnumerateAnimAssets(NULL, AssetCallback, &context);
	}
	context.xmlRoot->saveToFile(filename);
}

//////////////////////////////////////////////////////////////////////////
static void MN_DebugAI( IConsoleCmdArgs* pArgs )
{
	const IEntity* pDebugEntity = NULL;


	if ( 2 <= pArgs->GetArgCount() )
	{
		const char* const entityName = pArgs->GetArg( 1 );
		pDebugEntity = gEnv->pEntitySystem->FindEntityByName( entityName );
	}
	else
	{
		const CCamera& viewCamera = gEnv->pSystem->GetViewCamera();
		const Matrix34& viewCameraMatrix = viewCamera.GetMatrix();
		const Vec3 cameraWorldPosition = viewCameraMatrix.GetTranslation();
		const Vec3 cameraWorldDirectionForward = viewCameraMatrix.GetColumn1().GetNormalized();

		float highestDotProduct = -2.0f;

		const CActionController::TActionControllerList& actionControllersList = CActionController::GetGlobalActionControllers();
		for ( size_t i = 0; i < actionControllersList.size(); ++i )
		{
			const CActionController* pActionController = actionControllersList[ i ];
			CRY_ASSERT( pActionController );

			const EntityId entityId = pActionController->GetEntityId();
			if ( entityId == 0 )
			{
				continue;
			}

			IEntity& entity = pActionController->GetEntity();
			const IAIObject* pAiObject = entity.GetAI();

			const Vec3 entityWorldPosition = pAiObject ? pAiObject->GetPos() : entity.GetWorldPos();
			const Vec3 cameraToEntityWorldDirection = ( entityWorldPosition - cameraWorldPosition ).GetNormalized();
			const float dotProduct = cameraWorldDirectionForward.Dot( cameraToEntityWorldDirection );

			if (( highestDotProduct < dotProduct ) && pAiObject)
			{
				pDebugEntity = &entity;
				highestDotProduct = dotProduct;
			}
		}
	}

	stack_string oldDebugEntityName = "";
	{
		const ICVar* const pMNDebugCVar = gEnv->pConsole->GetCVar( "mn_debug" );
		const char* const mnDebugEntityName = ( pMNDebugCVar ? pMNDebugCVar->GetString() : NULL );
		oldDebugEntityName = ( mnDebugEntityName ) ? mnDebugEntityName : "";
	}

	const bool wasEnabled = !( oldDebugEntityName.empty() );
	if ( wasEnabled )
	{
		// Turn Off

		gEnv->pConsole->ExecuteString( "i_forcefeedback 1" );
		gEnv->pConsole->ExecuteString( "c_shakeMult 1" );
		gEnv->pConsole->ExecuteString( "pl_health.minimalHudEffect 0");
		//gEnv->pConsole->ExecuteString( "r_PostProcessEffects 1"); <-- this crashes in certain levels

		gEnv->pConsole->ExecuteString( "ai_debugdraw 0" );
		gEnv->pConsole->ExecuteString( "ac_debugLocations 0" );
		gEnv->pConsole->ExecuteString( "ai_drawpathfollower 0" );
		gEnv->pConsole->ExecuteString( "ca_drawlocator 0" );

		gEnv->pConsole->ExecuteString( "ac_debugfilter 0" );

		gEnv->pConsole->ExecuteString( "ai_filteragentname \"\"" );

		{
			stack_string cmd;
			cmd.Format( "es_debugAnim %s 0", oldDebugEntityName.c_str() );
			gEnv->pConsole->ExecuteString( cmd.c_str() );
		}

		gEnv->pConsole->ExecuteString( "mn_debug 0" );
	}

	if ( !pDebugEntity )
		return;

	const bool sameDebugEntity = wasEnabled && !strcmp( oldDebugEntityName.c_str(), pDebugEntity->GetName() );
	if ( !sameDebugEntity )
	{
		// Turn On

		gEnv->pConsole->ExecuteString( "i_forcefeedback 0" );
		gEnv->pConsole->ExecuteString( "c_shakeMult 0" );
		gEnv->pConsole->ExecuteString( "pl_health.minimalHudEffect 1");
		//gEnv->pConsole->ExecuteString( "r_PostProcessEffects 0"); <-- disabled this as re-enabling it crashes in certain levels

		gEnv->pConsole->ExecuteString( "ai_debugdraw 1" );
		gEnv->pConsole->ExecuteString( "ac_debugLocations 2" );
		gEnv->pConsole->ExecuteString( "ai_drawpathfollower 1" );
		gEnv->pConsole->ExecuteString( "ca_drawlocator 1" );

		{
			stack_string cmd;
			cmd.Format( "ac_debugFilter %s", pDebugEntity->GetName() );
			gEnv->pConsole->ExecuteString( cmd.c_str() );
		}
		{
			stack_string cmd;
			cmd.Format( "ai_filteragentname %s", pDebugEntity->GetName() );
			gEnv->pConsole->ExecuteString( cmd.c_str() );
		}
		{
			stack_string cmd;
			cmd.Format( "es_debugAnim %s 1", pDebugEntity->GetName() );
			gEnv->pConsole->ExecuteString( cmd.c_str() );
		}
		{
			stack_string cmd;
			cmd.Format( "mn_debug %s", pDebugEntity->GetName() );
			gEnv->pConsole->ExecuteString( cmd.c_str() );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
void mannequin::debug::RegisterCommands()
{
	REGISTER_COMMAND( "mn_reload", MN_ReloadAll, VF_CHEAT, "Reloads animation databases" );
	REGISTER_COMMAND( "mn_listAssets", MN_ListAssets, VF_CHEAT, "Lists all the currently referenced animation assets" );
	REGISTER_COMMAND( "mn_DebugAI", MN_DebugAI, VF_CHEAT, "" );
}