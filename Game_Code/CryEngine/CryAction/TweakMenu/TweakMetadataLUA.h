/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMetadataLua

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack
- 23:06:2006	:	Incrementer/decrementer functionality moved to CTweakMetaData

*************************************************************************/


#ifndef __CTWEAKMETADATALUA_H__
#define __CTWEAKMETADATALUA_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakMetadata.h"

//-------------------------------------------------------------------------

class CTweakMetadataLUA : public CTweakMetadata 
{
public:

	CTweakMetadataLUA(CTweakMenuController* pController, const string& command);

	~CTweakMetadataLUA() {}

	void InitItem(XmlNodeRef xml);

	string GetValue(void);

	bool DecreaseValue(void) { return ChangeValue(false); }

	bool IncreaseValue(void) { return ChangeValue(true); }

protected:

	// Wraps fetching a LUA value (possibly actually a function)
	ScriptAnyValue GetLuaValue(void) const;

	// Wraps setting a LUA variable
	void SetLVar(const ScriptAnyValue &value) const;

	// Wraps incrementing/decrementing a LUA variable (or calling the LUA functions)
	bool ChangeValue(bool increment) const;
};

#endif // __CTWEAKMETADATALUA_H__
