#include "StdAfx.h"
#include "HUDEventWrapper.h"
#include "HUDEventDispatcher.h"

//////////////////////////////////////////////////////////////////////////

#include "HUDEventDispatcher.h"
#include "HUDUtils.h"
#include "Audio/GameAudio.h"

//////////////////////////////////////////////////////////////////////////

/*static*/ const SHUDEventWrapper::SMsgAudio  SHUDEventWrapper::kMsgAudioNULL;
/*static*/ CAudioSignalPlayer SHUDEventWrapper::s_unlockSound;
/*static*/ CAudioSignalPlayer SHUDEventWrapper::s_unlockSoundRare;

//////////////////////////////////////////////////////////////////////////

namespace SHUDEventWrapperUtils
{
	void InternalGenericMessageNotify(const EHUDEventType type, const char* msg, const bool loop, const SHUDEventWrapper::SMsgAudio& audio/*=kMsgAudioNULL*/)
	{
#if !defined(DEDICATED_SERVER)
        SHUDEvent newRoundMessage(type);
		newRoundMessage.AddData(msg);
		newRoundMessage.AddData(loop);
		newRoundMessage.AddData(&audio);
		CHUDEventDispatcher::CallEvent(newRoundMessage);
#endif
	}
}

/*static*/ void SHUDEventWrapper::RoundMessageNotify(const char* msg, const SHUDEventWrapper::SMsgAudio& audio/*=kMsgAudioNULL*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEventWrapperUtils::InternalGenericMessageNotify( eHUDEvent_OnRoundMessage, msg, false, audio);
#endif
}

// Do not localise your message before passing into this
// Double localisation can cause corruption
/*static*/ void SHUDEventWrapper::GameStateNotify( const char* msg, const SHUDEventWrapper::SMsgAudio& audio, const char* p1 /*=NULL*/, const char* p2 /*=NULL*/, const char* p3 /*=NULL*/, const char* p4 /*=NULL*/ )
{
#if !defined(DEDICATED_SERVER)
    string localisedString = CHUDUtils::LocalizeString(msg, p1, p2, p3, p4);	// cache the string to ensure no subsequent LocalizeString() calls could mess with our result
	SHUDEventWrapperUtils::InternalGenericMessageNotify( eHUDEvent_OnGameStateNotifyMessage, localisedString.c_str(), false, audio);
#endif
}
/*static*/ void SHUDEventWrapper::GameStateNotify( const char* msg, const char* p1 /*=NULL*/, const char* p2 /*=NULL*/, const char* p3 /*=NULL*/, const char* p4 /*=NULL*/ )
{
#if !defined(DEDICATED_SERVER)
    SHUDEventWrapper::GameStateNotify( msg, INVALID_AUDIOSIGNAL_ID, p1, p2, p3, p4);
#endif
}

/*static*/ void SHUDEventWrapper::DisplayLateJoinMessage()
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent displayLateJoinMessage(eHUDEvent_DisplayLateJoinMessage);
	CHUDEventDispatcher::CallEvent(displayLateJoinMessage);
#endif
}

/*static*/ void SHUDEventWrapper::TeamMessage( const char *msg, const int team, const SHUDEventWrapper::SMsgAudio& audio, const bool bShowTeamName, const bool bShowTeamIcon, const char * pCustomHeader, const float timeToHoldPauseState)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent newTeamMessage(eHUDEvent_OnTeamMessage);
	newTeamMessage.AddData(msg);
	newTeamMessage.AddData(team);
	newTeamMessage.AddData(&audio);
	newTeamMessage.AddData(bShowTeamName);
	newTeamMessage.AddData(bShowTeamIcon);
	newTeamMessage.AddData(pCustomHeader);
	newTeamMessage.AddData(timeToHoldPauseState);
	CHUDEventDispatcher::CallEvent(newTeamMessage);
#endif
}

