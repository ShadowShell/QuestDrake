////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PlayerProfileImplDurango.h
//  Created:     17/05/2013 by Yeonwoon JUNG.
//  Description: Created based on PlayerProfileImplConsole:
//
//               Player profile implementation for consoles which manage
//               the profile data via the OS and not via a file system 
//               which may not be present.
//
//               Problem with Durango is that we need to split attributes
//               into multiple categories, and each one has to have diff-
//               erent container.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if defined(DURANGO)

#pragma once

#include "PlayerProfileImplConsole.h"

class CPlayerProfileImplDurango : public CPlayerProfileImplConsole
{
private:
	// CPlayerProfileImplConsole
	virtual bool Initialize(CPlayerProfileManager* pMgr);
	virtual void Release();
	virtual void GetMemoryStatistics(ICrySizer * s);

	virtual bool SaveProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name, bool initialSave = false, int reason = ePR_All);
	virtual bool LoadProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name);

	virtual ISaveGame* CreateSaveGame(SUserEntry* pEntry);
	virtual ILoadGame* CreateLoadGame(SUserEntry* pEntry);
	virtual bool DeleteSaveGame(SUserEntry* pEntry, const char* name);

	virtual bool ResolveAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* attrBlockName, IResolveAttributeBlockListener* pListener, int reason);
	virtual bool ResolveAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const SResolveAttributeRequest& attrBlockNameRequest, IResolveAttributeBlockListener* pListener, int reason);
	virtual bool WriteAttributeBlock(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* attrBlockName, int reason);
	// ~CPlayerProfileImplConsole

private:
	// profile data blocks, these are essentially static. see also CPlayerProfileManager::EPlayerProfileSection.
	enum EProfileBlocks
	{
		ePB_Profile,
		ePB_ActionMaps,
		ePB_Attributes,

		ePB_Count,
	};

	static const wchar_t* ms_profileBlockNames[ePB_Count];

	// load profile from connected storage
	XmlNodeRef LoadPlayerProfile();

	// load attributes as a sub node of profile; skeleton needs to be passed to determine data blocks to load
	XmlNodeRef LoadPlayerProfileAttributes(XmlNodeRef attributesSkeleton);

	// save profile into connected storage
	bool SavePlayerProfile(XmlNodeRef rootNode, XmlNodeRef actionMap, XmlNodeRef attributes);

	// save attributes in profile into a separate container
	bool SavePlayerProfileAttributes(XmlNodeRef& attributes);

	// Title storage (per user); replicating/reloading profile block selectively.
	void UploadProfileBlockToTMS(const char* attrBlockName, const string& blobJSON);

	// @param	fun		callback function when data is ready. it is called asynchronously and guaranteed to run on game thread
	void DownloadProfileBlockFromTMS(const char* attrBlockName, std::function<void(const char*)> onSuccess, std::function<void()> onFailure);

	// list of storage types that platform provides.
	enum EPlatformStorageType
	{
		eEST_Invalid,

		eEST_ConnectedStorage	= BIT(0),
		eEST_TitleStorage			= BIT(1),
		eEST_SmartGlass				= BIT(2),
	};

	// storage flag for each attribute blocks that defines where to save out, from where to resolve.
	struct SAttributeBlockStorageInfo
	{
		SAttributeBlockStorageInfo(uint defaultFlag = eEST_Invalid)
			: storageTypeFlag(defaultFlag)
			, resolveFrom(eEST_Invalid)
		{}

		uint storageTypeFlag;
		EPlatformStorageType resolveFrom;
	};

	typedef std::map<string, SAttributeBlockStorageInfo> TAttributeBlockStorageInfos;
	TAttributeBlockStorageInfos m_attributeBlockStorageInfos;

	EPlatformStorageType m_defaultSaveLocation;
};

#endif
