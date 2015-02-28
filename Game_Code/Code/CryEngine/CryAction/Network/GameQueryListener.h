/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  This listener looks for running servers in the LAN and
is keeping a list of all available servers.
There is a callback to get this list (ordered by ping) as well as to connect to one ...
-------------------------------------------------------------------------
History:
- 19/06/2006   : Implemented by Jan Müller
*************************************************************************/

#ifndef __GAMEQUERYLISTENER_H__
#define __GAMEQUERYLISTENER_H__

#pragma once

class CGameQueryListener : public IGameQueryListener
{

	struct SGameServer
	{
		string			m_target;
		string			m_description;
		string			m_data;
		uint32			m_ping;
		CTimeValue	m_lastTime;

		SGameServer(const char* target, const char* description, const char* data, const uint32 ping) :
			m_target(target), m_description(description), m_data(data), m_ping(ping)
		{ 
			m_lastTime = gEnv->pTimer->GetFrameStartTime();
		}

		~SGameServer()
		{

		}

		bool Compare(SGameServer &server)
		{
			if(server.m_target.compare(m_target) == 0 && server.m_description.compare(m_data) == 0)
				return true;
			return false;
		}

		bool operator<(const SGameServer &server) const
		{
			if(m_ping < server.m_ping)
				return true;
			return false;
		}	

		void Inc(SGameServer &server)
		{
			if(m_data.compare(server.m_data) != 0)
				m_data = server.m_data;
			IncPing(server.m_ping);
			m_lastTime = gEnv->pTimer->GetFrameStartTime();
		}

		void IncPing(const uint32 ping)
		{
			if(m_ping)
				m_ping = (3*m_ping + ping) / int(4);
			else
				m_ping = ping;
		}

		string GetServerGameVersion()
		{
			string retVal("version not found in data");
			int pos1, pos2;
			pos1 = m_data.find(string("version"));		//this code will be broken if game info changes
			pos2 = m_data.find(string("maxPlayers"));
			if(pos1 != string::npos && pos2 != string::npos)
			{
				pos1 += 9;
				retVal = m_data.substr(pos1, pos2-pos1-2);
			}
			return retVal;
		}
	};

public:
	CGameQueryListener();
	~CGameQueryListener(); 

	// IGameQueryListener
	VIRTUAL void AddServer(const char* description, const char* target, const char* additionalText, uint32 ping);
	VIRTUAL void RemoveServer(string address);
	VIRTUAL void AddPong(string address, uint32 ping);
	VIRTUAL void GetCurrentServers(char*** pastrServers, int& o_amount);
	VIRTUAL void GetServer(int number, char** server, char** data, int& ping);
	VIRTUAL const char* GetServerData(const char* server, int& o_ping);
	VIRTUAL void Update();
	VIRTUAL void OnReceiveGameState( const char * fromAddress, XmlNodeRef xmlData );
	virtual void Release();
	VIRTUAL void RefreshPings();
	VIRTUAL void ConnectToServer(const char* server);
	VIRTUAL void GetValuesFromData(char *strData,SServerData *pServerData);
	// ~IGameQueryListener

	void GetMemoryStatistics(ICrySizer * s);

	void SetNetListener( INetQueryListener * pListener );
	void Complete();


private:

	SGameServer* FindServer(const char* target);

	//throw out old servers
	void CleanUpServers();

	INetQueryListener * m_pNetListener;

	//a vector of servers currently running (from NetQueryListener)
	std::vector<SGameServer>	m_servers;
	char											*m_astrServers[120];
	int												m_iServers;
	
	//flag for server sorting
	bool		m_bSortingNeeded;
};

#endif
