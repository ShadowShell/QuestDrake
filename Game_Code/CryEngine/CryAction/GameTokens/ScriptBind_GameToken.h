#pragma once
#include "ScriptHelpers.h"
#include "IGameTokens.h"
#include "GameTokenSystem.h"

/*! <description>This class implements script functions for game tokens.</description> */
class CScriptBind_GameToken :
	public CScriptableBase
{
public:
	CScriptBind_GameToken( CGameTokenSystem *pTokenSystem )
	{
		m_pTokenSystem = pTokenSystem;

		Init( gEnv->pScriptSystem, GetISystem() );
		SetGlobalName("GameToken");

#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME &CScriptBind_GameToken::

		SCRIPT_REG_FUNC(SetToken);
		SCRIPT_REG_TEMPLFUNC(GetToken, "sTokenName");
		SCRIPT_REG_FUNC(DumpAllTokens);
	}

	virtual ~CScriptBind_GameToken(void)
	{
	}

	void Release();

	//! <code>GameToken.SetToken( TokenName, TokenValue )</code>
	int SetToken( IFunctionHandler *pH);

	//! <code>GameToken.GetToken( TokenName )</code>
	int GetToken( IFunctionHandler *pH, const char *sTokenName );

	//! <code>GameToken.DumpAllTokens()</code>
	int DumpAllTokens( IFunctionHandler* pH);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const;

private:
	CGameTokenSystem *m_pTokenSystem;
};

