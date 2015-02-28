/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMenuController

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/

#ifndef __TWEAKMENUCONTROLLER_H__
#define __TWEAKMENUCONTROLLER_H__

#pragma once

#include "TweakMenu.h"
#include "TweakTraverser.h"
#include "ITweakMenuController.h"


//-----------------------------------------------------------------------------------------------------

class CTweakMenuController : public ITweakMenuController
{
public:
	CTweakMenuController();
	
	virtual ~CTweakMenuController(void);

	void Init();
	virtual void Release();

	virtual void Update(float fDeltaTime);

	virtual bool OnAction(ETweakAction eTweakAction, int activationMode, float value);

	virtual TTweakableItemId AddTweakableValue(const char* menuPath, int* pValue, float delta = 1, float min = 0, float max = 100.0f, const char* settings = NULL);
	virtual TTweakableItemId AddTweakableValue(const char* menuPath, float* pValue, float delta = 1, float min = 0, float max = 100.0f);

	virtual TTweakableItemId AddCallbackItem(const char* menuPath, TweakCallbackFunction pFunction, void* pUserData);

	virtual bool RemoveTweakableValue(TTweakableItemId id);

	void SaveProfile();

	void GetMemoryStatistics(ICrySizer * s);

	bool IsActive() const { return m_bActive; }

	TTweakableItemId RegisterItem() { return m_nextVariableId++; }

protected:
	// Colors to use 
	enum ETextColor
	{
		eTC_Red,
		eTC_Green,
		eTC_Blue,
		eTC_Yellow,
		eTC_Cyan,
		eTC_Magenta,
		eTC_White,
	};

	// Methods to use to draw text
	enum EDrawTextMode
	{
		eDTM_Normal,
		eDTM_Wrap,
		eDTM_Scroll,
	};

	// Draw the menu
	void DrawMenu();

	// Reset the menu pane
	void ResetMenuPane(void);

	// Print line to menu pane
	void PrintToMenuPane( const char * line, ETextColor colour, float alpha=1.f, bool isSelected=false, int indentLevel=0, EDrawTextMode eMode=eDTM_Normal );

	// Return a string representing the current path through the menu
	string GetMenuPath(void) const;

	// Compile and execute a trivial Lua command
	// Use with caution
	bool ExecuteString(const char *sBuffer) const;

	CTweakCommon* CreateSubMenuItem(CTweakMenu* pParent, string& menuPath, CTweakCommon::ETweakType type);

	// The root Tweak menu
	CTweakMenu *m_pMenu;

	// Traverser to keep place in the menu
	CTweakTraverser m_traverser;	

	// Dimensions of menu pane
	float m_fX, m_fY;
	float m_fWidth, m_fHeight, m_fDescriptionHeight;

	// Font size and spacing
	float m_fFontSize, m_fVertSpacing;

	// State of console
	int m_nLineCount;

	// black texture
	uint32 m_nBlackTexID;

	IFFont *m_pDefaultFont;

	// Is the Tweak menu visible?
	bool m_bActive;

	// For showing hidden options
	bool m_bShowHidden;

	// rolling id for items registered from code
	TTweakableItemId m_nextVariableId;
};

#endif //__TWEAKMENUCONTROLLER_H__
