/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2011.
-------------------------------------------------------------------------

Description: 
Tweak menu item to change a gametoken

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataGametoken.h"

#include "IGameTokens.h"

//-------------------------------------------------------------------------

CTweakMetadataGameToken::CTweakMetadataGameToken(CTweakMenuController* pController, const string& command)
: CTweakMetadata(pController, command)
{
	m_type = eTT_GameToken;
	m_sName = command;
}

//-------------------------------------------------------------------------

void CTweakMetadataGameToken::InitItem(XmlNodeRef xml)
{
	CTweakMetadata::InitItem(xml);
}

//-------------------------------------------------------------------------

string CTweakMetadataGameToken::GetValue(void)
{
	string result = "Not found";

	IGameToken* pGameToken = GetGameToken();
	if (pGameToken) 
	{
		// Should we treat this as a bool?
		if (eFDT_Bool == pGameToken->GetType() )
		{
			bool value = false;
			pGameToken->GetValueAs(value);
			result = value ? "True" : "False";
		}
		else
		{
			result = pGameToken->GetValueAsString();
		}
	}
	return result;
}

//-------------------------------------------------------------------------

bool CTweakMetadataGameToken::ChangeValue(bool bIncrement)
{
	// Get delta
	double fDelta = m_fDelta;
	if (!bIncrement)
		fDelta *= -1.0f;

	// Get and check CVAR
	IGameToken* pGameToken = GetGameToken();
	if(!pGameToken)
		return false;

	string sValue;

	// Deal with appropriate type
	switch (pGameToken->GetType())
	{
	case eFDT_Bool:
		{
			bool value = false;
			pGameToken->GetValueAs(value);
			TFlowInputData newValue;
			newValue.SetValueWithConversion(!value);
			pGameToken->SetValue(newValue);
		}
		break;
	case eFDT_Int:
		{
			int value = 0;
			pGameToken->GetValueAs(value);
			TFlowInputData newValue;
			newValue.SetValueWithConversion((int)ClampToLimits(value + fDelta));
			pGameToken->SetValue(newValue);
		}
		break;
	case eFDT_Float:
		{
			float value = 0.0f;
			pGameToken->GetValueAs(value);
			TFlowInputData newValue;
			newValue.SetValueWithConversion((float)ClampToLimits(value + fDelta));
			pGameToken->SetValue(newValue);
		}
		break;
	case eFDT_String:
		{
			// bools pretending to be strings
			string value = pGameToken->GetValueAsString();
			if(!value.compareNoCase("true"))
				pGameToken->SetValueAsString("false");
			else if(!value.compareNoCase("false"))
				pGameToken->SetValueAsString("true");

			// other strings are non-obvious
		}
		break;
	default:
		break;
	}

	return true;
}

//-------------------------------------------------------------------------

// Wraps fetching the game token
IGameToken* CTweakMetadataGameToken::GetGameToken(void) const 
{
	IGameTokenSystem* pSystem = CCryAction::GetCryAction()->GetIGameTokenSystem();
	if(pSystem)
	{
		return pSystem->FindToken(m_command);
	}

	return NULL;
}

//-------------------------------------------------------------------------

