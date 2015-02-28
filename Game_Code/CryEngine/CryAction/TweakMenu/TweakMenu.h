/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMenu

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

Notes:
	I don't like this method of accessing the container. Need a really simple general purpose iterator

*************************************************************************/

#ifndef __CTWEAKMENU_H__
#define __CTWEAKMENU_H__

#pragma once

#include "TweakCommon.h"

//-------------------------------------------------------------------------
// Forward declarations

class CTweakTraverser;
class CTweakMetadata;

//-------------------------------------------------------------------------


class CTweakMenu : public CTweakCommon
{
public:
	friend class CTweakMenuController;
	friend class CTweakTraverser;

	CTweakMenu(CTweakMenuController* pController);
	~CTweakMenu();

	virtual string GetValue(void) { return ""; }
	virtual bool DecreaseValue(void) { return false; }
	virtual bool IncreaseValue(void) { return false; }

	// Get a traverser 
	CTweakTraverser GetTraverser(void);

	// Register a Traverser with this menu
	void RegisterTraverser( CTweakTraverser * traverser );

	// Deregister a Traverser with this menu
	bool DeregisterTraverser( CTweakTraverser * traverser );

	void InitMenu(XmlNodeRef xml);

protected:

	CTweakMetadata* CreateItem(ETweakType type, const string& command);
	void AddItem(CTweakCommon* pItem);
	void RemoveItem(CTweakCommon* pItem);

	void SetName(const string& name) { m_sName = name; }
	void SetDescription(const string& desc) { m_sDescription = desc; }

	// Items in this menu
	std::vector<CTweakCommon*> m_items;

	// Traversers associated with this menu
	std::set<CTweakTraverser*> m_traversers;
};

#endif  // __CTWEAKMENU_H__
