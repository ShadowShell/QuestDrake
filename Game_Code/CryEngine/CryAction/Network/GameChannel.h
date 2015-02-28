/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Common functionality for CGameServerChannel, CGameClientChannel

-------------------------------------------------------------------------
History:
- 6:10:2004   11:38 : Created by Craig Tiller

*************************************************************************/
#ifndef __GAMECHANNEL_H__
#define __GAMECHANNEL_H__

#pragma once

class CGameContext;
class CPhysicsSync;

class CGameChannel : public IGameChannel
{
public:
	CGameChannel();
	~CGameChannel();
	
	// IGameChannel
	// ~IGameChannel

	virtual bool IsServer() = 0;

	// internal, useful stuff
	EntityId GetPlayerId() const { return m_playerId; }
	INetChannel * GetNetChannel() { return m_pNetChannel; }
	CGameContext * GetGameContext() { return m_pContext; }
	CPhysicsSync * GetPhysicsSync() { return m_pPhysicsSync.get(); }

	void SetNetChannel(INetChannel *pNetChannel)
	{
		m_pNetChannel=pNetChannel;
	};

	void ConfigureContext( bool isLocal );

	void ResetPlayerId() { m_playerId = 0; }

protected:
	void Init( INetChannel * pNetChannel, CGameContext * pGameContext );
	void SetPlayerId( EntityId id );

	bool ReadPacketHeader( CTimeValue tm );
	bool ReadPacketFooter();

protected:
	INetChannel *m_pNetChannel;
	CGameContext *m_pContext;
	std::auto_ptr<CPhysicsSync> m_pPhysicsSync;

	EntityId m_playerId;
};

#endif
