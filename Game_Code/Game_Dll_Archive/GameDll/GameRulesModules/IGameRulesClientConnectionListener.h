/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for a class that receives events when clients connect or
		disconnect
	-------------------------------------------------------------------------
	History:
	- 25:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _IGAME_RULES_CLIENT_CONNECTION_LISTENER_H_
#define _IGAME_RULES_CLIENT_CONNECTION_LISTENER_H_

#if _MSC_VER > 1000
# pragma once
#endif

class IGameRulesClientConnectionListener
{
public:
	virtual ~IGameRulesClientConnectionListener() {}

	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId) = 0;
	virtual void OnClientDisconnect(int channelId, EntityId playerId) = 0;
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) = 0;
	virtual void OnOwnClientEnteredGame() = 0;
};

#endif // _IGAME_RULES_CLIENT_CONNECTION_LISTENER_H_
