/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2008.
-------------------------------------------------------------------------

Description: 
Simple tweak menu item for debugging and other items not requiring LUA

-------------------------------------------------------------------------
History:
- 1:11:2008  : Created by Adam Rutkowski

*************************************************************************/


#ifndef __CTWEAKITEMSIMPLE_H__
#define __CTWEAKITEMSIMPLE_H__

#pragma once

#include "TweakMetadata.h"
#include "TweakMenuController.h"

//-------------------------------------------------------------------------

class CTweakItemSimple : public CTweakMetadata
{
public:

	CTweakItemSimple(CTweakMenuController* pController, const string& command);
	~CTweakItemSimple() {};

	virtual string GetValue(void) { return ""; }
	virtual bool DecreaseValue(void) { return Execute(); }
	virtual bool IncreaseValue(void) { return Execute(); }

protected:
	virtual bool Execute() { return true; }
};

//-------------------------------------------------------------------------

class CTweakItemReload : public CTweakItemSimple
{
public:
	CTweakItemReload(CTweakMenuController* pController) : CTweakItemSimple(pController, "")
	{
		m_sName = "Reload Tweaks";
		m_sDescription = "Reloads the tweak menu from xml";
	}

protected:
	virtual bool Execute()
	{
		m_pMenuController->Init();
		return true;
	}
};

#endif // __CTWEAKITEMSIMPLE_H__
