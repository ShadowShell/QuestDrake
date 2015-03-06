#include "StdAfx.h"

#include "GameServerLists.h"
#include "Network/Lobby/GameBrowser.h"
#include "UI/ProfileOptions.h"
#include "GameLobbyData.h"

#if IMPLEMENT_PC_BLADES
/////////////////////////////////////////
CGameServerLists::CGameServerLists()
{
	Reset();

	IPlayerProfileManager *pProfileMgr = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if(pProfileMgr)
	{
		pProfileMgr->AddListener(this, true);
	}

	m_bHasChanges = false;












}

//---------------------------------------
CGameServerLists::~CGameServerLists()
{
#if !defined(_RELEASE)
	gEnv->pConsole->RemoveCommand("gsl_addFavourite");
	gEnv->pConsole->RemoveCommand("gsl_removeFavourite");
	gEnv->pConsole->RemoveCommand("gsl_listFavourite");
	gEnv->pConsole->RemoveCommand("gsl_showFavourite");

	gEnv->pConsole->RemoveCommand("gsl_addRecent");
	gEnv->pConsole->RemoveCommand("gsl_removeRecent");
	gEnv->pConsole->RemoveCommand("gsl_listRecent");
	gEnv->pConsole->RemoveCommand("gsl_showRecent");
#endif

	IPlayerProfileManager *pProfileMgr = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if(pProfileMgr)
	{
		pProfileMgr->RemoveListener(this);
	}
}

//---------------------------------------
void CGameServerLists::Reset()
{
	for(int i = 0; i < eGSL_Size; i++)
	{
		m_list[i].clear();
		m_rule[i].Reset();
	}
}

//---------------------------------------
const bool CGameServerLists::Add(const EGameServerLists list, const char* name, const uint32 favouriteId, bool bFromProfile)
{
	if(name && name[0])
	{
		//CryLog("[UI] CGameServerLists::Add %s %u", name, favouriteId);

		SServerInfoInt newServerInfo(name, favouriteId);
		m_rule[list].PreApply(&m_list[list], newServerInfo);
		m_list[list].push_front(newServerInfo);

		if (bFromProfile == false)
		{
			if (list == eGSL_Favourite)
			{
				m_bHasChanges = true;
			}
			else
			{














			}
		}
		return true;
	}

	return false;
}

//---------------------------------------
const bool CGameServerLists::Remove(const EGameServerLists list, const uint32 favouriteId)
{
	//CryLog("[UI] CGameServerLists::Remove %u", favouriteId);

	SServerInfoInt removeInfo(NULL, favouriteId);
	m_list[list].remove(removeInfo);

	m_bHasChanges = true;

	return true;
}

//---------------------------------------
const int CGameServerLists::GetTotal(const EGameServerLists list) const
{
	return m_list[list].size();
}

//---------------------------------------
const bool CGameServerLists::InList(const EGameServerLists list, const uint32 favouriteId) const
{
	std::list<SServerInfoInt>::const_iterator it;
	std::list<SServerInfoInt>::const_iterator end = m_list[list].end();

	for (it = m_list[list].begin(); it != end; it++)
	{
		if (it->m_favouriteId == favouriteId)
		{
			return true;
		}
	}
	return false;	
}

//---------------------------------------
void CGameServerLists::PopulateMenu(const EGameServerLists list) const
{
	if (CGameBrowser *pGameBrowser = g_pGame->GetGameBrowser())
	{
		uint32 numIds = 0;
		uint32 favouriteIdList[k_maxServersStoredInList];
		memset(favouriteIdList, INVALID_SESSION_FAVOURITE_ID, sizeof(favouriteIdList));

		std::list<SServerInfoInt>::const_iterator it;
		std::list<SServerInfoInt>::const_iterator end = m_list[list].end();

		for(it = m_list[list].begin(); it != end; it++)
		{
			favouriteIdList[numIds] = it->m_favouriteId;
			++numIds;
		}

		pGameBrowser->StartFavouriteIdSearch(list, favouriteIdList, numIds );
	}
}

