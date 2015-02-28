/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMetadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack
- 23:06:2006	:	Incrementer/decrementer functionality moved here

Notes:
	State table awaits implementation, for more powerful LUA incrementers.

*************************************************************************/


#ifndef __CTWEAKMETADATA_H__
#define __CTWEAKMETADATA_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakCommon.h"

//-------------------------------------------------------------------------

class CTweakMetadata : public CTweakCommon 
{
public:

	CTweakMetadata(CTweakMenuController* pController, const string& command);
	~CTweakMetadata() {};

	virtual void InitItem(XmlNodeRef xml);

	void SetCommand(const string& command) { m_command = command; }
	void AddDisplayString(int value, const string& name);

protected:

	ScriptAnyValue GetLuaValue(void);

	double ClampToLimits( double x ) const;

	string GetDisplayString(int value) const;

	// The delta hint amount
	float m_fDelta;
	bool m_boolean;

	// The minimum and maximum numeric values
	float m_fMin, m_fMax;

	// command could be a cvar, lua string, gametoken name...
	string m_command;

	// display strings for specific values (eg 0="off", 1="on")
	std::map<int, string> m_intSettings;
};

#endif // __CTWEAKMETADATA_H__