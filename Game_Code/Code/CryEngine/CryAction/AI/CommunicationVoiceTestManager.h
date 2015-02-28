// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CommunicationVoiceTestManager_h__
#define __CommunicationVoiceTestManager_h__

#pragma once


#include <IAgent.h>
#include "CommunicationVoiceLibrary.h"


class CommunicationVoiceTestManager
	: public IAICommunicationHandler::IEventListener
{
public:
	void Reset();

	void StartFor(EntityId actorID, const char* groupName);
	void Stop(EntityId actorID);

private:
	// IAICommunicationHandler::IEventListener
	virtual void OnCommunicationHandlerEvent(IAICommunicationHandler::ECommunicationHandlerEvent type, CommPlayID playID, EntityId actorID);
	//~IAICommunicationHandler::IEventListener

	struct PlayingActor
	{
		PlayingActor()
			: libraryID(0)
			, groupID(0)
			, variation(0)
			, failedCount(0)
			, totalCount(0)
			, onlyOne(false)
		{
		}

		VoiceLibraryID libraryID;
		uint32 groupID;
		uint32 variation;
		
		uint32 failedCount;
		uint32 totalCount;
		bool onlyOne;

		string groupName;
	};

	void PlayNext(EntityId actorID);
	void Report(EntityId actorID, const PlayingActor& playingActor);

	typedef std::map<tAIObjectID, PlayingActor> PlayingActors;
	PlayingActors m_playingActors;
};


#endif //__CommunicationTestManager_h__