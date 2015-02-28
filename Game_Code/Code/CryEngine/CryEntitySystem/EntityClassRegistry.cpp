////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   EntityClassRegistry.cpp
//  Version:     v1.00
//  Created:     3/8/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EntityClassRegistry.h"
#include "EntityClass.h"
#include "EntityScript.h"
#include <CryFile.h>

//////////////////////////////////////////////////////////////////////////
CEntityClassRegistry::CEntityClassRegistry()
{
	m_pSystem = GetISystem();
}

//////////////////////////////////////////////////////////////////////////
CEntityClassRegistry::~CEntityClassRegistry()
{
}

//////////////////////////////////////////////////////////////////////////
bool CEntityClassRegistry::RegisterClass( IEntityClass *pClass )
{
	assert( pClass != NULL );

	IEntityClass *pOldClass = FindClass(pClass->GetName());
	if (pOldClass)
	{
		EntityWarning( "CEntityClassRegistry::RegisterClass failed, class with name %s already registered",
			pOldClass->GetName() );
		return false;
	}
	m_mapClassName[pClass->GetName()] = pClass;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityClassRegistry::UnregisterClass( IEntityClass *pClass )
{
	assert( pClass != NULL );
	if (FindClass(pClass->GetName()))
	{
		m_mapClassName.erase(pClass->GetName());
		pClass->Release();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
IEntityClass* CEntityClassRegistry::FindClass( const char *sClassName ) const
{
	ClassNameMap::const_iterator it = m_mapClassName.find(CONST_TEMP_STRING(sClassName));

	if(it==m_mapClassName.end())
		return 0;

	return it->second;
}

//////////////////////////////////////////////////////////////////////////
IEntityClass* CEntityClassRegistry::GetDefaultClass() const
{
	return m_pDefaultClass;
}

//////////////////////////////////////////////////////////////////////////
IEntityClass* CEntityClassRegistry::RegisterStdClass( const SEntityClassDesc &entityClassDesc )
{
	// Creates a new entity class.
	CEntityClass *pClass = new CEntityClass;
	pClass->SetName(entityClassDesc.sName);
	pClass->SetFlags(entityClassDesc.flags);
	pClass->SetScriptFile(entityClassDesc.sScriptFile);
	pClass->SetUserProxyCreateFunc(entityClassDesc.pUserProxyCreateFunc, entityClassDesc.pUserProxyData);
	pClass->SetPropertyHandler(entityClassDesc.pPropertyHandler);
	pClass->SetEventHandler(entityClassDesc.pEventHandler);
	pClass->SetScriptFileHandler(entityClassDesc.pScriptFileHandler);
	pClass->SetEditorClassInfo(entityClassDesc.editorClassInfo);

	// Check if need to create entity script.
	if (entityClassDesc.sScriptFile[0])
	{
		// Create a new entity script.
		CEntityScript *pScript = new CEntityScript;
		if (!pScript->Init(entityClassDesc.sName,entityClassDesc.sScriptFile))
		{
			EntityWarning( "EntityScript %s failed to initialize",entityClassDesc.sScriptFile );
			pScript->Release();
			pClass->Release();
			return NULL;
		}
		pClass->SetEntityScript( pScript );
	}

	if (!RegisterClass( pClass ))
	{
		// Register class failed.
		pClass->Release();
		return NULL;
	}
	return pClass;
}

//////////////////////////////////////////////////////////////////////////
void CEntityClassRegistry::IteratorMoveFirst()
{
	m_currentMapIterator = m_mapClassName.begin();
}

//////////////////////////////////////////////////////////////////////////
IEntityClass* CEntityClassRegistry::IteratorNext()
{
	IEntityClass *pClass = NULL;
	if (m_currentMapIterator != m_mapClassName.end())
	{
		pClass = m_currentMapIterator->second;
		++m_currentMapIterator;
	}
	return pClass;
}

//////////////////////////////////////////////////////////////////////////
void CEntityClassRegistry::InitializeDefaultClasses()
{
	LoadClasses( "Entities" );

	SEntityClassDesc stdClass;
	stdClass.flags |= ECLF_INVISIBLE|ECLF_DEFAULT;
	stdClass.sName = "Default";
	m_pDefaultClass = RegisterStdClass( stdClass );

	SEntityClassDesc stdGeomClass;
	stdGeomClass.flags |= ECLF_INVISIBLE|ECLF_DEFAULT;
	stdGeomClass.sName = "GeomEntity";
	stdGeomClass.sScriptFile = "Scripts/Entities/Default/GeomEntity.lua";
	RegisterStdClass( stdGeomClass );

	SEntityClassDesc stdRopeClass;
	stdRopeClass.flags |= ECLF_INVISIBLE|ECLF_DEFAULT;
	stdRopeClass.sName = "RopeEntity";
	stdRopeClass.sScriptFile = "Scripts/Entities/Default/RopeEntity.lua";
	RegisterStdClass( stdRopeClass );

	SEntityClassDesc stdFlowgraphClass;
	stdFlowgraphClass.flags |= ECLF_DEFAULT;
	stdFlowgraphClass.sName = "FlowgraphEntity";
	stdFlowgraphClass.editorClassInfo.sIcon = "FlowgraphEntity.bmp";
	RegisterStdClass( stdFlowgraphClass );

  SEntityClassDesc stdListenerClass;
  stdListenerClass.flags |= ECLF_INVISIBLE|ECLF_DEFAULT;
  stdListenerClass.sName = "SoundListener";
  RegisterStdClass( stdListenerClass );
}

//////////////////////////////////////////////////////////////////////////
void CEntityClassRegistry::LoadClasses( const char *sRootPath,bool bOnlyNewClasses )
{
	ICryPak *pCryPak = gEnv->pCryPak;
	_finddata_t fd;
	char filename[_MAX_PATH];

	string sPath = sRootPath;
	sPath.TrimRight("/\\");
	string sSearch = sPath + "/*.ent";
	intptr_t handle = pCryPak->FindFirst( sSearch, &fd, 0);
	if (handle != -1)
	{
		int res = 0;
		do
		{
			// Animation file found, load it.
			strcpy(filename,sPath);
			strcat(filename,"/");
			strcat(filename,fd.name);

			// Load xml file.
			XmlNodeRef root = m_pSystem->LoadXmlFromFile(filename);
			if (root)
			{
				LoadClassDescription(root,bOnlyNewClasses);
			}

			res = pCryPak->FindNext( handle,&fd );
		} while (res >= 0);
		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityClassRegistry::LoadClassDescription( XmlNodeRef &root,bool bOnlyNewClasses )
{
	assert( root != (IXmlNode*)NULL );
	if (root->isTag("Entity"))
	{
		const char *sName = root->getAttr("Name");
		if (*sName == 0)
			return; // Empty name.

		const char *sScript = root->getAttr("Script");

		IEntityClass *pClass = FindClass( sName );
		if (!pClass)
		{
			// New class.
			SEntityClassDesc cd;
			cd.flags = 0;
			cd.sName = sName;
			cd.sScriptFile = sScript;

			bool bInvisible = false;
			if (root->getAttr("Invisible",bInvisible))
			{
				if (bInvisible)
					cd.flags |= ECLF_INVISIBLE;
			}

			bool bBBoxSelection = false;
			if (root->getAttr("BBoxSelection",bBBoxSelection))
			{
				if (bBBoxSelection)
					cd.flags |= ECLF_BBOX_SELECTION;
			}

			RegisterStdClass( cd );
		}
		else
		{
			// This class already registered.
			if (!bOnlyNewClasses)
			{
				EntityWarning( "[CEntityClassRegistry] LoadClassDescription failed, Entity Class name %s already registered",sName );
			}
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IEntityClassRegistry)