/*static*/ void SHUDEventWrapper::SimpleBannerMessage( const char *msg, const SHUDEventWrapper::SMsgAudio& audio, const float timeToHoldPauseState)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent newBannerMessage(eHUDEvent_OnSimpleBannerMessage);
	newBannerMessage.AddData(msg);
	newBannerMessage.AddData(&audio);
	newBannerMessage.AddData(timeToHoldPauseState);
	CHUDEventDispatcher::CallEvent(newBannerMessage);
#endif
}

/*static*/ void SHUDEventWrapper::OnChatMessage( const EntityId entity, const int teamId, const char* message)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent chatMessageEvent(eHUDEvent_OnChatMessage);
	chatMessageEvent.AddData( static_cast<int>(entity) );
	chatMessageEvent.AddData( teamId );
	chatMessageEvent.AddData( message );
	CHUDEventDispatcher::CallEvent(chatMessageEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnAssessmentCompleteMessage( const char* groupName, const char* assesmentName, const char* description, const char* descriptionParam, const int xp )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent xpHudEvent(eHUDEvent_OnAssessmentComplete);
	xpHudEvent.AddData((const void*) groupName);		//Group name, e.g 'Scar Kills'
	xpHudEvent.AddData((const void*) assesmentName);	//Assessment name e.g 'Recruit'
	xpHudEvent.AddData((const void*) description);
	xpHudEvent.AddData((const void*) descriptionParam);
	xpHudEvent.AddData(xp);		//XP given
	CHUDEventDispatcher::CallEvent(xpHudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewMedalMessage( const char* name, const char* description, const char* descriptionParam )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent newMedalEvent(eHUDEvent_OnNewMedalAward);
	newMedalEvent.AddData((const void*) name);		     // title
	newMedalEvent.AddData((const void*) description); // description
	newMedalEvent.AddData((const void*) descriptionParam);
	CHUDEventDispatcher::CallEvent(newMedalEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnPromotionMessage( const char* rankName, const int rank, const int xpRequired )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent newPromotionEvent(eHUDEvent_OnPromotionMessage);
	newPromotionEvent.AddData((const void*) rankName);		     // rank name, will be localised with @pp_promoted.
	newPromotionEvent.AddData(rank);                           // rank id for icon.
	newPromotionEvent.AddData(xpRequired);
	CHUDEventDispatcher::CallEvent(newPromotionEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnSupportBonusXPMessage( const int xpType /*EPPType*/, const int xpGained )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent xpHudEvent(eHUDEvent_OnNewSkillKill);
	xpHudEvent.AddData(xpType);   // type info EPPType
	xpHudEvent.AddData(xpGained); // XP Points
	xpHudEvent.AddData(true);
	CHUDEventDispatcher::CallEvent(xpHudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnSkillKillMessage( const int xpType /*EPPType*/, const int xpGained )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent skillKillEvent(eHUDEvent_OnNewSkillKill);
	skillKillEvent.AddData(xpType);   // type info EPPType
	skillKillEvent.AddData(xpGained); // XP Points
	skillKillEvent.AddData(false);
	CHUDEventDispatcher::CallEvent(skillKillEvent);
#endif
}

// Set Message to NULL to clear.
/*static*/ void SHUDEventWrapper::OnGameStatusUpdate( const EGoodBadNeutralForLocalPlayer good, const char* message )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent gameSatusMessageEvent(eHUDEvent_OnSetGameStateMessage);
	gameSatusMessageEvent.AddData((int)good);         // for colours/anims
	gameSatusMessageEvent.AddData((void*)message);    // XP Points
	CHUDEventDispatcher::CallEvent(gameSatusMessageEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnRoundEnd( const int winner, const bool clientScoreIsTop, const char* victoryDescMessage, const char* victoryMessage, const TAudioSignalID roundEndMusicSignal, const EAnnouncementID announcement )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent endRoundMessage(eHUDEvent_OnRoundEnd);
	endRoundMessage.AddData( SHUDEventData(winner) );
	endRoundMessage.AddData( SHUDEventData(clientScoreIsTop) );
	endRoundMessage.AddData( SHUDEventData(victoryDescMessage) );
	endRoundMessage.AddData( SHUDEventData(victoryMessage) );
	endRoundMessage.AddData( SHUDEventData((int) roundEndMusicSignal) );
	endRoundMessage.AddData( SHUDEventData((int) announcement) );
	CHUDEventDispatcher::CallEvent(endRoundMessage);
#endif
}

/*static*/ void SHUDEventWrapper::OnSuddenDeath()
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent suddenDeathMessage(eHUDEvent_OnSuddenDeath);
	CHUDEventDispatcher::CallEvent(suddenDeathMessage);
#endif
}

/*static*/ void SHUDEventWrapper::OnGameEnd(const int teamOrPlayerId, const bool clientScoreIsTop, EGameOverReason reason, const char* message, ESVC_DrawResolution drawResolution, const TAudioSignalID musicSignal, const EAnnouncementID announcement)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent endGameMessage(eHUDEvent_GameEnded);
	endGameMessage.AddData(SHUDEventData(teamOrPlayerId));
	endGameMessage.AddData(SHUDEventData(clientScoreIsTop));
	endGameMessage.AddData(SHUDEventData((int)reason));
	endGameMessage.AddData(SHUDEventData(message));
	endGameMessage.AddData(SHUDEventData((int) drawResolution));
	endGameMessage.AddData(SHUDEventData((int) musicSignal));
	endGameMessage.AddData(SHUDEventData((int) announcement));
	CHUDEventDispatcher::CallEvent(endGameMessage);
#endif
}

/*static*/ void SHUDEventWrapper::UpdateGameStartCountdown(const EPreGameCountdownType countdownType, const float timeTillStartInSeconds)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent prematchEvent;
	prematchEvent.eventType = eHUDEvent_OnUpdateGameStartMessage;
	prematchEvent.AddData(timeTillStartInSeconds);
	prematchEvent.AddData((int)countdownType);
	CHUDEventDispatcher::CallEvent(prematchEvent);
#endif
}

/*static*/ void SHUDEventWrapper::CantFire( void )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent cantfire( eHUDEvent_CantFire );
	CHUDEventDispatcher::CallEvent(cantfire);
#endif
}

