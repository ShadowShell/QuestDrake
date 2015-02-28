// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CommunicationHandler_h__
#define __CommunicationHandler_h__


#pragma once


#include <IAgent.h>
#include "CommunicationVoiceLibrary.h"


class CommunicationHandler
	: public IAICommunicationHandler
	, public IAnimationGraphStateListener
{
	friend class CommunicationVoiceTestManager;
public:
	CommunicationHandler(CAIProxy& proxy, IEntity* entity);
	virtual ~CommunicationHandler();

	// IAICommunicationHandler
	/*virtual tSoundID PlaySound(CommPlayID playID, const char* name, IEventListener* listener = 0);
	virtual void StopSound(tSoundID soundID);

	virtual tSoundID PlayVoice(CommPlayID playID, const char* name, ELipSyncMethod lipSyncMethod, IEventListener* listener = 0);
	virtual void StopVoice(tSoundID soundID);*/

	virtual void SendDialogueRequest(CommPlayID playID, const char* name, IEventListener* listener = 0){}

	virtual void PlayAnimation(CommPlayID playID, const char* name, EAnimationMethod method, IEventListener* listener = 0);
	virtual void StopAnimation(CommPlayID playID, const char* name, EAnimationMethod method);

	virtual bool IsInAGState(const char* name);
	virtual void ResetAnimationState();

	virtual bool IsPlayingAnimation() const;
	virtual bool IsPlayingSound() const;

	//~IAICommunicationHandler

	// IAnimationgGraphStateListener
	virtual void SetOutput(const char* output, const char* value) {};
	virtual void QueryComplete( TAnimationGraphQueryID queryID, bool succeeded );
	virtual void DestroyedState(IAnimationGraphState*);
	//~IAnimationgGraphStateListener

	void Reset();
	void OnReused(IEntity* entity);

private:
	enum ESoundType
	{
		Sound = 0,
		Voice,
	};

	//tSoundID PlaySound(CommPlayID playID, const char* name, ESoundType type, ELipSyncMethod lipSyncMethod, IEventListener* listener = 0);
	IAnimationGraphState* GetAGState();

	CAIProxy& m_proxy;
	EntityId m_entityId;
	IAnimationGraphState* m_agState;

	struct PlayingSound
	{
		PlayingSound()
			: listener(0)
			, type(Sound)
			, playID(0)
		{
		}

		ESoundType type;
		IAICommunicationHandler::IEventListener* listener;
		//Index used to reference this event in listener. Set when sound event started
		CommPlayID playID;
	};

	struct PlayingAnimation
	{
		PlayingAnimation()
			: listener(0)
			, playing(false)
			, playID(0)
		{
		}

		IAICommunicationHandler::IEventListener* listener;
		string name;

		EAnimationMethod method;
		bool playing;
		//Index used to reference this event in listener. Set when animation event started
		CommPlayID playID;
	};

	typedef std::map<TAnimationGraphQueryID, PlayingAnimation> PlayingAnimations;
	PlayingAnimations m_playingAnimations;
	TAnimationGraphQueryID m_currentQueryID;	// because animation graph can send query result during SetInput, 
	bool m_currentPlaying;										// before we had chance to insert in the map

	/*typedef std::map<tSoundID, PlayingSound> PlayingSounds;
	PlayingSounds m_playingSounds;*/

	VoiceLibraryID m_voiceLibraryID;

	AnimationGraphInputID m_signalInputID;
	AnimationGraphInputID m_actionInputID;
};

#endif