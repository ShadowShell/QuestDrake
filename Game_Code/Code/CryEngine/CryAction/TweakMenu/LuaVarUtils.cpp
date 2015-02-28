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

#include "StdAfx.h"
#include "LuaVarUtils.h"

//-------------------------------------------------------------------------

bool LuaVarUtils::GetRecursive(const char *sKey, ScriptAnyValue &result)
{
	// Copy the string
	string tokenStream(sKey);
	string token;
	int curPos = 0;

	ScriptAnyValue value;

	// Deal with first token specially
	token = tokenStream.Tokenize(".", curPos);
	if (token.empty()) return false; // Catching, say, an empty string
	if (! gEnv->pScriptSystem->GetGlobalAny(token,value) || value.type == ANY_TNIL ) return false;

	// Tokenize remainder
	token = tokenStream.Tokenize(".", curPos);
	while (!token.empty()) {
		// Make sure the last step was a table
		if (value.type != ANY_TTABLE) return false;

		// Must use temporary 
		ScriptAnyValue getter;
		value.table->GetValueAny(token, getter);
		value = getter;	
		token = tokenStream.Tokenize(".", curPos);
	}

	result = value;
	return true;
}

//-------------------------------------------------------------------------

bool LuaVarUtils::GetRecursive(const char *sKey, ScriptAnyValue &result,const ScriptAnyValue &initVal)
{
	bool bRet=true;
	if(!GetRecursive(sKey, result))
	{
		if(SetRecursive(sKey,initVal))
			;//result=initVal;
		else
			bRet=false;
		result=initVal;
	}
	return bRet;
}

//-------------------------------------------------------------------------

bool LuaVarUtils::SetRecursive(const char *sKey, const ScriptAnyValue &newValue)
{

	string tokenStream(sKey);

	IScriptSystem *pIScriptSystem = gEnv->pScriptSystem;

	// It might be a global - i.e. only one token
	if (tokenStream.find(".") == string::npos) {
		// Set as a global only
		pIScriptSystem->SetGlobalAny(sKey, newValue);
		return true;
	}

	// Init for tokenizing
	string token;
	int curPos = 0;
	ScriptAnyValue value;

	// Deal with first token specially
	token = tokenStream.Tokenize(".", curPos);
	pIScriptSystem->GetGlobalAny(token,value);

	token = tokenStream.Tokenize(".", curPos);
	if (token.empty()) return false; // Must be malformed path, ending with "."

	// Tokenize remainder, always looking ahead for end
	string nextToken = tokenStream.Tokenize(".", curPos);
	while (!nextToken.empty()) {
		// Make sure the last step was a table
		if (value.type != ANY_TTABLE) return false;

		// Must use temporary
		ScriptAnyValue getter;
		value.table->GetValueAny(token, getter);
		value = getter;	

		// Advance to the token ahead
		token = nextToken;
		nextToken = tokenStream.Tokenize(".", curPos);
	}

	// Make sure the last step was a table
	if (value.type != ANY_TTABLE) return false;

	// We should be left on the final token
	value.table->SetValueAny(token, newValue);

	// Delete copy and return
	return true;
}
