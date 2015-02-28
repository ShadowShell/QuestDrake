/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Wraps a unit of Tweak metadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataCVAR.h"

//-------------------------------------------------------------------------

CTweakMetadataCVAR::CTweakMetadataCVAR(CTweakMenuController* pController, const string& command)
: CTweakMetadata(pController, command)
{
	m_CVarType = 0;
	m_type = eTT_Cvar;
}

//-------------------------------------------------------------------------

void CTweakMetadataCVAR::InitItem(XmlNodeRef xml)
{
	CTweakMetadata::InitItem(xml);

	xml->getAttr("RCON", m_bUseRconCommand);

	// Get description and type
	ICVar *cVar = GetCVar();
	if (cVar)
	{
		m_sDescription = cVar->GetHelp();
		m_CVarType  = GetCVar()->GetType();
	}
}

//-------------------------------------------------------------------------

string CTweakMetadataCVAR::GetValue(void)
{
	string result = "Not found";

	ICVar *cVar = GetCVar();
	if (cVar) 
	{
		result = cVar->GetString();

		// Should we treat this as a bool?
		if ( m_boolean && CVAR_INT == cVar->GetType() )
		{
			result = ( cVar->GetIVal() ? "True" : "False" );
		}
		else if(CVAR_INT == cVar->GetType())
		{
			string temp = GetDisplayString(cVar->GetIVal());
			if(!temp.empty())
			{
				result = temp;
			}
		}
	}
	return result;
}
		
//-------------------------------------------------------------------------

bool CTweakMetadataCVAR::ChangeValue(bool bIncrement)
{
	// Get delta
	double fDelta = m_fDelta;
	if (!bIncrement)
		fDelta *= -1.0f;

	// Get and check CVAR
	ICVar *cVar = GetCVar();
	if (!cVar)
		return false;

	string sValue;
	
	// Deal with appropriate type
	switch (cVar->GetType())
	{
		case CVAR_INT:
			// Should we treat this as a bool?
			if (m_boolean)
				sValue.Format("%d", cVar->GetIVal() ? 0 : 1);
			else
				sValue.Format("%d", (int)ClampToLimits(cVar->GetIVal() + fDelta));
			break;
		case CVAR_FLOAT:
			sValue.Format("%f", (float)ClampToLimits(cVar->GetFVal() + fDelta));
			break;
		default:;
			// Strings are non-obvious
				// Might also be a non-existent variable			
	}

	// Send RCon command?
	IRemoteControlClient *pRConClient = gEnv->pNetwork->GetRemoteControlSystemSingleton()->GetClientSingleton();
	CRY_ASSERT(pRConClient);
	if (m_bUseRconCommand && !gEnv->bServer && pRConClient)
	{
		string sCommand;
		sCommand.Format("%s %s", m_command.c_str(), sValue.c_str());
		pRConClient->SendCommand(sCommand.c_str());
	}
	else
	{
		// Set value
		cVar->Set(sValue.c_str());
	}

	return true;
}

//-------------------------------------------------------------------------

// Wraps fetching the console variable
ICVar * CTweakMetadataCVAR::GetCVar(void) const 
{
	return gEnv->pConsole->GetCVar(m_command.c_str());
}

//-------------------------------------------------------------------------