/*static*/ void SHUDEventWrapper::FireModeChanged( IWeapon* pWeapon, const int currentFireMode, bool bForceFireModeUpdate /* = false */ )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_OnFireModeChanged);
	event.AddData(SHUDEventData((void*)pWeapon));
	event.AddData(SHUDEventData(currentFireMode));
	event.AddData(SHUDEventData(bForceFireModeUpdate));

	CHUDEventDispatcher::CallEvent(event);
#endif
}

/*static*/ void SHUDEventWrapper::ForceCrosshairType( IWeapon* pWeapon, const ECrosshairTypes desiredCrosshairType )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_ForceCrosshairType);
	event.AddData(SHUDEventData((void*)pWeapon));
	event.AddData(SHUDEventData(desiredCrosshairType));

	CHUDEventDispatcher::CallEvent(event);
#endif
}

/*static*/ void SHUDEventWrapper::OnInteractionUseHoldTrack(const bool bTrackingUse)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent interactionEvent(eHUDEvent_OnInteractionUseHoldTrack);
	interactionEvent.AddData(SHUDEventData(bTrackingUse));
	CHUDEventDispatcher::CallEvent(interactionEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnInteractionUseHoldActivated(const bool bSuccessful)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent interactionEvent(eHUDEvent_OnInteractionUseHoldActivated);
	interactionEvent.AddData(SHUDEventData(bSuccessful));
	CHUDEventDispatcher::CallEvent(interactionEvent);
#endif
}

