/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Script Bind for CActionMapManager
  
 -------------------------------------------------------------------------
  History:
  - 8:11:2004   16:48 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_ACTIONMAPMANAGER_H__
#define __SCRIPTBIND_ACTIONMAPMANAGER_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


class CActionMapManager;


class CScriptBind_ActionMapManager :
	public CScriptableBase
{
public:
	CScriptBind_ActionMapManager(ISystem *pSystem, CActionMapManager *pActionMapManager);
	virtual ~CScriptBind_ActionMapManager();

	void Release() { delete this; };

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	//! <code>ActionMapManager.EnableActionFilter( name, enable )</code>
	//!		<param name="name">Filter name.</param>
	//!		<param name="enable">True to enable the filter, false otherwise.</param>
	//! <description>Enables a filter for the actions.</description>
	int EnableActionFilter(IFunctionHandler *pH, const char *name, bool enable);

	//! <code>ActionMapManager.EnableActionMap( name, enable )</code>
	//!		<param name="name">Action map name.</param>
	//!		<param name="enable">True to enable the filter, false otherwise.</param>
	//! <description>Enables an action map.</description>
	int EnableActionMap(IFunctionHandler *pH, const char *name, bool enable);

	//! <code>ActionMapManager.LoadFromXML( name )</code>
	//!		<param name="name">XML file name.</param>
	//! <description>Loads information from an XML file.</description>
	int LoadFromXML(IFunctionHandler *pH, const char *name);

private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	CActionMapManager *m_pManager;
};


#endif //__SCRIPTBIND_ACTIONMAPMANAGER_H__