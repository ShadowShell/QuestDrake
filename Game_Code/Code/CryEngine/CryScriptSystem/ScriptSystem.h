// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#if !defined(AFX_SCRIPTSYSTEM_H__8FCEA01B_BD85_4E4D_B54F_B09429A7CDFF__INCLUDED_)
#define AFX_SCRIPTSYSTEM_H__8FCEA01B_BD85_4E4D_B54F_B09429A7CDFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#if !defined(__GNUC__)
#include "StdAfx.h"
#endif
#include <IConsole.h>
#include <IScriptSystem.h>
extern "C"{
#include <lua.h>
#include <lauxlib.h>
}

#include "StackGuard.h"
#include "ScriptBindings/ScriptBinding.h"
#include "ScriptTimerMgr.h"

class CLUADbg;

struct SLuaStackEntry
{
	int line;
	string source;
	string description;
};

// Returns literal representation of the type value
inline const char* ScriptAnyTypeToString( ScriptAnyType type )
{
	switch (type)
	{
	case ANY_ANY: return "Any";
	case ANY_TNIL: return "Null";
	case ANY_TBOOLEAN: return "Boolean";
	case ANY_TSTRING:  return "String";
	case ANY_TNUMBER:  return "Number";
	case ANY_TFUNCTION:  return "Function";
	case ANY_TTABLE: return "Table";
	case ANY_TUSERDATA: return "UserData";
	case ANY_THANDLE: return "Pointer";
	case ANY_TVECTOR: return "Vec3";
	default: return "Unknown";
	}
}

// Returns literal representation of the type value
inline const char* ScriptVarTypeAsCStr(ScriptVarType t)
{
	switch (t)
	{
	case svtNull: return "Null";
	case svtBool: return "Boolean";
	case svtString: return "String";
	case svtNumber: return "Number";
	case svtFunction: return "Function";
	case svtObject: return "Table";
	case svtUserData: return "UserData";
	case svtPointer: return "Pointer";
	default: return "Unknown";
	}
}

typedef std::set<string, stl::less_stricmp<string> > ScriptFileList;
typedef ScriptFileList::iterator ScriptFileListItor;

//////////////////////////////////////////////////////////////////////////
// forwarde declarations.
class CScriptSystem;
class CScriptTable;

#define SCRIPT_OBJECT_POOL_SIZE 15000

/*! IScriptSystem implementation
	@see IScriptSystem 
*/
class CScriptSystem : public IScriptSystem, public ISystemEventListener
{
public:
	//! constructor
	CScriptSystem();
	//! destructor
	virtual ~CScriptSystem();
	//!
	bool Init( ISystem *pSystem,bool bStdLibs,int nStackSize );

	void Update();
	void SetGCFrequency( const float fRate );

	void SetEnvironment(HSCRIPTFUNCTION scriptFunction, IScriptTable* pEnv);
	IScriptTable* GetEnvironment(HSCRIPTFUNCTION scriptFunction); 

	//!
	void RegisterErrorHandler(void);

	//!
	bool _ExecuteFile(const char *sFileName,bool bRaiseError, IScriptTable* pEnv = 0);
	//!

	// interface IScriptSystem -----------------------------------------------------------

	VIRTUAL bool ExecuteFile(const char *sFileName,bool bRaiseError,bool bForceReload, IScriptTable* pEnv = 0);
	VIRTUAL bool ExecuteBuffer(const char *sBuffer, size_t nSize,const char *sBufferDescription, IScriptTable* pEnv = 0);
	VIRTUAL void UnloadScript(const char *sFileName);
	VIRTUAL void UnloadScripts();
	VIRTUAL bool ReloadScript(const char *sFileName,bool bRaiseError);
	VIRTUAL bool ReloadScripts();
	VIRTUAL void DumpLoadedScripts();
	
	VIRTUAL IScriptTable* CreateTable( bool bEmpty=false );
	
	//////////////////////////////////////////////////////////////////////////
	// Begin Call.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL int BeginCall(HSCRIPTFUNCTION hFunc);
	VIRTUAL int BeginCall(const char *sFuncName);
	VIRTUAL int BeginCall(const char *sTableName,const char *sFuncName);
	VIRTUAL int BeginCall( IScriptTable *pTable, const char *sFuncName );
	
	//////////////////////////////////////////////////////////////////////////
	// End Call.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL bool EndCall();
	VIRTUAL bool EndCallAny( ScriptAnyValue &any );
	VIRTUAL bool EndCallAnyN( int n, ScriptAnyValue* anys );

	//////////////////////////////////////////////////////////////////////////
	// Get function pointer.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL HSCRIPTFUNCTION GetFunctionPtr(const char *sFuncName);
	VIRTUAL HSCRIPTFUNCTION GetFunctionPtr(const char *sTableName, const char *sFuncName);
	VIRTUAL void ReleaseFunc( HSCRIPTFUNCTION f );
	VIRTUAL HSCRIPTFUNCTION AddFuncRef(HSCRIPTFUNCTION f);
	VIRTUAL bool CompareFuncRef(HSCRIPTFUNCTION f1, HSCRIPTFUNCTION f2);
	
	VIRTUAL ScriptAnyValue CloneAny(const ScriptAnyValue& any);
	VIRTUAL void ReleaseAny(const ScriptAnyValue& any);

	//////////////////////////////////////////////////////////////////////////
	// Push function param.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void PushFuncParamAny( const ScriptAnyValue &any );

	//////////////////////////////////////////////////////////////////////////
	// Set global value.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetGlobalAny( const char *sKey,const ScriptAnyValue &any );
	VIRTUAL bool GetGlobalAny( const char *sKey,ScriptAnyValue &any );

