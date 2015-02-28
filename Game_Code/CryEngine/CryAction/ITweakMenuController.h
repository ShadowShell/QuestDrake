/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Interface for using a Tweak Menu controller

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/

#ifndef __ITWEAKMENUCONTROLLER_H__
#define __ITWEAKMENUCONTROLLER_H__

#pragma once

//! Tweak input action names
enum ETweakAction
{
	eTA_Enter,		// Enter menu
	eTA_Exit,		// Exit menu
	eTA_Left,		// Navigate left (to parent menu)
	eTA_Right,		// Navigate right (to child menu)
	eTA_Up,			// Navigate up (previous entry)
	eTA_Down,		// Navigate down (next entry)
	eTA_Inc,		// Increment current value
	eTA_Dec,		// Decrement current value
};

typedef uint32 TTweakableItemId;
const TTweakableItemId INVALID_TWEAK_ITEM = -1;

// callback function type 
typedef void (*TweakCallbackFunction)( void* );

struct ITweakMenuController
{
	virtual ~ITweakMenuController() {}
	virtual void Release() = 0;

	//! Update - Update the controller
	virtual void Update(float fDeltaTime) = 0;

	//! OnAction - Handle input action (see ActionMapManager)
	virtual bool OnAction(ETweakAction eTweakAction, int nActivationMode, float fValue) = 0;

	// Adds a menu item for a variable somewhere in code. Caller is responsible for removing the item, by storing 
	//	the returned variable id and then calling RemoveTweakableValue()
	//	menuPath: can specify sub menus by using '.' - eg "Camera.fov" creates an "fov" item in the "Camera" menu.
	virtual TTweakableItemId AddTweakableValue(const char* menuPath, int* pValue, float delta = 1, float min = 0, float max = 100.0f, const char* settings=NULL) = 0;
	virtual TTweakableItemId AddTweakableValue(const char* menuPath, float* pValue, float delta = 1, float min = 0, float max = 100.0f) = 0;

	// Adds a menu item which calls a callback function. Userdata is passed back with the callback.
	virtual TTweakableItemId AddCallbackItem(const char* menuPath, TweakCallbackFunction pFunction, void* pUserData) = 0;

	virtual bool RemoveTweakableValue(TTweakableItemId id) = 0;

	virtual void GetMemoryStatistics(ICrySizer * s) = 0;
};

#endif //__ITWEAKMENUCONTROLLER_H__