/*static*/ void SHUDEventWrapper::InteractionRequest( const bool activate, const char* msg, const char* action, const char* actionmap, const float duration, const bool bGameRulesRequest, const bool bIsFlowNodeReq, const bool bShouldSerialize )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent interactionEvent(eHUDEvent_OnInteractionRequest);
	interactionEvent.AddData(SHUDEventData(activate));
	interactionEvent.AddData(SHUDEventData(msg));
	interactionEvent.AddData(SHUDEventData(action));
	interactionEvent.AddData(SHUDEventData(actionmap));
	interactionEvent.AddData(SHUDEventData(duration));
	interactionEvent.AddData(SHUDEventData(bGameRulesRequest));
	interactionEvent.AddData(SHUDEventData(bIsFlowNodeReq));
	interactionEvent.AddData(SHUDEventData(bShouldSerialize));
	CHUDEventDispatcher::CallEvent(interactionEvent);
#endif
}

/*static*/ void SHUDEventWrapper::ClearInteractionRequest( const bool bGameRulesRequest ) // TODO: Remove this and all uses of it, should use ClearInteractionMsg
{
#if !defined(DEDICATED_SERVER)
	SHUDEventWrapper::InteractionRequest( false, NULL, NULL, NULL, -1.0f, bGameRulesRequest );
#endif
}