//---------------------------------------
void CGameServerLists::SaveChanges()
{
	if (m_bHasChanges)
	{














		CProfileOptions *pProfileOptions = g_pGame->GetProfileOptions();
		if (pProfileOptions)
		{
			pProfileOptions->SaveProfile();
		}

		m_bHasChanges = false;
	}
}

//---------------------------------------
void CGameServerLists::SaveToProfile(IPlayerProfile* pProfile, bool online, unsigned int reason)
{
	if(reason & ePR_Options)
	{
		for(int i = 0; i < eGSL_Size; i++)
		{
			//Save it out backwards so it adds them in the same order
			std::list<SServerInfoInt>::const_reverse_iterator it;
			std::list<SServerInfoInt>::const_reverse_iterator end = m_list[i].rend();
			int j = 0;
			int numServers = m_list[i].size();
			pProfile->SetAttribute(string().Format("MP/ServerLists/%d/Num", i).c_str(), numServers);
			for(it = m_list[i].rbegin(); it != end; ++it)
			{
				pProfile->SetAttribute(string().Format("MP/ServerLists/%d/%d/Name", i, j).c_str(), (*it).m_name);
				pProfile->SetAttribute(string().Format("MP/ServerLists/%d/%d/Id", i, j).c_str(), (*it).m_favouriteId);
				j++;
			}
		}
	}
}

//---------------------------------------
void CGameServerLists::LoadFromProfile(IPlayerProfile* pProfile, bool online, unsigned int reason)
{
	if(reason & ePR_Options)
	{
		Reset();

		for(int i = 0; i < eGSL_Size; i++)
		{
			int numServers = 0;
			if (pProfile->GetAttribute(string().Format("MP/ServerLists/%d/Num", i).c_str(), numServers))
			{
				for (int j = 0; j < numServers; ++ j)
				{
					string name("");
					if (pProfile->GetAttribute(string().Format("MP/ServerLists/%d/%d/Name", i, j).c_str(), name))
					{
						uint32 favouriteId = INVALID_SESSION_FAVOURITE_ID;
						if (pProfile->GetAttribute(string().Format("MP/ServerLists/%d/%d/Id", i, j).c_str(), favouriteId))
						{
							if ((!name.empty()) && (favouriteId != INVALID_SESSION_FAVOURITE_ID))
							{
								Add((EGameServerLists) i, name.c_str(), favouriteId, true);
							}
						}
					}
				}
			}
		}
	}
}

//---------------------------------------
void CGameServerLists::ServerFound( const SServerInfo &serverInfo, const EGameServerLists list, const uint32 favouriteId )
{
	//CryLog("[UI] CGameServerLists::ServerFound %s %d", serverInfo.m_hostName.c_str(), serverInfo.m_sessionFavouriteKeyId);

//TODO: "Forward this to UILobbyMP"















}

//---------------------------------------
void CGameServerLists::ServerNotFound( const EGameServerLists list, const uint32 favouriteId )
{
	//CryLog("[UI] CGameServerLists::ServerNotFound %d", favouriteId);

//TODO: "Forward this to UILobbyMP"




















}








































































































/////////////////////////////////////////
CGameServerLists::SServerInfoInt::SServerInfoInt(const char* name, uint32 favouriteId)
{
	m_name = name;
	m_favouriteId = favouriteId;
}

//---------------------------------------
bool CGameServerLists::SServerInfoInt::operator == (const SServerInfoInt & other) const
{
	return (m_favouriteId == other.m_favouriteId);
}

/////////////////////////////////////////
CGameServerLists::SListRules::SListRules()
{
	Reset();
}

//---------------------------------------
void CGameServerLists::SListRules::Reset()
{
	m_limit = k_maxServersStoredInList;
	m_unique = true;
}

//---------------------------------------
void CGameServerLists::SListRules::PreApply(std::list<SServerInfoInt>* pList, const SServerInfoInt &pNewInfo)
{
	if(m_limit)
	{
		//Will hit limit so pop now
		if(pList->size() >= m_limit)
		{
			pList->pop_back();
		}
	}

	if(m_unique)
	{
		//if it already exists remove it
		pList->remove(pNewInfo);
	}
}

#endif //IMPLEMENT_PC_BLADES