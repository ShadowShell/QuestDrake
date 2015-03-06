/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Central storage and processing of all gamecode client side 
					   monitoring

-------------------------------------------------------------------------
History:
- 09:11:2011: Created by Stewart Needham

*************************************************************************/
#include "StdAfx.h"
#include "AntiCheat/ClientCheatMonitor.h"
#include "GameCVars.h"
#include "GameRules.h"
#include "LocalPlayerComponent.h"

#if CLIENT_CHEAT_MONITOR_ENABLED
//========================================================================
// Construction/Destruction
//========================================================================
CClientCheatMonitor::CClientCheatMonitor()
	:m_pFramework(NULL)
{
	m_MonitorAccessOffset = cry_rand();
}

CClientCheatMonitor::~CClientCheatMonitor()
{
	m_pFramework->UnregisterListener(this);
}

//========================================================================
// Init
//========================================================================
void CClientCheatMonitor::Init(IGameFramework *pFramework)
{
	pFramework->RegisterListener(this,"CClientCheatMonitor", FRAMEWORKLISTENERPRIORITY_DEFAULT);

	m_pFramework = pFramework;
}

//========================================================================
// IOnPostUpdate
//========================================================================
void CClientCheatMonitor::OnPostUpdate(float fDeltaTime)
{
}

//========================================================================
// OnSaveGame
//========================================================================
void CClientCheatMonitor::OnSaveGame(ISaveGame* pSaveGame)
{
}

//========================================================================
// OnLoadGame
//========================================================================
void CClientCheatMonitor::OnLoadGame(ILoadGame* pLoadGame)
{
}

//========================================================================
// OnLevelEnd
//========================================================================
void CClientCheatMonitor::OnLevelEnd(const char* nextLevel)
{
}

//========================================================================
// OnActionEvent
//========================================================================
void CClientCheatMonitor::OnActionEvent(const SActionEvent& event)
{
}

//========================================================================
// UploadTelemetry
//========================================================================
void CClientCheatMonitor::UploadTelemetry()
{
}
#endif
