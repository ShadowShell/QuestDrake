////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2010.
// -------------------------------------------------------------------------
//  File name:   FlashUIAction.h
//  Version:     v1.00
//  Created:     10/9/2010 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FlashUIAction_H__
#define __FlashUIAction_H__

#include <IFlashUI.h>
#include <IFlowSystem.h>
#include <CryListenerSet.h>

class CFlashUIAction : public IUIAction
{
public:
	CFlashUIAction(EUIActionType type);
	VIRTUAL ~CFlashUIAction();

	VIRTUAL EUIActionType GetType() const { return m_type; }

	VIRTUAL const char* GetName() const { return m_sName.c_str(); }
	VIRTUAL void SetName( const char* sName ) { m_sName = sName; }

	VIRTUAL bool Init();
	VIRTUAL bool IsValid() const { return m_bIsValid; }

	VIRTUAL void SetEnabled(bool bEnabled);
	VIRTUAL bool IsEnabled() const { return m_bEnabled && m_bIsValid; }

	VIRTUAL IFlowGraphPtr GetFlowGraph() const { CRY_ASSERT_MESSAGE(m_type == eUIAT_FlowGraph, "Try to access Flowgraph of Lua UI Action"); return m_pFlowGraph; }
	VIRTUAL SmartScriptTable GetScript() const { CRY_ASSERT_MESSAGE(m_type == eUIAT_LuaScript, "Try to access ScriptTable of FG UI Action"); return m_pScript; }

	VIRTUAL bool Serialize( XmlNodeRef& xmlNode, bool bIsLoading );
	VIRTUAL bool Serialize( const char* scriptFile, bool bIsLoading );

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

	bool ReloadScript();

	void SetValid( bool bValid ) { m_bIsValid = bValid; }

	void Update();

	// for script actions
	void StartScript(const SUIArguments& args);
	void EndScript();

private:
	string m_sName;
	string m_sScriptFile;
	IFlowGraphPtr	m_pFlowGraph;
	SmartScriptTable m_pScript;
	bool m_bIsValid;
	bool m_bEnabled;
	EUIActionType m_type;
	enum EScriptFunction
	{
		eSF_OnInit,
		eSF_OnStart,
		eSF_OnUpdate,
		eSF_OnEnd,
		eSF_OnEnabled
	};
	std::map<EScriptFunction, bool> m_scriptAvail;
};

//--------------------------------------------------------------------------------------------
struct CUIActionManager : public IUIActionManager
{
public:
	CUIActionManager() : m_listener(32), m_bAcceptRequests(true) {}
	void Init();

	VIRTUAL void StartAction( IUIAction* pAction, const SUIArguments& args );
	VIRTUAL void EndAction( IUIAction* pAction, const SUIArguments& args );

	VIRTUAL void EnableAction( IUIAction* pAction, bool bEnable );

	VIRTUAL void AddListener( IUIActionListener* pListener, const char* name );
	VIRTUAL void RemoveListener( IUIActionListener* pListener );

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

	void Update();

private:
	void StartActionInt( IUIAction* pAction, const SUIArguments& args );
	void EndActionInt( IUIAction* pAction, const SUIArguments& args );
	void EnableActionInt( IUIAction* pAction, bool bEnable );

private:
	typedef CListenerSet< IUIActionListener* > TActionListener;
	typedef std::map< IUIAction*, bool > TActionMap;
	typedef std::map< IUIAction*, SUIArguments > TActionArgMap;

	TActionListener m_listener;
	TActionMap m_actionStateMap;
	TActionMap m_actionEnableMap;
	TActionArgMap m_actionStartMap;
	TActionArgMap m_actionEndMap;
	bool m_bAcceptRequests;
};

#endif // #ifndef __FlashUIAction_H__