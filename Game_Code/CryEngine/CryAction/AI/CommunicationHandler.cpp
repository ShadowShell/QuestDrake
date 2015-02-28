// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "CommunicationHandler.h"
#include "CommunicationVoiceLibrary.h"
#include "AIProxy.h"


CommunicationHandler::CommunicationHandler(CAIProxy& proxy, IEntity* entity)
: m_proxy(proxy)
, m_entityId(entity->GetId())
, m_agState(0)
, m_currentQueryID(0)
, m_currentPlaying(0)
, m_signalInputID(0)
, m_actionInputID(0)
, m_voiceLibraryID(0)
{
	assert(entity);
	Reset();
}

CommunicationHandler::~CommunicationHandler()
{
	if (m_agState)
		m_agState->RemoveListener(this);

	/*PlayingSounds::iterator it = m_playingSounds.begin();
	PlayingSounds::iterator end = m_playingSounds.end();

	for ( ; it != end; ++it)
	{
		if (ISound* sound = gEnv->pAudioSystem->GetSound(it->first))
			sound->RemoveEventListener(this);
	}*/
}

void CommunicationHandler::Reset()
{
	REINST("trigger voice events");
	//CryLogAlways("CommunicationHandler::Reset()");
	// Notify sound/voice listeners that the sounds have stopped
	//{
	//	PlayingSounds::iterator end = m_playingSounds.end();

	//	for (PlayingSounds::iterator it(m_playingSounds.begin()); it != end; ++it)
	//	{
	//		// important to remove the listener before clearing all Sound-IDs
	//		if (ISound* sound = gEnv->pAudioSystem->GetSound(it->first))
	//			sound->RemoveEventListener(this);

	//		PlayingSound& playingSound(it->second);

	//		if (playingSound.listener)
	//		{
	//			ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
	//			playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID, m_entityId);
	//		}
	//	}
	//	
	//	m_playingSounds.clear();
	//}

	// Notify animation listeners that the animations have stopped
	{
		PlayingAnimations::iterator end = m_playingAnimations.end();

		for (PlayingAnimations::iterator it(m_playingAnimations.begin()); it != end; ++it)
		{
			PlayingAnimation& playingAnim(it->second);

			if (playingAnim.listener)
			{
				ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodAction) ? ActionCancelled : SignalCancelled;
				playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
			}
		}
		
		m_playingAnimations.clear();
	}

	const ICVar* pCvar = gEnv->pConsole->GetCVar("ai_CommunicationForceTestVoicePack");
	const bool useTestVoicePack = pCvar ? pCvar->GetIVal() == 1 : false;

	const VoiceLibraryID voiceLibraryID = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryID(m_proxy.GetVoiceLibraryName(useTestVoicePack));
	const bool isVoiceLibraryExisting = CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryName(voiceLibraryID) != NULL;
	// If the library checked before doesn't exist maybe it means that the test voice pack doesn't exist so we need to try to return the normal voice library
	m_voiceLibraryID = ( isVoiceLibraryExisting ) ? voiceLibraryID : CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoiceLibraryID(m_proxy.GetVoiceLibraryName());
}

bool CommunicationHandler::IsInAGState(const char* name)
{
	if (GetAGState())
	{
		char inputValue[256] = "";
		m_agState->GetInput(m_signalInputID, inputValue);

		if(strcmp(inputValue, name) == 0 )
			return true;

		m_agState->GetInput(m_actionInputID, inputValue);

		if(strcmp(inputValue, name) == 0 )
			return true;

	}

	return false;
}

void CommunicationHandler::ResetAnimationState()
{
	if (GetAGState())
	{
		m_agState->SetInput(m_signalInputID,"none");
		m_agState->SetInput(m_actionInputID, "idle");

		m_agState->Update();
		m_agState->ForceTeleportToQueriedState();
	}
}

void CommunicationHandler::OnReused(IEntity* entity)
{
	assert(entity);
	m_entityId = entity->GetId();
	Reset();
}

