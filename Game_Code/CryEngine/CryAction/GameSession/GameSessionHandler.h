/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Default session handler implementation.

-------------------------------------------------------------------------
History:
- 08:12:2009 : Created By Ben Johnson

*************************************************************************/
#ifndef __GAME_SESSION_HANDLER_H__
#define __GAME_SESSION_HANDLER_H__

#include <IGameSessionHandler.h>

class CGameSessionHandler : public IGameSessionHandler
{
public:
	CGameSessionHandler();
	virtual ~CGameSessionHandler();

	// IGameSessionHandler
	VIRTUAL bool ShouldCallMapCommand(const char *pLevelName, const char *pGameRules);
	VIRTUAL void JoinSessionFromConsole(CrySessionID sessionID);
	VIRTUAL int EndSession();
	
	VIRTUAL int StartSession();
	VIRTUAL void LeaveSession();

	VIRTUAL void OnUserQuit();
	VIRTUAL void OnGameShutdown();

	VIRTUAL CrySessionHandle GetGameSessionHandle() const;
	VIRTUAL bool ShouldMigrateNub() const;

	VIRTUAL bool IsMultiplayer() const;
	VIRTUAL int GetNumberOfExpectedClients();

	VIRTUAL bool IsGameSessionMigrating() const;
	VIRTUAL bool IsMidGameLeaving() const;
	// ~IGameSessionHandler
};

#endif //__GAME_SESSION_HANDLER_H__
