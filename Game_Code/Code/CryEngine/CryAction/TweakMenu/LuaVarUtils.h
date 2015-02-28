/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Lua variable helpers used by Tweak Menu

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/


#ifndef __LUAVARUTILS_H__
#define __LUAVARUTILS_H__

#pragma once

#include "IScriptSystem.h"

//-------------------------------------------------------------------------
// Lua variable helpers used by Tweak entries and Tweak Menu Controller

namespace LuaVarUtils
{
	// Gets a variable from Lua using a recursive lookup
	bool GetRecursive(const char *sKey, ScriptAnyValue &result);
	bool GetRecursive(const char *sKey, ScriptAnyValue &result,const ScriptAnyValue &initVal);

	template <class T> bool GetRecursive(const char *sKey, T &result)
	{ 
		ScriptAnyValue value;
		bool success = false;
		if (GetRecursive(sKey, value)) 
			success = value.CopyTo(result);
		return success;
	}

	template <class T> bool GetRecursive(const char *sKey, T &result, const T &initVal)
	{ 
		ScriptAnyValue value,val(initVal);
		bool success = GetRecursive(sKey, value, val);
		bool bCopied = value.CopyTo(result);
		success = success && bCopied;
		return success;
	}

	// Set a LUA value by a full path, fails if tables in path do not exist
	bool SetRecursive(const char *sKey, const ScriptAnyValue &newValue);

	template <class T> bool SetRecursive(const char *sKey, const T &newValue)
	{ 
		ScriptAnyValue value(newValue);
		return SetRecursive(sKey, value);
	}
};

#endif //__LUAVARUTILS_H__
