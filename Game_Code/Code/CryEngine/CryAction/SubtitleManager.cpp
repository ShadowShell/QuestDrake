////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2007.
// -------------------------------------------------------------------------
//  File name:   SubtitleManager.cpp
//  Version:     v1.00
//  Created:     29/01/2007 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description: Subtitle Manager Implementation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SubtitleManager.h"



CSubtitleManager::CSubtitleManager()
{
	m_bEnabled = false;
	m_bAutoMode = true;
	m_pHandler = NULL;
}

CSubtitleManager::~CSubtitleManager()
{
	/*if (m_bEnabled)
		gEnv->pAudioSystem->RemoveEventListener(this);*/

}

void CSubtitleManager::SetEnabled(bool bEnabled)
{
	if (bEnabled != m_bEnabled)
	{
		m_bEnabled = bEnabled;
		/*if (m_bEnabled)
			gEnv->pAudioSystem->AddEventListener(this, true);
		else
			gEnv->pAudioSystem->RemoveEventListener(this);*/
	}
}

void CSubtitleManager::SetAutoMode(bool bOn)
{
	m_bAutoMode = bOn;	
}

//void CSubtitleManager::OnSoundSystemEvent(ESoundSystemCallbackEvent event, ISound *pSound)
//{
//	const bool bStart = event == SOUNDSYSTEM_EVENT_ON_PLAYBACK_STARTED;
//	const bool bStop  = event == SOUNDSYSTEM_EVENT_ON_PLAYBACK_STOPPED;
//	if (bStart || bStop)
//	{
//		if (m_bAutoMode && m_bEnabled && m_pHandler)
//			m_pHandler->ShowSubtitle(pSound, bStart);
//	}
//}

//void CSubtitleManager::ShowSubtitle(ISound* pSound, bool bShow)
//{
//	if (m_bEnabled && m_pHandler)
//		m_pHandler->ShowSubtitle(pSound, bShow);
//}

void CSubtitleManager::ShowSubtitle(const char* subtitleLabel, bool bShow)
{
	if (m_bEnabled && m_pHandler)
		m_pHandler->ShowSubtitle(subtitleLabel, bShow);
}

#include UNIQUE_VIRTUAL_WRAPPER(ISubtitleManager)