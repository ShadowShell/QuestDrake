/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------

Description: 
Header for CTweakMetadataCommand

-------------------------------------------------------------------------
History:
- 07:10:2009  : Created by Alex McCarthy

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataCommand.h"

CTweakMetadataCommand::CTweakMetadataCommand(CTweakMenuController* pController, const string& command)
: CTweakMetadata(pController, command)
{
	m_bUseRconCommand = false;
	m_type = eTT_Command;
}

void CTweakMetadataCommand::InitItem(XmlNodeRef xml)
{
	CTweakMetadata::InitItem(xml);

	// Use Rcon command?
	xml->getAttr("RCON", m_bUseRconCommand);
}

bool CTweakMetadataCommand::ExecuteCommand()
{
	const char* szCommand = m_command.c_str();
	// Send RCon command?
	IRemoteControlClient *pRConClient = gEnv->pNetwork->GetRemoteControlSystemSingleton()->GetClientSingleton();
	CRY_ASSERT(pRConClient);
	if (m_bUseRconCommand && !gEnv->bServer && pRConClient)
	{
		pRConClient->SendCommand(szCommand);
	}
	else
	{
		gEnv->pConsole->ExecuteString(szCommand);
	}

	return true;
}