//tSoundID CommunicationHandler::PlaySound(CommPlayID playID, const char* name, IEventListener* listener)
//{
//	return PlaySound(playID, name, Sound, eLSM_None, listener);
//}
//
//void CommunicationHandler::StopSound(tSoundID soundID)
//{
//	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
//	if(pEntity)
//	{
//		IEntityAudioProxy* pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
//		if (pIEntityAudioProxy)
//			pIEntityAudioProxy->StopSound(soundID);
//
//		PlayingSounds::iterator it = m_playingSounds.find(soundID);
//		if (it != m_playingSounds.end())
//		{
//			if (ISound* pSound = gEnv->pAudioSystem->GetSound(it->first))
//				pSound->RemoveEventListener(this);
//
//			PlayingSound& playingSound = it->second;
//			if (playingSound.listener)
//			{
//				ECommunicationHandlerEvent cancelEvent = (playingSound.type == Sound) ? SoundCancelled : VoiceCancelled;
//				playingSound.listener->OnCommunicationHandlerEvent(cancelEvent, playingSound.playID,  m_entityId);
//			}
//
//			m_playingSounds.erase(it);
//		}
//	}
//}
//
//tSoundID CommunicationHandler::PlayVoice(CommPlayID playID, const char* name, ELipSyncMethod lipSyncMethod, IEventListener* listener)
//{
//	const char* path = 0;
//	const char* fileName = 0;
//	
//	if (!CCryAction::GetCryAction()->GetCommunicationVoiceLibrary()->GetVoice(m_voiceLibraryID, name, path, fileName))
//	{
//		if (listener)
//			listener->OnCommunicationHandlerEvent(VoiceFailed, playID, m_entityId);
//
//		return INVALID_SOUNDID;
//	}
//
//	stack_string completeName(path ? path : "");
//	completeName.append(fileName);
//
//	return PlaySound(playID, completeName.c_str(), Voice, lipSyncMethod, listener);
//}
//
//void CommunicationHandler::StopVoice(tSoundID soundID)
//{
//	StopSound(soundID);
//}

void CommunicationHandler::PlayAnimation(CommPlayID playID, const char* name, EAnimationMethod method, IEventListener* listener)
{
	bool ok = false;

	if (GetAGState())
	{
		AnimationGraphInputID inputID = method == AnimationMethodAction ? m_actionInputID : m_signalInputID;

		if (ok = m_agState->SetInput(inputID, name, &m_currentQueryID))
		{
			//Force the animation graph to update to the new signal, to provide quicker communication responsiveness
			m_agState->Update();
			m_agState->ForceTeleportToQueriedState();
			std::pair<PlayingAnimations::iterator, bool> iresult = m_playingAnimations.insert(
				PlayingAnimations::value_type(m_currentQueryID, PlayingAnimation()));

			PlayingAnimation& playingAnimation = iresult.first->second;
			playingAnimation.listener = listener;
			playingAnimation.name = name;
			playingAnimation.method = method;
			playingAnimation.playing = m_currentPlaying;
			playingAnimation.playID = playID;
		}
		m_currentPlaying = false;
		m_currentQueryID = 0;
	}

	if (!ok && listener)
		listener->OnCommunicationHandlerEvent(
		(method == AnimationMethodAction) ? ActionFailed : SignalFailed, playID, m_entityId);
}

