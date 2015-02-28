/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakCommon, a base class for Tweak menu components

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/


#ifndef __CTWEAKCOMMON_H__
#define __CTWEAKCOMMON_H__

#pragma once

#include "ITweakMenuController.h"

//-------------------------------------------------------------------------
// Forward declarations

struct IScriptTable;

//-------------------------------------------------------------------------
// BC for common elements of a Tweak entry

class CTweakCommon
{
public:

	enum ETweakType
	{
		eTT_Menu,				// A menu
		eTT_Broken,			// Any item that failed in construction

		eTT_Cvar,				// item that changes a cvar value
		eTT_Command,		// item that executes a console command
		eTT_Lua,				// item that alters a lua value
		eTT_GameToken,	// item that changes a gametoken value
		eTT_ItemSimple,	// A simple item implemented in CPP (for simple debugging items and such)
		eTT_CodeVariable,// item registered by game code that directly changes a variable 		
		eTT_Callback,		// item that calls a user function when changed
	};

	// Common tweak constructor
	CTweakCommon(CTweakMenuController* pController);

	// Common tweak destructor
	virtual ~CTweakCommon() {};

	// item related
	virtual string GetValue(void) = 0;
	virtual bool DecreaseValue(void) = 0;
	virtual bool IncreaseValue(void) = 0;

	// Get the type of this tweak
	ETweakType GetType(void) { return m_type; }

	// Get the name of this tweak component
	const string &GetName(void) const { return m_sName; }
	void SetName(const string& name) { m_sName = name; }

	// Get the description of this tweak component
	const string &GetDescription(void) const { return m_sDescription; }

	// Get if this tweak component is hidden
	bool IsHidden(void) const { return m_bHidden; }

	void SetId(TTweakableItemId id) { m_id = id; }
	TTweakableItemId GetId() const	{ return m_id; }

protected:

	// Wraps fetching a Lua table entry
	string FetchStringValue(IScriptTable *pTable, const char *sKey) const;

	// The name of this tweak component
	string m_sName;

	// The description of this tweak component
	string m_sDescription;

	// Is this component hidden?
	bool m_bHidden;

	// If there are non-fatal errors, they can be reported here
	string m_sError;

	ETweakType m_type;

	CTweakMenuController* m_pMenuController;

	TTweakableItemId m_id;
};

#endif // __CTWEAKCOMMON_H__