/*static*/ void SHUDEventWrapper::SetInteractionMsg(const EHUDInteractionMsgType interactionType, 
																										const char* message, 
																										const float duration, 
																										const bool bShouldSerialize, 
																										const char* interactionAction/*= NULL*/,
																										const char* interactionActionmap/* = NULL*/,
																										const EntityId usableEntityId/* = 0*/,
																										const EntityId usableSwapEntityId/* = 0*/,
																										const EInteractionType usableInteractionType/* = eInteraction_None*/,
																										const char* szExtraParam1/* = NULL*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_OnSetInteractionMsg);
	event.AddData(SHUDEventData((const int)interactionType));
	event.AddData(SHUDEventData(message));
	event.AddData(SHUDEventData(duration));
	event.AddData(SHUDEventData(bShouldSerialize));
	event.AddData(SHUDEventData(interactionAction));
	event.AddData(SHUDEventData(interactionActionmap));
	event.AddData(SHUDEventData((const int)usableEntityId));
	event.AddData(SHUDEventData((const int)usableSwapEntityId));
	event.AddData(SHUDEventData((const int)usableInteractionType));
	event.AddData(SHUDEventData(szExtraParam1));
	
	CHUDEventDispatcher::CallEvent(event);	
#endif
}

/*static*/ void SHUDEventWrapper::ClearInteractionMsg(EHUDInteractionMsgType interactionType, const char* onlyRemoveMsg ,const float fadeOutTime /*= -1.0f*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_OnClearInteractionMsg);
	event.AddData(SHUDEventData((int)interactionType));
	event.AddData(SHUDEventData(onlyRemoveMsg));
	event.AddData(SHUDEventData(fadeOutTime));
	CHUDEventDispatcher::CallEvent(event);	
#endif
}

/*static*/ void SHUDEventWrapper::DisplayInfo( const int system, const float time, const char* msg, const char* param1/*=NULL*/, const char* param2/*=NULL*/, const char* param3/*=NULL*/, const char* param4/*=NULL*/, const EInfoSystemPriority priority/*=eInfoPriority_Low*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_InfoSystemsEvent);
	event.AddData(SHUDEventData(system));
	event.AddData(SHUDEventData(time));
	event.AddData(SHUDEventData(msg));
	event.AddData(SHUDEventData(param1));
	event.AddData(SHUDEventData(param2));
	event.AddData(SHUDEventData(param3));
	event.AddData(SHUDEventData(param4));
	event.AddData(SHUDEventData((int)priority));
	CHUDEventDispatcher::CallEvent(event);
#endif
}

/*static*/ void SHUDEventWrapper::ClearDisplayInfo( const int system, const EInfoSystemPriority priority/*=eInfoPriority_Low*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEventWrapper::DisplayInfo( system, 0.0f, NULL, NULL, NULL, NULL, NULL, priority);
#endif
}

/*static*/ void SHUDEventWrapper::GenericBattleLogMessage(const EntityId actorId, const char *message, const char* p1 /*=NULL*/, const char* p2 /*=NULL*/, const char* p3 /*=NULL*/, const char* p4 /*=NULL*/ )
{
#if !defined(DEDICATED_SERVER)
	string localisedString = CHUDUtils::LocalizeString(message, p1, p2, p3, p4);	// cache the string to ensure no subsequent LocalizeString() calls could mess with our result
	
	SHUDEvent genericMessageEvent(eHUDEvent_OnGenericBattleLogMessage);
	genericMessageEvent.AddData(static_cast<int>(actorId));
	genericMessageEvent.AddData(localisedString.c_str());
	CHUDEventDispatcher::CallEvent(genericMessageEvent);
#endif
}

/*static*/ void SHUDEventWrapper::HitTarget( EGameRulesTargetType targetType, int bulletType, EntityId targetId )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnHitTarget);
	hudEvent.AddData( static_cast<int>(targetType) );
	hudEvent.AddData( bulletType );
	hudEvent.AddData( static_cast<int>(targetId) );
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnGameStateMessage( const EntityId actorId, const bool bIsFriendly, const char* pMessage, EBL_IconFrames icon)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewGameStateMessage);
	hudEvent.AddData(static_cast<int>(actorId));
	hudEvent.AddData(bIsFriendly);
	hudEvent.AddData(static_cast<const void*>(pMessage));
	hudEvent.AddData(static_cast<int>(icon));
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewObjective( const EntityId entityId, const EGameRulesMissionObjectives iconType )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( static_cast<int>(iconType) );
	CRY_ASSERT(hudEvent.GetDataSize() != 7);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewObjective( const EntityId entityId, const EGameRulesMissionObjectives iconType, const float progress, const int priority )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( static_cast<int>(iconType) );
	hudEvent.AddData( progress ); 
	hudEvent.AddData( priority );
	CRY_ASSERT(hudEvent.GetDataSize() != 7);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewObjective( const EntityId entityId, const EGameRulesMissionObjectives iconType, const float progress, const int priority, const char *pNameOverride, const char *pColourStr )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( static_cast<int>(iconType) );
	hudEvent.AddData( progress ); 
	hudEvent.AddData( priority );
	hudEvent.AddData( pNameOverride );
	hudEvent.AddData( pColourStr );
	CRY_ASSERT(hudEvent.GetDataSize() != 7);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewObjectiveWithRadarEntity( const EntityId entityId, const EntityId radarEntityId, const EGameRulesMissionObjectives iconType, const float progress, const int priority, const char *pNameOverride, const char *pColourStr )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( static_cast<int>(iconType) );
	hudEvent.AddData( progress ); 
	hudEvent.AddData( priority );
	hudEvent.AddData( pNameOverride );
	hudEvent.AddData( pColourStr );
	hudEvent.AddData( static_cast<int>(radarEntityId) );
	CRY_ASSERT(hudEvent.GetDataSize() == 7);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnRemoveObjective( const EntityId entityId )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnRemoveObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	CRY_ASSERT(hudEvent.GetDataSize() != 3);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnRemoveObjective( const EntityId entityId, const int priority )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnRemoveObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( priority );
	CRY_ASSERT(hudEvent.GetDataSize() != 3);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnRemoveObjectiveWithRadarEntity( const EntityId entityId, const EntityId radarEntityId, const int priority )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnRemoveObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	hudEvent.AddData( priority );
	hudEvent.AddData( static_cast<int>(radarEntityId) );
	CRY_ASSERT(hudEvent.GetDataSize() == 3);	//The HUD_Radar will need updating if this changes
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnNewGameRulesObjective( const EntityId entityId )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_OnNewGameRulesObjective);
	hudEvent.AddData( static_cast<int>(entityId) );
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::OnBootAction(const uint32 bootAction, const char* elementName)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent hudEvent(eHUDEvent_HUDBoot);
	hudEvent.AddData(static_cast<int>(bootAction));
	if (elementName && elementName[0])
	{
		hudEvent.AddData((void*)elementName);
	}
	CHUDEventDispatcher::CallEvent(hudEvent);
#endif
}

