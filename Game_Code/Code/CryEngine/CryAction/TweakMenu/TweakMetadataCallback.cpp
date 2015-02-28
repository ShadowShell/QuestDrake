/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2011.
-------------------------------------------------------------------------

Description: 
	Header for tweak item that calls a user-supplied function

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataCallback.h"

CTweakMetadataCallback::CTweakMetadataCallback(CTweakMenuController* pMenuController, const string& command)
	: CTweakMetadata(pMenuController, command)
{
	m_type = eTT_Callback;
	m_pFunction = NULL;
	m_pUserData = NULL;
}

bool CTweakMetadataCallback::ChangeValue(bool bIncrement)
{
	if(m_pFunction)
		(m_pFunction)(m_pUserData);

	return true;
}
