// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "CommunicationVoiceTestManager.h"
#include "CommunicationHandler.h"
#include <IAIObject.h>
#include <IAIActorProxy.h>


void CommunicationVoiceTestManager::Reset()
{
	while (!m_playingActors.empty())
		Stop(m_playingActors.begin()->first);
}

void CommunicationVoiceTestManager::StartFor(EntityId actorID, const char* groupName)
{
	if (IEntity* entity = gEnv->pEntitySystem->GetEntity(actorID))
	{
		if (IAIObject* aiObject = entity->GetAI())
		{
			if (IAIActorProxy* proxy = aiObject->GetProxy())
			{
				CryLogAlways("Playing voice test for '%s'...", entity ? entity->GetName() : "<null>");

				std::pair<PlayingActors::iterator, bool> iresult = m_playingActors.insert(
					PlayingActors::value_type(actorID, PlayingActor()));

				if (!iresult.second)
					return;

				const ICVar* pCvar = gEnv->pConsole->GetCVar("ai_CommunicationForceTestVoicePack");
				const bool useTestVoicePack = pCvar ? pCvar->GetIVal() == 1 : false;

				PlayingActor& playingActor = iresult.first->second;

				VoiceLibraryID actorLibraryID = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryID(proxy->GetVoiceLibraryName(useTestVoicePack));
				const bool isVoiceLibraryExisting = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryName(actorLibraryID) != NULL;
				// If the library checked before doesn't exist maybe it means that the test voice pack doesn't exist so we need to try to return the normal voice library
				playingActor.libraryID = isVoiceLibraryExisting ? actorLibraryID : 
					CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryID(proxy->GetVoiceLibraryName());
					
				playingActor.onlyOne = groupName && *groupName;

				if (playingActor.onlyOne)
					playingActor.groupName = groupName;

				PlayNext(actorID);
			}
		}
	}
}

void CommunicationVoiceTestManager::Stop(EntityId actorID)
{
	REINST("is this class still needed?");
	/*PlayingActors::iterator it = m_playingActors.find(actorID);

	if (it != m_playingActors.end())
	{
		PlayingActor& playingActor = it->second;
		tSoundID soundID = playingActor.soundID;
		m_playingActors.erase(it);

		if (IEntity* entity = gEnv->pEntitySystem->GetEntity(actorID))
		{
			if (IAIObject* aiObject = entity->GetAI())
			{
				if (IAIActorProxy* proxy = aiObject->GetProxy())
				{
					proxy->GetCommunicationHandler()->StopVoice(playingActor.soundID);
					CryLogAlways("Cancelled voice test for '%s'...", entity ? entity->GetName() : "<null>");
				}
			}
		}
	}*/
}

void CommunicationVoiceTestManager::OnCommunicationHandlerEvent(
	IAICommunicationHandler::ECommunicationHandlerEvent type, CommPlayID playID, EntityId actorID)
{
	if ((type == IAICommunicationHandler::VoiceFinished) || (type == IAICommunicationHandler::VoiceFailed))
	{
		PlayingActors::iterator pit = m_playingActors.find(actorID);
		if (pit != m_playingActors.end())
		{
			PlayingActor& playingActor = pit->second;

			if (type == IAICommunicationHandler::VoiceFailed)
			{
				const char* libraryName = 
					CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryName(playingActor.libraryID);

				GameWarning("Failed to play voice test '%s' variation '%d' from '%s'...", 
					playingActor.groupName.c_str(), playingActor.variation - 1, libraryName ? libraryName : "<null>");

				++playingActor.failedCount;
			}

			//playingActor.soundID = INVALID_SOUNDID;

			if (playingActor.onlyOne)
			{
				Report(actorID, playingActor);
				m_playingActors.erase(pit);
			}
			else
				PlayNext(actorID);
		}
	}
	else if (type == IAICommunicationHandler::VoiceStarted)
	{
		PlayingActors::iterator pit = m_playingActors.find(actorID);
		if (pit != m_playingActors.end())
		{
			PlayingActor& playingActor = pit->second;

			const char* libraryName = 
				CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryName(playingActor.libraryID);

			CryLogAlways("Now playing voice '%s' variation '%d' from '%s'...", 
				playingActor.groupName.c_str(), playingActor.variation - 1, libraryName ? libraryName : "<null>");
		}
	}
}

void CommunicationVoiceTestManager::PlayNext(EntityId actorID)
{
	PlayingActors::iterator pit = m_playingActors.find(actorID);
	if (pit != m_playingActors.end())
	{
		PlayingActor& playingActor = pit->second;

		CommunicationVoiceLibrary* voiceLib = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary();

		while (true)
		{
			uint32 groupCount = voiceLib->GetGroupCount(playingActor.libraryID);
			if (playingActor.groupName.empty() && groupCount > 0)
			{
				const char* groupName = voiceLib->GetGroupName(playingActor.libraryID, 0);
				playingActor.groupName = groupName;
				playingActor.groupID = 0;
			}

			const char* voiceName;
			const char* path;

			if (!voiceLib->GetVoiceVariation(playingActor.libraryID, playingActor.groupName.c_str(), playingActor.variation++,
				path, voiceName))
			{
				if ((playingActor.groupID + 1 >= groupCount) || playingActor.onlyOne)
				{
					Report(actorID, playingActor);
					m_playingActors.erase(pit);

					return;
				}
				else
				{
					playingActor.variation = 0;
					++playingActor.groupID;
					
					playingActor.groupName = voiceLib->GetGroupName(playingActor.libraryID, playingActor.groupID);

					continue;
				}
			}

			++playingActor.totalCount;

			if (IEntity* entity = gEnv->pEntitySystem->GetEntity(actorID))
			{
				if (IAIObject* aiObject = entity->GetAI())
				{
					if (IAIActorProxy* proxy = aiObject->GetProxy())
					{
						stack_string completeName(path ? path : "");
						completeName.append(voiceName);

						CommunicationHandler* commHandler = static_cast<CommunicationHandler*>(proxy->GetCommunicationHandler());
						//playingActor.soundID = commHandler->PlaySound(CommPlayID(0), completeName.c_str(), CommunicationHandler::Voice, eLSM_MatchAnimationToSoundName, this);

						return;			
					}
				}
			}
		}
	}
}

void CommunicationVoiceTestManager::Report(EntityId actorID, const PlayingActor& playingActor)
{
	IEntity* entity = gEnv->pEntitySystem->GetEntity(actorID);
	const char* libraryName = 
		CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryName(playingActor.libraryID);

	CryLogAlways("Finished voice test for '%s' using '%s'...",
		entity ? entity->GetName() : "<null>",	libraryName ? libraryName : "<null>");
	CryLogAlways("Attempted: %d", playingActor.totalCount);
	CryLogAlways("Failed: %d", playingActor.failedCount);
}