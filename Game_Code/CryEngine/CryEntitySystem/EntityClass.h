////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   EntityClass.h
//  Version:     v1.00
//  Created:     18/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntityClass_h__
#define __EntityClass_h__
#pragma once

#include <IEntityClass.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Standard implementation of the IEntityClass interface.
//////////////////////////////////////////////////////////////////////////
class CEntityClass : public IEntityClass
{
public:
	CEntityClass();
	virtual ~CEntityClass();
	
	//////////////////////////////////////////////////////////////////////////
	// IEntityClass interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { delete this; };

	VIRTUAL uint32 GetFlags() const { return m_nFlags; };
	VIRTUAL void SetFlags( uint32 nFlags ) { m_nFlags = nFlags; };
	
	VIRTUAL const char* GetName() const { return m_sName.c_str(); }
	VIRTUAL const char* GetScriptFile() const { return m_sScriptFile.c_str(); }

	VIRTUAL IEntityScript* GetIEntityScript() const { return m_pEntityScript; }
	VIRTUAL IScriptTable* GetScriptTable() const;
	VIRTUAL bool LoadScript( bool bForceReload );
	VIRTUAL UserProxyCreateFunc GetUserProxyCreateFunc() const { return m_pfnUserProxyCreate; };
	VIRTUAL void* GetUserProxyData() const { return m_pUserProxyUserData; };

	VIRTUAL IEntityPropertyHandler* GetPropertyHandler() const;
	VIRTUAL IEntityEventHandler* GetEventHandler() const;
	VIRTUAL IEntityScriptFileHandler* GetScriptFileHandler() const;

	VIRTUAL const SEditorClassInfo& GetEditorClassInfo() const;
	VIRTUAL void SetEditorClassInfo(const SEditorClassInfo& editorClassInfo);

	VIRTUAL int GetEventCount();
	VIRTUAL IEntityClass::SEventInfo GetEventInfo( int nIndex );
	VIRTUAL bool FindEventInfo( const char *sEvent,SEventInfo &event );

	//////////////////////////////////////////////////////////////////////////

	void SetName( const char *sName );
	void SetScriptFile( const char *sScriptFile );
	void SetEntityScript( IEntityScript *pScript );

	void SetUserProxyCreateFunc( UserProxyCreateFunc pFunc, void *pUserData=NULL );
	void SetPropertyHandler( IEntityPropertyHandler* pPropertyHandler );
	void SetEventHandler( IEntityEventHandler* pEventHandler );
	void SetScriptFileHandler( IEntityScriptFileHandler* pScriptFileHandler );

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_sName);
		pSizer->AddObject(m_sScriptFile);
		pSizer->AddObject(m_pEntityScript);
		pSizer->AddObject(m_pPropertyHandler);
		pSizer->AddObject(m_pEventHandler);
		pSizer->AddObject(m_pScriptFileHandler);
	}
private:
	uint32 m_nFlags;
	string m_sName;
	string m_sScriptFile;
	IEntityScript *m_pEntityScript;

	UserProxyCreateFunc m_pfnUserProxyCreate;
	void*  m_pUserProxyUserData;

	bool m_bScriptLoaded;

	IEntityPropertyHandler* m_pPropertyHandler;
	IEntityEventHandler* m_pEventHandler;
	IEntityScriptFileHandler* m_pScriptFileHandler;

	SEditorClassInfo m_EditorClassInfo;
};

#endif // __EntityClass_h__
