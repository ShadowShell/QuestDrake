/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	CTweakMenuController - Draws the tweak menu

-------------------------------------------------------------------------
History:
- 04:02:2009  : Created by Kevin Kirst

*************************************************************************/


#include "StdAfx.h"
#include "TweakMenuController.h"
#include "CryActionCVars.h"
#include "LuaVarUtils.h"

#include "TweakItemSimple.h"

#include "TweakMetadataGametoken.h"
#include "TweakMetadataVariable.h"
#include "TweakMetadataCallback.h"
#include "IGameTokens.h"

// NOTE [Kevin] Xbox requires "*" for finding files in directory
#if defined(XENON)
	#define SEARCH_STRING "*"
#else
	#define SEARCH_STRING "*.*"
#endif
;

static const float INDENT_AMOUNT = 15.0f;

//-----------------------------------------------------------------------------------------------------

CTweakMenuController::CTweakMenuController()
{
	m_fX = 150.0f;
	m_fY = 75.0f;

	m_fWidth = 650.0f;
	m_fHeight = 500.0f;
	m_fDescriptionHeight = (m_fHeight*0.15f);
	
	m_fFontSize = 1.5f;
	m_fVertSpacing = 24.0f;

	m_bActive = false;
	m_bShowHidden = false;

	m_pMenu = NULL;
  
	m_nBlackTexID = gEnv->pRenderer->EF_LoadTexture("Textures/Gui/black.dds",FT_NOMIPS|FT_DONT_STREAM)->GetTextureID();

	m_pDefaultFont = gEnv->pCryFont->GetFont("default");
	CRY_ASSERT(m_pDefaultFont);

	m_nextVariableId = 0;
	
	Init();
}

//-----------------------------------------------------------------------------------------------------

CTweakMenuController::~CTweakMenuController(void)
{
	// Handle exiting game with tweak menu active
	if (m_bActive)
	{
		OnAction(eTA_Exit,0,0);
	}

	gEnv->pRenderer->RemoveTexture(m_nBlackTexID);

	SAFE_DELETE(m_pMenu);
}

//-----------------------------------------------------------------------------------------------------

CTweakCommon* CTweakMenuController::CreateSubMenuItem(CTweakMenu* pParent, string& menuPath, CTweakCommon::ETweakType type)
{
	if(!pParent || menuPath.empty())
		return NULL;

	CTweakMenu* pCurrentMenu = pParent;

	size_t firstDot = menuPath.find_first_of('.');
	string itemName = menuPath;

	// find existing submenu, OR create new submenu for the new path name
	CTweakTraverser t = pParent->GetTraverser();

	if(firstDot != string::npos)
	{
		itemName = menuPath.Left(firstDot);

		pCurrentMenu = static_cast<CTweakMenu*>(t.Find(itemName));

		if(!pCurrentMenu || pCurrentMenu->GetType() != CTweakCommon::eTT_Menu)
		{
			pCurrentMenu = new CTweakMenu(this);
		
			pCurrentMenu->SetName(itemName);
			pCurrentMenu->SetDescription("");

			pParent->AddItem(pCurrentMenu);
		}

		itemName = menuPath.Right(menuPath.size() - firstDot - 1);
	}

	assert(pCurrentMenu->GetType() == CTweakCommon::eTT_Menu);

	// if more '.' in name, recurse to create sub-sub-menu
	size_t offset = itemName.find_first_of('.');
	if(offset != string::npos)
	{
		return CreateSubMenuItem(pCurrentMenu, itemName, type);
	}

	// no path name found; add a menu entry
	CTweakMetadata* pItem = pCurrentMenu->CreateItem(type, itemName);
	pItem->SetName(itemName);
	pCurrentMenu->AddItem(pItem);
	return pItem;
}


