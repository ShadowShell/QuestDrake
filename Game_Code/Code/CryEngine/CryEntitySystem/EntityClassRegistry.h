////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   EntityClassRegistry.h
//  Version:     v1.00
//  Created:     3/8/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntityClassRegistry_h__
#define __EntityClassRegistry_h__
#pragma once

#include <IEntityClass.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Standard implementation of the IEntityClassRegistry interface.
//////////////////////////////////////////////////////////////////////////
class CEntityClassRegistry : public IEntityClassRegistry
{
public:
	CEntityClassRegistry();
	~CEntityClassRegistry();

	bool RegisterClass( IEntityClass *pClass );
	bool UnregisterClass( IEntityClass *pClass );

	IEntityClass* FindClass( const char *sClassName ) const;
	IEntityClass* GetDefaultClass() const;

	IEntityClass* RegisterStdClass( const SEntityClassDesc &entityClassDesc );

	void LoadClasses( const char *sRootPath,bool bOnlyNewClasses=false );

	//////////////////////////////////////////////////////////////////////////
	// Registry iterator.
	//////////////////////////////////////////////////////////////////////////
	void IteratorMoveFirst();
	IEntityClass* IteratorNext();
	int GetClassCount() const { return m_mapClassName.size(); };

	void InitializeDefaultClasses();

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pDefaultClass);
		pSizer->AddContainer(m_mapClassName);
	}

private:
	void LoadClassDescription( XmlNodeRef &root,bool bOnlyNewClasses );

	typedef std::map<string,IEntityClass*> ClassNameMap;
	ClassNameMap m_mapClassName;

	IEntityClass *m_pDefaultClass;

	ISystem *m_pSystem;
	ClassNameMap::iterator m_currentMapIterator;
};



#endif // __EntityClassRegistry_h__

