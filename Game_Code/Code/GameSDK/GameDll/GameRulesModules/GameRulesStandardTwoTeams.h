/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 02:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _GameRulesStandardTwoTeams_h_
#define _GameRulesStandardTwoTeams_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameRulesTeamsModule.h"
#include "CryFixedString.h"

class CGameRules;

class CGameRulesStandardTwoTeams : public IGameRulesTeamsModule
{
public:
	CGameRulesStandardTwoTeams();
	virtual ~CGameRulesStandardTwoTeams();

	virtual void Init(XmlNodeRef xml);
	virtual void PostInit();

	virtual void RequestChangeTeam(EntityId playerId, int teamId, bool onlyIfUnassigned);

	virtual int GetAutoAssignTeamId(EntityId playerId);

	virtual bool	CanTeamModifyWeapons(int teamId);

protected:
	void DoTeamChange(EntityId playerId, int teamId);

	CGameRules *m_pGameRules;
	bool m_bCanTeamModifyWeapons[2];
};

#endif // _GameRulesStandardTwoTeams_h_