void CTweakMenuController::Init() 
{
	INDENT_LOG_DURING_SCOPE(true, "While initializing tweaks menu...");

	// cleanup for re-init
	if(m_pMenu)
	{
		delete m_pMenu;
	}

	m_pMenu = new CTweakMenu(this);
	m_traverser = m_pMenu->GetTraverser();
	m_traverser.First();

	// first item should be 'reload'
	m_pMenu->AddItem(new CTweakItemReload(this));	

	// then the xml contents
	XmlNodeRef menuXml = gEnv->pSystem->LoadXmlFromFile("Libs/Tweaks/TweakMenu.xml");
	if(menuXml)
	{
		m_pMenu->InitMenu(menuXml);
	}

	// add gametokens to a specific submenu
	IGameTokenSystem* pGTS = CCryAction::GetCryAction()->GetIGameTokenSystem();
	if(pGTS)
	{
		IGameTokenIt* pIt = pGTS->GetGameTokenIterator();
		if(pIt)
		{
			CTweakMenu* pGTMenu = new CTweakMenu(this);
			pGTMenu->SetName("Game Tokens");
			pGTMenu->SetDescription("Edit current level game tokens. Use with care");

			bool tokens = false;
			while(IGameToken* pGameToken = pIt->Next())
			{
				string token = pGameToken->GetName();
				CTweakMetadata* pItem = static_cast<CTweakMetadata*>(CreateSubMenuItem(pGTMenu, token, CTweakCommon::eTT_GameToken));
				if(pItem)
					pItem->SetCommand(token);
				tokens = true;
			}
			pIt->Release();

			// only add the menu if there are subitems
			if(tokens)
			{
				m_pMenu->AddItem(pGTMenu);
			}
			else
			{
				delete pGTMenu;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::Release()
{
	// go through all items and list those which should have been unregistered by now
	if(m_pMenu)
	{
		CTweakTraverser t = m_pMenu->GetTraverser();
		t.Top();
		while(t.Next())
		{
			CTweakCommon* pItem = t.GetItem();
			if(pItem && pItem->GetId() != INVALID_TWEAK_ITEM)
			{
				CryLogAlways("Unreleased tweakable variable: %s", pItem->GetName().c_str());
			}
		}
	}

	delete this;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::Update(float fDeltaTime) 
{
	if (!m_bActive)
	{
		return;
	}
	if (fDeltaTime < FLT_EPSILON)
	{
		fDeltaTime = 0.1f;
	}

	// Draw the menu
	if (m_bActive)
	{
		m_traverser.Update(fDeltaTime);
		DrawMenu();
	}
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::DrawMenu() 
{
	IRenderer *pRenderer = gEnv->pRenderer;
	CRY_ASSERT(pRenderer);

	ResetMenuPane();

	bool displayBackground=true;
	float backgroundAlpha = 0.70f;
	if (displayBackground)
	{
		// draw a semi-transparent black box background to make
		// sure the menu text is visible on any background		
		pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);	
		const float fHeightMinusDesc = m_fHeight-m_fDescriptionHeight;
		pRenderer->Draw2dImage(80.f,50.f,m_fWidth,fHeightMinusDesc,m_nBlackTexID,0,1,1,0,0,0,0,0,backgroundAlpha);
		pRenderer->Draw2dImage(80.f,fHeightMinusDesc+55.0f,m_fWidth,m_fDescriptionHeight,m_nBlackTexID,0,1,1,0,0,0,0,0,backgroundAlpha);
	}

	// Set up renderer state
	pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONE|GS_NODEPTHTEST);

	{
		string title;
		const char *psCvar=CCryActionCVars::Get().g_TweakProfile->GetString();
		title.Format("Tweak Menu - Profile: %s - %s",psCvar ? psCvar : "",gEnv->bServer?"Server":"Client");
		PrintToMenuPane(title.c_str(), eTC_White );
	}

	// Display the menu path
	PrintToMenuPane( GetMenuPath().c_str(), eTC_Cyan );

	// Update line visible count with remaining space (in 1024x768 res)
	const float fRemainingSpace = ((m_fHeight-m_fDescriptionHeight)*(1.f/600.f)*768.f) - (m_fY+m_fVertSpacing*m_nLineCount);
	m_traverser.SetLineVisibleCount(int(fRemainingSpace/m_fVertSpacing));

	// Get a Traverser pointing at the top of this menu
	CTweakTraverser itemIter = m_traverser;
	itemIter.Top();

	string sDescription = "Description: ";
	int nItemNumber = -1;
	const int nLineVisibleCount = m_traverser.GetLineVisibleCount();
	const int nLineStartOffset = m_traverser.GetLineStartOffset();

	while (itemIter.Next())
	{
		// Skip items that are off the screen
		nItemNumber++;
		if (nItemNumber < nLineStartOffset)
		{
			float fColor[4] = {1.0f,0.0f,1.0f,1.0f};
			pRenderer->Draw2dLabel(m_fX - 15.0f, m_fY + m_fVertSpacing * m_nLineCount, m_fFontSize, fColor, false, "^");
			continue;
		}
		else if (nItemNumber > nLineVisibleCount + nLineStartOffset)
		{
			float fColor[4] = {1.0f,0.0f,1.0f,1.0f};
			pRenderer->Draw2dLabel(m_fX - 15.0f, m_fY + m_fVertSpacing * (m_nLineCount-1), m_fFontSize, fColor, false, "V");
			continue;
		}

		CTweakCommon * item = itemIter.GetItem();
		string text = item->GetName();

		if (item->IsHidden() && !m_bShowHidden)
			continue;

		// Accessorize by type
		ETextColor colour = eTC_White;
		switch (item->GetType())
		{
			case CTweakCommon::eTT_Menu:
				colour = eTC_Yellow; 
				break;
			case CTweakCommon::eTT_Callback:
			case CTweakCommon::eTT_ItemSimple:
			case CTweakCommon::eTT_Command:
				colour = eTC_White;
				break;
			default:
				colour = eTC_White; 
				text += ": " + item->GetValue();
				break;
		}
		 
		// Is this the currently selected item?
		bool isSelected = false;
		if (itemIter == m_traverser) 
		{
			isSelected = true;
			sDescription += item->GetDescription();
			colour = eTC_Red;
		}

		// Display forced draw 'hidden' items slightly transparent
		float alpha = 1.f;
		if (item->IsHidden())
			alpha = 0.5f;

		// Display it
		PrintToMenuPane( text.c_str(), colour, alpha, isSelected, 1, (isSelected?eDTM_Scroll:eDTM_Normal) );
	}

	// Draw description of selected item at bottom
	m_nLineCount += (max(nLineVisibleCount-nItemNumber,0) + 4);
	PrintToMenuPane( sDescription.c_str(), eTC_Cyan, 1.0f, false, 1, eDTM_Wrap );

	// Set back renderer state
	pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA|GS_NODEPTHTEST);
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::ResetMenuPane(void) 
{
	m_nLineCount = 0;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::PrintToMenuPane( const char * line, ETextColor color, float alpha, bool isSelected, int indentLevel, EDrawTextMode eMode ) 
{
	IRenderer *pRenderer = gEnv->pRenderer;
	CRY_ASSERT(pRenderer);

	float fColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float& fR = fColor[0];
	float& fG = fColor[1];
	float& fB = fColor[2];
	float fI = 1.0f;
	switch (color) {
		case eTC_Red:				
			fR = fI; break;		// Selection
		case eTC_Green:
			fG = fI; break;		// Tweaks
		case eTC_Blue:
			fB = fI; break;		// Path
		case eTC_Yellow:
			fR = fG = fI; break;// Submenus
		case eTC_White:
			fR = fG = fB = fI; break; // Default
		case eTC_Cyan:
			fR=0.0f; fG=1.0f; fB=1.0f; break;
		case eTC_Magenta:
			fR=1.0f; fG=0.0f; fB=1.0f; break;
	}

	float posX(m_fX);
	float posY(m_fY + m_fVertSpacing * m_nLineCount);

	float fIndentLevel = CLAMP( indentLevel, 0, 10 ) * INDENT_AMOUNT;

	if (isSelected)
	{
		alpha = 1.f; // Always draw selected options at full opacity
		pRenderer->Draw2dLabel(posX + fIndentLevel - 15.f, posY, m_fFontSize, fColor, false, ">");
	}

	int nLineCount = 0;
	int nCharOffset = 0;
	string sLine = line;
	
	const int nLineSize = strlen(line);
	STextDrawContext ctx;
	const vector2f vTextSize = m_pDefaultFont->GetTextSize(line, true, ctx);
	const float fTextWidth = vTextSize.x;
	const float fMaxTextWidth = m_fWidth-fIndentLevel*2.0f;
	int nExtractSize = int(fMaxTextWidth / (fTextWidth / (float)nLineSize));
	
	switch (eMode)
	{
		case eDTM_Scroll:
		{
			// Scroll text by incrementing character offset over time
			nCharOffset = (int)max(m_traverser.GetSelectionTimer()-2.0f,0.0f);
			nCharOffset = min(nCharOffset, max(nLineSize-nExtractSize,0));
			pRenderer->Draw2dLabel(posX + fIndentLevel, posY, m_fFontSize, fColor, false, sLine.substr(nCharOffset,nExtractSize).c_str());
			nLineCount = 1;
		}
		break;

		case eDTM_Wrap:
		{
			// Perform text wrap
			do 
			{
				string::size_type nLastSpace = (nCharOffset+nExtractSize > nLineSize ? nLineSize : sLine.find_last_of(' ', nCharOffset+nExtractSize) + 1);
				pRenderer->Draw2dLabel(posX + fIndentLevel, posY + m_fVertSpacing*nLineCount, m_fFontSize, fColor, false, sLine.substr(nCharOffset,(nLastSpace-nCharOffset)).c_str());
				nCharOffset = nLastSpace;
				nLineCount++;
			} while (nCharOffset < nLineSize);
		}
		break;

		// Just print out the whole line (up to extraction size)
		case eDTM_Normal:
		{
			pRenderer->Draw2dLabel(posX + fIndentLevel, posY, m_fFontSize, fColor, false, sLine.substr(0,nExtractSize).c_str());
			nLineCount = 1;
		}
		break;

		default:
			CRY_ASSERT_MESSAGE(0, "CTweakMenuController::PrintToMenuPane Non-implemented version of EDrawTextMode used");
	}

	m_nLineCount += nLineCount;
}


//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::OnAction(ETweakAction eTweakAction, int activationMode, float value) 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if((eTweakAction != eTA_Enter) && !m_bActive)
	{
		return false;
	}

	bool bCheckAutoSave = false;
	
	// make enter a toggle
	if (eTweakAction == eTA_Enter && m_bActive && activationMode == eAAM_OnPress)
		eTweakAction = eTA_Exit;

	// Check and perform the actions
	if (eTweakAction == eTA_Enter)
	{
		CRY_ASSERT(!m_bActive);
	
		m_bActive = true;

		if (!gEnv->bMultiplayer)
		{
			CCryAction::GetCryAction()->PauseGame(true, true);
		}
	}
	else if (eTweakAction == eTA_Exit)
	{
		CRY_ASSERT(m_bActive);

		m_bActive = false;

		if (!gEnv->bMultiplayer)
		{
			CCryAction::GetCryAction()->PauseGame(false, true);
		}
	}
	else if (eTweakAction == eTA_Left)
	{
		CRY_ASSERT(m_bActive);
		m_traverser.Back();
	}
	else if (eTweakAction == eTA_Right)
	{
		CRY_ASSERT(m_bActive);
		m_traverser.Forward();
	}
	else if (eTweakAction == eTA_Up)
	{
		CRY_ASSERT(m_bActive);

		// Keep traversing until we find an unhidden item or have tried them all
		int count = (int)m_traverser.GetMenuSize();
		do 
		{
			--count;
			if (!m_traverser.Previous())
				m_traverser.Last();
		} 
		while (count > 0 && m_traverser.GetItem()->IsHidden() && !m_bShowHidden);		 
	}
	else if (eTweakAction == eTA_Down)
	{
		CRY_ASSERT(m_bActive);

		// Keep traversing until we find an unhidden item or have tried them all
		int count = (int)m_traverser.GetMenuSize();
		do 
		{
			--count;
			if (!m_traverser.Next())
				m_traverser.First();
		} 
		while (count > 0 && m_traverser.GetItem()->IsHidden() && !m_bShowHidden);		 
	}
	else if (eTweakAction == eTA_Inc)
	{
		CRY_ASSERT(m_bActive);

		CTweakCommon *item = m_traverser.GetItem();
		if (item) 
		{
			if (item->GetType() != CTweakCommon::eTT_Menu)
			{
				item->IncreaseValue();	
				bCheckAutoSave = true;
			}
		}
	}
	else if (eTweakAction == eTA_Dec)
	{
		CRY_ASSERT(m_bActive);

		CTweakCommon *item = m_traverser.GetItem();
		if (item) 
		{
			if (item->GetType() != CTweakCommon::eTT_Menu)
			{
				item->DecreaseValue();	
				bCheckAutoSave = true;
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------

string CTweakMenuController::GetMenuPath(void) const 
{
	// Check validity
	if (!m_traverser.IsRegistered()) return "No valid menu";

	// Create a copy of the traverser we can use
	CTweakTraverser backTracker = m_traverser;

	// Form string to display of menu position
	string sPathText;
	do {
		sPathText = backTracker.GetMenu()->GetName() + "->" + sPathText;
	}	while (backTracker.Back());
	return sPathText;
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::SaveProfile() 
{
	/*string rescuePath = PathUtil::MakeFullPath( PathUtil::Make( PathUtil::GetGameFolder() + PROFILE_FOLDER, TWEAK_RESCUE ) );

	// Call Lua function to get text dump of changes table
	ExecuteString(string(LUA_FUN_SAVE) + "();");
	
	SmartScriptTable flatProfileTable;
	LuaVarUtils::GetRecursive("Tweaks.Saving.SaveDump", flatProfileTable);


	if (!flatProfileTable.GetPtr())
	{
		m_pLog->Log("[Tweaks] Failed to collect Tweak values for save");
		ExecuteString("Tweaks.Saving.SaveFailed(\"Unrecoverable save failure, nothing could be saved\")");
		return;
	}

	// Form list of files to write
	CAtomicFiles atomicFiles;
	if (!CreateProfileFileList( atomicFiles, flatProfileTable))
	{
		// Nothing to save!
		if (!autoSave) 
			ExecuteString("Tweaks.Saving.SaveFailed(\"No changes to save\");");
		return;
	}

	if (autoSave)
	{
		// Simple dump procedure, non-robust, then early-out	
		atomicFiles.AppendToFile(rescuePath, "--Autosave" );
		m_pLog->Log("[Tweaks] Autosave complete");
		return;
	} 


	// Try to lock files
	std::vector<string> failures;
	failures = atomicFiles.AcquireFiles();
	
	if (!failures.empty())
	{
		ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - perhaps read-only? See log for details\")");
		m_pLog->LogError("[Tweaks] Failed to acquire write access to files for save:");
	}
	else
	{
		// Check if the files are up-to-date
		failures = atomicFiles.CheckCurrency();
		if (!failures.empty())
		{
			ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - files have changed. See log for details\")");
			m_pLog->LogError("[Tweaks] Failed currency check on files for save:");
		}
		else
		{
			// Try to write the files
			failures = atomicFiles.WriteFiles();
			if (!failures.empty())
			{
				ExecuteString("Tweaks.Saving.SaveFailed(\"Failed - perhaps read-only? See log for details\")");
				m_pLog->LogError("[Tweaks] Failed to write files for save:");
			}
		}
	}

	if (!failures.empty())
	{
		// Log the files that failed
		for (std::vector<string>::iterator it = failures.begin(); it != failures.end(); ++it)
			m_pLog->LogError(it->c_str());
		
		// Try to append to the TweakRescue file
		if (!atomicFiles.AppendToFile( rescuePath, "--Save failed" ))
			atomicFiles.DumpToLog("Tweak value dump:");
	}
	else
	{
		m_pLog->Log("[Tweaks] Saved profile successfully");
	
		// Put them into the rescue file for consistency, don't worry if it fails
		atomicFiles.AppendToFile( rescuePath, "--Save completed" );

		// In the case of actually overwriting the previous settings, we update the timestamp for feedback
		ExecuteString("Tweaks.Saving.SaveComplete();");
	}*/
}

//-----------------------------------------------------------------------------------------------------

void CTweakMenuController::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

//-----------------------------------------------------------------------------------------------------

bool CTweakMenuController::ExecuteString(const char *sBuffer) const
{
	return gEnv->pScriptSystem->ExecuteBuffer( sBuffer, strlen(sBuffer));
}

//////////////////////////////////////////////////////////////////////////

TTweakableItemId CTweakMenuController::AddTweakableValue(const char* menuPath, int* pValue, float delta, float min, float max, const char* settings)
{
	TTweakableItemId id = INVALID_TWEAK_ITEM;

	if(m_pMenu)
	{
		string itemName = menuPath;
		CTweakMetadataVariable* pItem = static_cast<CTweakMetadataVariable*>(CreateSubMenuItem(m_pMenu, itemName, CTweakCommon::eTT_CodeVariable));
		
		if(pItem)
		{
			pItem->Init(pValue, min, max, delta);
			id = pItem->GetId();

			if(settings != NULL)
			{
				// settings is an (optional) string of the form: "0=Off,1=On". Parse to get display text for different values.
				string set = settings;
				assert(set.find_first_of('=') != string::npos);
				size_t offset = 0;

				while (offset != string::npos)
				{
					string temp = set;
				
					offset = set.find_first_of(',');
					if(offset != string::npos)
					{
						temp = set.Left(offset);
						set = set.Right(set.length() - offset - 1);
					}
					
					size_t equalsPos = temp.find_first_of('=');
					assert(equalsPos != string::npos);

					string value = temp.Left(equalsPos);
					string name = temp.Right(temp.length() - equalsPos - 1);

					int v = atoi(value.c_str());

					pItem->AddDisplayString(v, name);
				}
			}
		}
	}

	return id;
}

TTweakableItemId CTweakMenuController::AddTweakableValue(const char* menuPath, float* pValue, float delta, float min, float max)
{
	TTweakableItemId id = INVALID_TWEAK_ITEM;

	if(m_pMenu)
	{
		string itemName = menuPath;
		CTweakMetadataVariable* pItem = static_cast<CTweakMetadataVariable*>(CreateSubMenuItem(m_pMenu, itemName, CTweakCommon::eTT_CodeVariable));

		if(pItem)
		{
			pItem->Init(pValue, min, max, delta);
			id = pItem->GetId();
		}
	}

	return id;
}

TTweakableItemId CTweakMenuController::AddCallbackItem(const char* menuPath, TweakCallbackFunction pFunction, void* pUserData)
{
	TTweakableItemId id = INVALID_TWEAK_ITEM;

	if(m_pMenu)
	{
		string itemName = menuPath;
		CTweakMetadataCallback* pItem = static_cast<CTweakMetadataCallback*>(CreateSubMenuItem(m_pMenu, itemName, CTweakCommon::eTT_Callback));

		if(pItem)
		{
			pItem->Init(pFunction, pUserData);
			id = pItem->GetId();
		}
	}

	return id;
}

bool CTweakMenuController::RemoveTweakableValue(TTweakableItemId id)
{
	if(m_pMenu)
	{
		CTweakTraverser t = m_pMenu->GetTraverser();
		CTweakCommon* pItem = t.Find(id);
		if(pItem && pItem->GetType() == CTweakCommon::eTT_CodeVariable)
		{
			if(m_traverser.Goto(pItem->GetName()))
				m_traverser.Remove();

			m_pMenu->RemoveItem(pItem);
			delete pItem;
		}
	}

	return true;
}