/*static*/ void SHUDEventWrapper::ActivateOverlay(const int type)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_ActivateOverlay);
	event.AddData(SHUDEventData(type));
	CHUDEventDispatcher::CallEvent(event);
#endif
}

/*static*/ void SHUDEventWrapper::DeactivateOverlay(const int type)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent event(eHUDEvent_DeactivateOverlay);
	event.AddData(SHUDEventData(type));
	CHUDEventDispatcher::CallEvent(event);
#endif
}

/*static*/ void SHUDEventWrapper::OnPrepareItemSelected(const EntityId itemId, const EItemCategoryType category, const EWeaponSwitchSpecialParam specialParam )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent eventPrepareItemSelected(eHUDEvent_OnPrepareItemSelected);
	eventPrepareItemSelected.AddData(SHUDEventData((int)itemId));
	eventPrepareItemSelected.AddData(SHUDEventData((int)category));
	eventPrepareItemSelected.AddData(SHUDEventData((int)specialParam));
	CHUDEventDispatcher::CallEvent(eventPrepareItemSelected);
#endif
}

/*static*/ void SHUDEventWrapper::PlayerRename(const EntityId playerId, const bool isReplay /*= false*/)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent renameEvent(eHUDEvent_RenamePlayer);
	renameEvent.AddData(SHUDEventData((int)playerId));
	renameEvent.AddData(SHUDEventData(isReplay));
	CHUDEventDispatcher::CallEvent(renameEvent);	
#endif	
}

/*static*/ void SHUDEventWrapper::OnPlayerLinkedToVehicle( const EntityId playerId, const EntityId vehicleId, const bool alwaysShowHealth )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent linkEvent(eHUDEvent_PlayerLinkedToVehicle);
	linkEvent.AddData(SHUDEventData((int)playerId));
	linkEvent.AddData(SHUDEventData((int)vehicleId));
	linkEvent.AddData(SHUDEventData(alwaysShowHealth));
	CHUDEventDispatcher::CallEvent(linkEvent);		
#endif
}

/*static*/ void SHUDEventWrapper::OnPlayerUnlinkedFromVehicle( const EntityId playerId, const EntityId vehicleId, const bool keepHealthBar )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent linkEvent(eHUDEvent_PlayerUnlinkedFromVehicle);
	linkEvent.AddData(SHUDEventData((int)playerId));
	linkEvent.AddData(SHUDEventData((int)vehicleId));
	linkEvent.AddData(SHUDEventData(keepHealthBar));
	CHUDEventDispatcher::CallEvent(linkEvent);
#endif	
}

/*static*/ void SHUDEventWrapper::OnStartTrackFakePlayerTagname( const EntityId tagEntityId, const int sessionNameIdxToTrack )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent linkEvent(eHUDEvent_OnStartTrackFakePlayerTagname);
	linkEvent.AddData(SHUDEventData((int)tagEntityId));
	linkEvent.AddData(SHUDEventData((int)sessionNameIdxToTrack));
	CHUDEventDispatcher::CallEvent(linkEvent);
#endif		
}

/*static*/ void SHUDEventWrapper::OnStopTrackFakePlayerTagname( const EntityId tagEntityId )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent linkEvent(eHUDEvent_OnEndTrackFakePlayerTagname);
	linkEvent.AddData(SHUDEventData((int)tagEntityId));
	CHUDEventDispatcher::CallEvent(linkEvent);
#endif
}

/*static*/ void SHUDEventWrapper::PowerStruggleNodeStateChange(const int activeNodeIdentityId, const int nodeHUDState)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent nodeStateChange(eHUDEvent_OnPowerStruggle_NodeStateChange);
	nodeStateChange.AddData(SHUDEventData(activeNodeIdentityId));
	nodeStateChange.AddData(SHUDEventData(nodeHUDState));
	CHUDEventDispatcher::CallEvent(nodeStateChange);
#endif
}

