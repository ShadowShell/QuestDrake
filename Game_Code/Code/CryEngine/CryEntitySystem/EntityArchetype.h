////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   EntityArchetype.h
//  Version:     v1.00
//  Created:     19/9/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntityArchetype_h__
#define __EntityArchetype_h__
#pragma once

#include <IScriptSystem.h>

class CEntityClass;

//////////////////////////////////////////////////////////////////////////
class CEntityArchetype : public IEntityArchetype, public _i_reference_target_t
{
public:
	CEntityArchetype( IEntityClass *pClass );

	//////////////////////////////////////////////////////////////////////////
	// IEntityArchetype
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IEntityClass* GetClass() const { return m_pClass; };
	const char* GetName() const { return m_name.c_str(); };
	IScriptTable* GetProperties() { return m_pProperties; };
	XmlNodeRef GetObjectVars() { return m_ObjectVars; };
	void LoadFromXML( XmlNodeRef &propertiesNode, XmlNodeRef &objectVarsNode );
	//////////////////////////////////////////////////////////////////////////

	void SetName( const string &sName ) { m_name = sName; };
	
private:
	string m_name;
	SmartScriptTable m_pProperties;
	XmlNodeRef m_ObjectVars;
	IEntityClass* m_pClass;
};

//////////////////////////////////////////////////////////////////////////
// Manages collection of the entity archetypes.
//////////////////////////////////////////////////////////////////////////
class CEntityArchetypeManager
{
public:
	IEntityArchetype* CreateArchetype( IEntityClass *pClass,const char *sArchetype );
	IEntityArchetype* FindArchetype( const char *sArchetype );
	IEntityArchetype* LoadArchetype( const char *sArchetype );

	void Reset();

private:
	bool LoadLibrary( const string &library );
	string GetLibraryFromName( const string &sArchetypeName );

	typedef std::map<const char*,_smart_ptr<CEntityArchetype>,stl::less_stricmp<const char*> > ArchetypesNameMap;
	ArchetypesNameMap m_nameToArchetypeMap;

	DynArray<string> m_loadedLibs;
};

#endif // __EntityArchetype_h__