	VIRTUAL IScriptTable * CreateUserData(void * ptr, size_t size);
	VIRTUAL void ForceGarbageCollection();
	VIRTUAL int GetCGCount();
	VIRTUAL void SetGCThreshhold(int nKb);
	virtual void Release();
	VIRTUAL void ShowDebugger(const char *pszSourceFile, int iLine, const char *pszReason);
	VIRTUAL HBREAKPOINT AddBreakPoint(const char *sFile,int nLineNumber);
	VIRTUAL IScriptTable *GetLocalVariables(int nLevel = 0);
	VIRTUAL IScriptTable *GetCallsStack();// { return 0; };
	VIRTUAL void DumpCallStack();
	
	VIRTUAL void DebugContinue(){}
	VIRTUAL void DebugStepNext(){}
	VIRTUAL void DebugStepInto(){}
	VIRTUAL void DebugDisable(){}

	VIRTUAL BreakState GetBreakState(){return bsNoBreak;}
	VIRTUAL void GetMemoryStatistics(ICrySizer *pSizer) const;
	VIRTUAL void GetScriptHash( const char *sPath, const char *szKey, unsigned int &dwHash );
	VIRTUAL void PostInit();
	VIRTUAL void RaiseError( const char *format,... ) PRINTF_PARAMS(2, 3);
	VIRTUAL void LoadScriptedSurfaceTypes( const char *sFolder,bool bReload );
	VIRTUAL void SerializeTimers( ISerialize *pSer );
	VIRTUAL void ResetTimers( );

	VIRTUAL int GetStackSize();
	VIRTUAL uint32 GetScriptAllocSize();

	VIRTUAL void* Allocate(size_t sz);
	VIRTUAL size_t Deallocate(void* ptr);

	void PushAny( const ScriptAnyValue &var );
	bool PopAny( ScriptAnyValue &var );
	// Convert top stack item to Any.
	bool ToAny( ScriptAnyValue &var,int index );
	void PushVec3( const Vec3 &vec );
	bool ToVec3( Vec3 &vec,int index );
	// Push table reference
	void PushTable( IScriptTable *pTable );
	// Attach reference at the top of the stack to the specified table pointer.
	void AttachTable( IScriptTable *pTable );
	bool GetRecursiveAny( IScriptTable *pTable,const char *sKey,ScriptAnyValue &any );

	lua_State* GetLuaState() const { return L; }
	void TraceScriptError(const char* file, int line, const char* errorStr);
	void LogStackTrace();

	CScriptTimerMgr* GetScriptTimerMgr() { return m_pScriptTimerMgr; };

	void GetCallStack( std::vector<SLuaStackEntry> &callstack );
	bool IsCallStackEmpty(void);
	void DumpStateToFile( const char *filename );

	//////////////////////////////////////////////////////////////////////////
	// Facility to pre-catch any lua buffer
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL HSCRIPTFUNCTION CompileBuffer(const char *sBuffer, size_t nSize, const char* sBufferDesc);
	VIRTUAL int PreCacheBuffer(const char *sBuffer, size_t nSize, const char* sBufferDesc);
	VIRTUAL int BeginPreCachedBuffer(int iIndex );
	VIRTUAL void ClearPreCachedBuffer();

	//////////////////////////////////////////////////////////////////////////
	// ISystemEventListener
	//////////////////////////////////////////////////////////////////////////
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	//////////////////////////////////////////////////////////////////////////


	// Used by Lua debugger to maintain callstack exposed to C++
	void ExposedCallstackClear();
	void ExposedCallstackPush( const char * sFunction, int nLine, const char *sSource );
	void ExposedCallstackPop();

private: // ---------------------------------------------------------------------
	//!
	bool EndCallN( int nReturns );

	static int ErrorHandler(lua_State *L);

	// Create default metatables.
	void CreateMetatables();

	// Now private, doesn't need to be updated by main thread
	void EnableDebugger( ELuaDebugMode eDebugMode );
	void EnableCodeCoverage( bool enable );

	static void DebugModeChange(ICVar *cvar);
	static void CodeCoverageChange(ICVar* cvar);

	// Loaded file tracking helpers
	void AddFileToList(const char * sName);
	void RemoveFileFromList(const ScriptFileListItor &itor);

	// ----------------------------------------------------------------------------
private:
	static CScriptSystem *s_mpScriptSystem;
	lua_State *L;
	ICVar *m_cvar_script_debugger; // Stores current debugging mode
	ICVar *m_cvar_script_coverage;
	int m_nTempArg;
	int m_nTempTop;

	IScriptTable* m_pUserDataMetatable;
	IScriptTable* m_pPreCacheBufferTable;
	std::vector<string> m_vecPreCached;

	HSCRIPTFUNCTION m_pErrorHandlerFunc;

	ScriptFileList m_dqLoadedFiles;

	CScriptBindings m_stdScriptBinds;
	ISystem *m_pSystem;

	float						m_fGCFreq;				//!< relative time in seconds
	float						m_lastGCTime;			//!< absolute time in seconds
	int							m_nLastGCCount;		//!<
	int             m_forceReloadCount;

	CScriptTimerMgr* m_pScriptTimerMgr;

	// Store a simple callstack that can be inspected in C++ debugger
	const static int MAX_CALLDEPTH = 32;
	int m_nCallDepth;
	stack_string m_sCallDescriptions[MAX_CALLDEPTH];

public: // -----------------------------------------------------------------------

	string m_sLastBreakSource;		//!
	int    m_nLastBreakLine;		//!
	CLUADbg *m_pLuaDebugger;
};


#endif // !defined(AFX_SCRIPTSYSTEM_H__8FCEA01B_BD85_4E4D_B54F_B09429A7CDFF__INCLUDED_)