void CommunicationHandler::StopAnimation(CommPlayID playID, const char* name, EAnimationMethod method)
{
	if (GetAGState())
	{
		if(method == AnimationMethodSignal)
		{
			m_agState->SetInput(m_signalInputID,"none");
		}
		else
		{
			m_agState->SetInput(m_actionInputID, "idle");
		}
		m_agState->Update();
		m_agState->ForceTeleportToQueriedState();
	}

	PlayingAnimations::iterator it = m_playingAnimations.begin();
	PlayingAnimations::iterator end = m_playingAnimations.end();

	for ( ; it != end;)
	{
		PlayingAnimation& playingAnim = it->second;
		
		bool animMatch = playID ? playingAnim.playID == playID : strcmp(playingAnim.name, name) == 0;

		if (animMatch)
		{
			if (playingAnim.listener)
			{
				ECommunicationHandlerEvent cancelEvent = (playingAnim.method == AnimationMethodSignal) ? SignalCancelled : ActionCancelled;
				playingAnim.listener->OnCommunicationHandlerEvent(cancelEvent, playingAnim.playID, m_entityId);
			}

			m_playingAnimations.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

//void CommunicationHandler::OnSoundEvent(ESoundCallbackEvent event, ISound* sound)
//{
//	bool removeSound = false;
//
//	switch (event)
//	{
//	case SOUND_EVENT_ON_LOAD_FAILED:
//	case SOUND_EVENT_ON_STOP:
//	case SOUND_EVENT_ON_PLAYBACK_STOPPED:
//		removeSound = true;
//		// Fall through...
//
//	case SOUND_EVENT_ON_START:
//		{
//			PlayingSounds::iterator it = m_playingSounds.find(sound->GetId());
//			if (it != m_playingSounds.end())
//			{
//				PlayingSound& playing = it->second;
//				if (playing.listener)
//				{
//
//					ECommunicationHandlerEvent outEvent = SoundFailed;
//					switch (event)
//					{
//					case SOUND_EVENT_ON_LOAD_FAILED:
//						outEvent = (playing.type == Sound) ? SoundFailed : VoiceFailed;
//						break;
//					case SOUND_EVENT_ON_START:
//						outEvent = (playing.type == Sound) ? SoundStarted : VoiceStarted;
//						break;
//					case SOUND_EVENT_ON_STOP:
//						outEvent = (playing.type == Sound) ? SoundFinished : VoiceFinished;
//						break;
//					default:
//                        assert(false && "Missing initialization of outEvent");
//						break;
//					}
//
//					// Let the CommunicationPlayer know this sound/voice has finished
//					playing.listener->OnCommunicationHandlerEvent(outEvent, playing.playID, m_entityId);
//				}
//
//				// Remove the sound if it's no longer scheduled or playing
//				if (removeSound)
//				{
//					m_playingSounds.erase(it);
//					sound->RemoveEventListener(this);
//				}
//			}
//		}
//		break;
//	default:
//		break;
//	};
//}

//tSoundID CommunicationHandler::PlaySound(CommPlayID playID, const char* name, ESoundType type, ELipSyncMethod lipSyncMethod, IEventListener* listener)
//{
//	if (ISound *sound = gEnv->pAudioSystem->CreateSound(name, 
//		FLAG_SOUND_DEFAULT_3D | ((type == Voice) ? FLAG_SOUND_VOICE : 0) | (listener ? FLAG_SOUND_EVENT : 0)))
//	{
//		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
//		if(pEntity)
//		{
//			IEntityAudioProxy* pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
//			if (!pIEntityAudioProxy)
//			{
//				pEntity->CreateProxy(ENTITY_PROXY_AUDIO);
//				pIEntityAudioProxy = static_cast<IEntityAudioProxy*>(pEntity->GetProxy(ENTITY_PROXY_AUDIO));
//			}
//		
//			sound->SetSemantic(eSoundSemantic_AI_Readability);
//			
//			if(IAIObject* pAIObject = pEntity->GetAI())
//			{
//				if(IAIActorProxy* pAIProxy = pAIObject->GetProxy())
//				{
//					sound->SetParam("character_type", pAIProxy->GetFmodCharacterTypeParam(), true);
//				}
//			}
//
//			tSoundID soundID = sound->GetId();
//
//			if (soundID != INVALID_SOUNDID)
//			{
//				if (listener)
//				{
//					sound->AddEventListener(this, "CommunicationHandler");
//
//					std::pair<PlayingSounds::iterator, bool> iresult = m_playingSounds.insert(
//						PlayingSounds::value_type(soundID, PlayingSound()));
//
//					PlayingSound& playingSound = iresult.first->second;
//					playingSound.listener = listener;
//					playingSound.type = type;
//					playingSound.playID = playID;
//				}
//
//				pIEntityAudioProxy->PlaySound(sound, ZERO, FORWARD_DIRECTION, 1.0f, lipSyncMethod);
//
//				return soundID;
//			}
//		}
//	}
//	
//	if (listener)
//	{
//		ECommunicationHandlerEvent event = (type == Sound) ? SoundFailed : VoiceFailed;
//		listener->OnCommunicationHandlerEvent(event, playID, m_entityId);
//	}
//
//	return INVALID_SOUNDID;
//}

IAnimationGraphState* CommunicationHandler::GetAGState()
{
	if (m_agState)
		return m_agState;

	if (IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(m_entityId))
	{
		if (m_agState = actor->GetAnimationGraphState())
		{
			m_agState->AddListener("AICommunicationHandler", this);
			m_signalInputID = m_agState->GetInputId("Signal");
			m_actionInputID = m_agState->GetInputId("Action");
		}
	}	

	return m_agState;
}

void CommunicationHandler::QueryComplete(TAnimationGraphQueryID queryID, bool succeeded)
{
	if (queryID == m_currentQueryID) // this call happened during SetInput
	{
		m_currentPlaying = true;
		m_agState->QueryLeaveState(&m_currentQueryID);

		return;
	}

	PlayingAnimations::iterator animationIt = m_playingAnimations.find(queryID);
	if (animationIt != m_playingAnimations.end())
	{
		PlayingAnimation& playingAnimation = animationIt->second;
		if (!playingAnimation.playing)
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = succeeded ? ActionStarted : ActionFailed;
			else
				event = succeeded ? SignalStarted : SignalFailed;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);

			if (succeeded)
			{
				playingAnimation.playing = true;

				TAnimationGraphQueryID leaveQueryID;
				m_agState->QueryLeaveState(&leaveQueryID);

				m_playingAnimations.insert(PlayingAnimations::value_type(leaveQueryID, playingAnimation));
			}
		}
		else
		{
			ECommunicationHandlerEvent event;

			if (playingAnimation.method == AnimationMethodAction)
				event = ActionCancelled;
			else
				event = succeeded ? SignalFinished : SignalCancelled;

			if (playingAnimation.listener)
				playingAnimation.listener->OnCommunicationHandlerEvent(event, playingAnimation.playID, m_entityId);
		}

		m_playingAnimations.erase(animationIt);
	}
}

void CommunicationHandler::DestroyedState(IAnimationGraphState* agState)
{
	if (agState == m_agState)
		m_agState = 0;
}

bool CommunicationHandler::IsPlayingAnimation() const
{
	return !m_playingAnimations.empty(); 
}

bool CommunicationHandler::IsPlayingSound() const
{
	return true;//!m_playingSounds.empty(); 
}
