/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakTraverser

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/


#ifndef __CTWEAKTRAVERSER_H__
#define __CTWEAKTRAVERSER_H__

#pragma once

//-------------------------------------------------------------------------

class CTweakCommon;
class CTweakMenu;

#include "ITweakMenuController.h"

//-------------------------------------------------------------------------

class CTweakTraverser
{
public:

	friend class CTweakMenu;

	// Create an empty Traverser
	CTweakTraverser(void);

	// Create a Traverser, registering it in the process
	CTweakTraverser(CTweakMenu *root);

	// Copy a Traverser, registering the copy
	CTweakTraverser(const CTweakTraverser &that);

	// Copy a Traverser, registering the copy
	void operator= (const CTweakTraverser &that);

	// Deregister and destroy
	~CTweakTraverser();

	// Compare Traversers to see if they point to the same item
	// If both unregistered, returns true
	bool operator== (const CTweakTraverser &that);

	void Register(CTweakMenu * root);
	void Deregister(void);
	bool IsRegistered(void) const;

	bool Previous(void);		// Go to previous item (true) or to before-start (false)
	bool Next(void);				// Go to next item (true) or after-end (false)
	bool Forward(void);			// If a submenu item move into it (true) or do nothing (false)
	bool Back(void);				// If above root go back one level (true) or do nothing (false)
	void Top(void);					// Go to before-start
	void Bottom(void);			// Go to after-end
	bool First(void);				// Go to first item if any (true) or to before-start (false)
	bool Last(void);				// Go to last item if any (true) or to after-end (false)

	// Scrolling properties
	void SetLineVisibleCount(int nLineVisibleCount) { m_nLineVisibleCount = nLineVisibleCount; }
	int GetLineVisibleCount() const { return m_nLineVisibleCount; }
	int GetLineStartOffset() const { return m_nLineStartOffset; }

	// If you can find this item go there (true) or do nothing (false)
	bool Goto(const char *name);

	// find items by name / id 
	CTweakCommon* Find(const char* name) const;
	CTweakCommon* Find(TTweakableItemId id) const;

	// Return whatever item we are currently pointing to
	CTweakCommon *GetItem(void) const;
	float GetSelectionTimer() const { return m_fSelectionTimer; }

	// Return the submenu we are currently in
	CTweakMenu *GetMenu(void) const;

	// Return number of items in current submenu
	float GetMenuSize(void) const;

	// Remove the item currently pointed at from menu, 
	// Returns item so caller can delete it (to maintain consistent create/delete contract)
	CTweakCommon *Remove(void);

	// Insert a new item into the current position, return true on success
	bool Insert(CTweakCommon *pItem);

	// Update internal info
	void Update(float fDeltaTime);


protected:
	typedef std::vector<CTweakCommon*> TItems;
	typedef std::vector<CTweakMenu*> TMenus;

	// Wrapped fetch of menu vector from the relevant CTweakMenu instance
	TItems &GetMenuItems(void) const;

	// Pointers to submenus along our path
	// The traverser is only valid while we at least have a root for the path
	TMenus m_menuPath;

	// The index of the current item
	int m_index;

	// Scrolling
	int m_nLineVisibleCount;
	int m_nLineStartOffset;

	// Selection persistant info
	float m_fSelectionTimer;

	const static int START_INDEX;
};

#endif // __CTWEAKCOMMON_H__