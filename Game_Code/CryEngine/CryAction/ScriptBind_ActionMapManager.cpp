/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 8:11:2004   16:49 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ScriptBind_ActionMapManager.h"
#include "ActionMapManager.h"


//------------------------------------------------------------------------
CScriptBind_ActionMapManager::CScriptBind_ActionMapManager(ISystem *pSystem, CActionMapManager *pActionMapManager)
: m_pSystem(pSystem),
	m_pManager(pActionMapManager)
{
	Init(gEnv->pScriptSystem, m_pSystem);
	SetGlobalName("ActionMapManager");

	RegisterGlobals();
	RegisterMethods();
}

//------------------------------------------------------------------------
CScriptBind_ActionMapManager::~CScriptBind_ActionMapManager()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActionMapManager::RegisterGlobals()
{
}

//------------------------------------------------------------------------
void CScriptBind_ActionMapManager::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_ActionMapManager::

	SCRIPT_REG_TEMPLFUNC(EnableActionFilter, "name, enable");
	SCRIPT_REG_TEMPLFUNC(EnableActionMap, "name, enable");
	SCRIPT_REG_TEMPLFUNC(LoadFromXML, "name");
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::EnableActionFilter(IFunctionHandler *pH, const char *name, bool enable)
{
	m_pManager->EnableFilter(name, enable);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::EnableActionMap(IFunctionHandler *pH, const char *name, bool enable)
{
	m_pManager->EnableActionMap(name, enable);

	return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_ActionMapManager::LoadFromXML(IFunctionHandler *pH, const char *name)
{
	XmlNodeRef rootNode = m_pSystem->LoadXmlFromFile(name);
	m_pManager->LoadFromXML(rootNode);

	return pH->EndFunction();
}