/*static*/ void SHUDEventWrapper::PowerStruggleManageCaptureBar(EHUDPowerStruggleCaptureBarType inType, float inChargeAmount, bool inContention, const char *inBarString)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent manageCaptureBar(eHUDEvent_OnPowerStruggle_ManageCaptureBar);
	manageCaptureBar.AddData(SHUDEventData(static_cast<int>(inType)));
	manageCaptureBar.AddData(SHUDEventData(inChargeAmount));
	manageCaptureBar.AddData(SHUDEventData(inContention));
	manageCaptureBar.AddData(SHUDEventData(inBarString));
	CHUDEventDispatcher::CallEvent(manageCaptureBar);
#endif
}

/*static*/ void SHUDEventWrapper::OnBigMessage(const char *inSubTitle, const char *inTitle)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent bigMessage(eHUDEvent_OnBigMessage);
	bigMessage.AddData(inSubTitle);
	bigMessage.AddData(inTitle);
	CHUDEventDispatcher::CallEvent(bigMessage);
#endif
}

/*static*/ void SHUDEventWrapper::OnBigWarningMessage(const char *line1, const char *line2, const float duration)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent bigWarningMessage(eHUDEvent_OnBigWarningMessage);
	bigWarningMessage.AddData(line1);
	bigWarningMessage.AddData(line2);
	bigWarningMessage.AddData(duration);
	CHUDEventDispatcher::CallEvent(bigWarningMessage);
#endif
}

/*static*/ void SHUDEventWrapper::OnBigWarningMessageUnlocalized(const char *line1, const char *line2, const float duration)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent bigWarningMessage(eHUDEvent_OnBigWarningMessageUnlocalized);
	bigWarningMessage.AddData(line1);
	bigWarningMessage.AddData(line2);
	bigWarningMessage.AddData(duration);
	CHUDEventDispatcher::CallEvent(bigWarningMessage);
#endif
}

/*static*/ void SHUDEventWrapper::UpdatedDirectionIndicator(const EntityId &id)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent updatedDirectionIndicator(eHUDEvent_OnShowHitIndicatorBothUpdated);
	updatedDirectionIndicator.AddData((int)id);
	CHUDEventDispatcher::CallEvent(updatedDirectionIndicator);
#endif
}

/*static*/ void SHUDEventWrapper::SetStaticTimeLimit(bool active, int time)
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent staticTimeLimitMessage(eHUDEvent_OnSetStaticTimeLimit);
	staticTimeLimitMessage.AddData(active);
	staticTimeLimitMessage.AddData(time);
	CHUDEventDispatcher::CallEvent(staticTimeLimitMessage);
#endif
}

/*static*/ void SHUDEventWrapper::PlayUnlockSound()
{
#if !defined(DEDICATED_SERVER)
	if(!s_unlockSound.HasValidSignal())
	{
		s_unlockSound.SetSignal("HUD_CollectiblePickUp");
	}
	REINST("needs verification!");
	//s_unlockSound.Play();
#endif
}

/*static*/ void SHUDEventWrapper::PlayUnlockSoundRare()
{
#if !defined(DEDICATED_SERVER)
	if(!s_unlockSoundRare.HasValidSignal())
	{
		s_unlockSoundRare.SetSignal("HUD_CollectiblePickUpRare");
	}
	REINST("needs verification!");
	//s_unlockSoundRare.Play();
#endif
}

/*static*/ void SHUDEventWrapper::DisplayWeaponUnlockMsg(const char* szCollectibleId)
{
	SHUDEventWrapper::PlayUnlockSound();
}

/*static*/ void SHUDEventWrapper::RadarSweepActivated( int targetTeam, float activateDuration )
{
#if !defined(DEDICATED_SERVER)
	SHUDEvent radarSweepMessage(eHUDEvent_OnRadarSweep);
	radarSweepMessage.AddData(targetTeam);
	radarSweepMessage.AddData(activateDuration);
	CHUDEventDispatcher::CallEvent(radarSweepMessage);
#endif
}
