////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2007.
// -------------------------------------------------------------------------
//  File name:   SubtitleManager.h
//  Version:     v1.00
//  Created:     29/01/2007 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description: Subtitle Manager Implementation 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SUBTITLEMANAGER_H__
#define __SUBTITLEMANAGER_H__
#pragma once

#include "ISubtitleManager.h"

class CSubtitleManager : public ISubtitleManager
{
public:
	CSubtitleManager();
	virtual ~CSubtitleManager();

	// ISubtitleManager
	VIRTUAL void SetHandler(ISubtitleHandler* pHandler) { m_pHandler = pHandler; }
	VIRTUAL ISubtitleHandler* GetHandler() { return m_pHandler ? m_pHandler : &m_dummyHandler; }
	VIRTUAL void SetEnabled(bool bEnabled);
	VIRTUAL void SetAutoMode(bool bOn);
	//VIRTUAL void ShowSubtitle(ISound* pSound, bool bShow);
	VIRTUAL void ShowSubtitle(const char* subtitleLabel, bool bShow);
	// ~ISubtitleManager

protected:
	struct CSubtitleHandlerDummy : public ISubtitleHandler
	{
		CSubtitleHandlerDummy() {}
		//void ShowSubtitle(ISound* pSound, bool bShow) {}
		void ShowSubtitle(const char* subtitleLabel, bool bShow) {}
	};
	CSubtitleHandlerDummy m_dummyHandler;
	ISubtitleHandler* m_pHandler;
	bool m_bEnabled;
	bool m_bAutoMode;

};

#endif // __SUBTITLEMANAGER_H__