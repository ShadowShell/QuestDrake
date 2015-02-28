/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Base class for Tweak menu components

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakCommon.h"
#include "TweakMenu.h"
#include "TweakMenuController.h"
#include "LuaVarUtils.h"

//-------------------------------------------------------------------------

CTweakCommon::CTweakCommon(CTweakMenuController* pController)
: m_bHidden(false)
, m_type(eTT_Broken)
, m_pMenuController(pController)
{
	m_id = m_pMenuController->RegisterItem();
}

//-------------------------------------------------------------------------

string CTweakCommon::FetchStringValue(IScriptTable *pTable, const char *sKey) const 
{
	string result;
	const char * sString;
	if (pTable->GetValue(sKey,sString)) {
		result = sString;
	}
	return result;
}
