/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Allows getting structured Tweak metadata, input from LUA script

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMenu.h"
#include "TweakTraverser.h"
#include "IScriptSystem.h"

#include "TweakMetadata.h"
#include "TweakMetadataCallback.h"
#include "TweakMetadataCommand.h"
#include "TweakMetadataCVAR.h"
#include "TweakMetadataGametoken.h"
#include "TweakMetadataLUA.h"
#include "TweakMetadataVariable.h"

//-------------------------------------------------------------------------

CTweakMenu::CTweakMenu(CTweakMenuController* pController)
: CTweakCommon(pController)
{
	m_type = eTT_Menu;
}

//-------------------------------------------------------------------------

CTweakMenu::~CTweakMenu()
{
	// Deregister all the Traversers
	// * When Deregistered a Traverser causes itself to be removed from the set. 
	//   Therefore we cannot iterate through the set, because our iterator would be invalidated each time.
	// * Deletion is left to whoever owns the Traverser.
	while (!m_traversers.empty())
	{	
		CTweakTraverser *pTraverser = *(m_traversers.begin());
		pTraverser->Deregister();
	}
	
	// Delete child Tweak items
	for (std::vector<CTweakCommon*>::iterator it = m_items.begin(); it != m_items.end(); ++it) 
		SAFE_DELETE(*it);
}


//-------------------------------------------------------------------------

// Register a Traverser with this menu
void CTweakMenu::RegisterTraverser( CTweakTraverser * p_traverser )
{
	m_traversers.insert(p_traverser);
}

//-------------------------------------------------------------------------

// Deregister a Traverser with this menu
bool CTweakMenu::DeregisterTraverser( CTweakTraverser * p_traverser  )
{
	return ( 0 != m_traversers.erase(p_traverser) );
}

//-------------------------------------------------------------------------

void CTweakMenu::InitMenu(XmlNodeRef xmlNode)
{
	m_sName = xmlNode->getAttr("title");
	m_sDescription = xmlNode->getAttr("description");

	int childCount = xmlNode->getChildCount();
	for(int i=0; i<childCount; ++i)
	{
		XmlNodeRef childXml = xmlNode->getChild(i);
		if(!stricmp(childXml->getTag(), "Menu"))
		{
			CTweakMenu* pMenu = new CTweakMenu(m_pMenuController);
			pMenu->InitMenu(childXml);
			AddItem(pMenu);
		}
		else if(!stricmp(childXml->getTag(), "Entry"))
		{
			XmlString command;
			ETweakType type=(ETweakType)0;
			if(childXml->getAttr("cvar", command))
			{
				type = eTT_Cvar;
			}
			else if(childXml->getAttr("command", command))
			{
				type = eTT_Command;
			}
			else if(childXml->getAttr("lua", command))
			{
				type = eTT_Lua;
			}
			else if(childXml->getAttr("gametoken", command))
			{
				type = eTT_GameToken;
			}
			else
			{
				assert(false);
			}

			CTweakMetadata* pItem = CreateItem(type, command);
			pItem->InitItem(childXml);
			AddItem(pItem);
		}
	}
}


CTweakMetadata* CTweakMenu::CreateItem(ETweakType type, const string& command)
{
	CTweakMetadata* pItem = NULL;

	switch(type)
	{
	case eTT_Cvar:
		pItem = new CTweakMetadataCVAR(m_pMenuController, command);
		break;

	case eTT_Command:
		pItem = new CTweakMetadataCommand(m_pMenuController, command);
		break;

	case eTT_Lua:
		pItem = new CTweakMetadataLUA(m_pMenuController, command);
		break;

	case eTT_GameToken:
		pItem = new CTweakMetadataGameToken(m_pMenuController, command);
		break;

	case eTT_CodeVariable:
		pItem = new CTweakMetadataVariable(m_pMenuController, command);
		break;

	case eTT_Callback:
		pItem = new CTweakMetadataCallback(m_pMenuController, command);
		break;

	default:
		assert(false);
		break;
	}


	return pItem;
}

//-------------------------------------------------------------------------

void CTweakMenu::AddItem(CTweakCommon *pItem)
{
	if(pItem)
		m_items.push_back(pItem);
}

//-------------------------------------------------------------------------

void CTweakMenu::RemoveItem(CTweakCommon* pItem)
{
	if(pItem)
	{
		stl::find_and_erase(m_items, pItem);
	}
}

//-------------------------------------------------------------------------

CTweakTraverser CTweakMenu::GetTraverser(void)
{
	return CTweakTraverser(this);
}

//-------------------------------------------------------------------------
