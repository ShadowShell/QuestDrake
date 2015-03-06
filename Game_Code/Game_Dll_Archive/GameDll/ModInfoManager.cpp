#include "StdAfx.h"
#include "ModInfoManager.h"
#include "ISystem.h"
#include "ICmdLine.h"
#include "CryPath.h"
#include "CryAction.h"
#include "Game.h"

#define FILE_CHECK_BUFFER_SIZE			(32768)

static EModInfoType ModInfoTypeFromString(const char* str)
{
	if (stricmp(str, "SPMOD") == 0)
		return eMIT_SPMOD;
	else if (stricmp(str, "MPMOD") == 0)
		return eMIT_MPMOD;
	else if (stricmp(str, "SPMPMOD") == 0)
		return eMIT_SPANDMPMOD;
	else if (stricmp(str, "SPLEVEL") == 0)
		return eMIT_SPLEVEL;
	else
		return eMIT_UNDEFINED;
}

template<size_t Size>
bool ModInfo_FetchContent(CryFixedStringT<Size>* str, const XmlNodeRef& pRoot, const char* pNodeName)
{
	if (!str)
		return false;

	XmlNodeRef pNode = pRoot->findChild(pNodeName);
	if (!pNode)
		return false;

	const char* pContent = pNode->getContent();
	if (!pContent)
		return false;

	size_t attrLen = strlen(pContent);
	if (attrLen >= Size)
		str->assign(pContent, pContent + Size - 1);
	else
		str->assign(pContent, pContent + attrLen);

	str->replace("\\n", "\n");

	return true;
}

template<size_t Size>
bool ModInfo_FetchAttribute(CryFixedStringT<Size>* str, const XmlNodeRef& pRoot, const char* pNodeName, const char* pAttrName)
{
	if (!str)
		return false;

	XmlNodeRef pNode = pRoot->findChild(pNodeName);
	if (!pNode)
		return false;

	const char* pAttr = 0;
	if (!pNode->getAttr(pAttrName, &pAttr))
		return false;

	if (!pAttr)
		return false;

	size_t attrLen = strlen(pAttr);
	if (attrLen >= Size)
		str->assign(pAttr, pAttr + Size - 1);
	else
		str->assign(pAttr, pAttr + attrLen);

	return true;
}

bool ModInfo_FetchAttribute(int* value, const XmlNodeRef& pRoot, const char* pNodeName, const char* pAttrName)
{
	if (!value)
		return false;

	XmlNodeRef pNode = pRoot->findChild(pNodeName);
	if (!pNode)
		return false;

	const char* pAttr = 0;
	if (!pNode->getAttr(pAttrName, &pAttr))
		return false;

	if (!pAttr)
		return false;

	*value = atoi(pAttr);
	return true;
}

static bool ModInfo_LoadFromXML(ModInfo* pMod, const XmlNodeRef& pRoot)
{
	ModInfo_FetchAttribute(&pMod->displayName, pRoot, "Name", "name");
	ModInfo_FetchAttribute(&pMod->author, pRoot, "Author", "name");
	CryFixedStringT<32> type;
	if (ModInfo_FetchAttribute(&type, pRoot, "Type", "name"))
		pMod->modType = ModInfoTypeFromString(type.c_str());
	ModInfo_FetchAttribute(&pMod->website, pRoot, "URL", "address");
	ModInfo_FetchAttribute(&pMod->versionMajor, pRoot, "Version", "major");
	ModInfo_FetchAttribute(&pMod->versionMinor, pRoot, "Version", "minor");
	ModInfo_FetchContent(&pMod->description, pRoot, "Description");
	return true;
}

static bool ModInfo_LoadFromFile(ModInfo* pMod, const char* pFilename)
{
	if (!pMod)
		return false;

	if (!gEnv || !gEnv->pCryPak)
	{
		assert(0);
		return false;
	}

	ICryPak* pCryPak = gEnv->pCryPak;

	FILE* f = pCryPak->FOpen(pFilename, "rb", ICryPak::FOPEN_ONDISK);
	if (!f)
		return false;

	pCryPak->FSeek(f, 0, SEEK_END);
	const size_t fileSize = pCryPak->FTell(f);
	pCryPak->FSeek(f, 0, SEEK_SET);
	if (fileSize == 0)
	{
		pCryPak->FClose(f);
		return false;
	}

	std::vector<char> buffer;
	buffer.resize(fileSize);
	if (pCryPak->FRead(&buffer[0], fileSize, f) != fileSize)
	{
		pCryPak->FClose(f);
		return false;
	}
	pCryPak->FClose(f);
	
	std::auto_ptr<IXmlParser> pParser(GetISystem()->GetXmlUtils()->CreateXmlParser());
	XmlNodeRef pRoot = pParser->ParseBuffer(&buffer[0], buffer.size(), true);
	if (!pRoot)
		return false;

	if (!ModInfo_LoadFromXML(pMod, pRoot))
		return false;

	return true;
}


#define FILE_HASH_SEED      40503 // This is a large 16 bit prime number (perfect for seeding)

bool GetFileHash(const char *pPath, uint32 &outResult)
{
	if (!gEnv || !gEnv->pCryPak)
	{
		assert(0);
		return false;
	}

	ICryPak* pCryPak = gEnv->pCryPak;

	// Try to open file on disk and hash it (using algorithm taken from CryHashStringId)
	FILE *file = pCryPak->FOpen( pPath,"rb",ICryPak::FOPEN_ONDISK );
	if (file)
	{
		pCryPak->FSeek( file,0,SEEK_END );
		unsigned int nFileSize = pCryPak->FTell(file);
		pCryPak->FSeek( file,0,SEEK_SET );

		outResult = FILE_HASH_SEED;

		unsigned char *pBuf = (unsigned char*)malloc( FILE_CHECK_BUFFER_SIZE );
		if (!pBuf)
		{
			pCryPak->FClose(file);
			return false;
		}

		while (nFileSize)
		{
			unsigned int fetchLength=min(nFileSize,(unsigned int)FILE_CHECK_BUFFER_SIZE);

			unsigned int result = pCryPak->FRead( pBuf,fetchLength,file );
			if (result != fetchLength)
			{
				free( pBuf );
				pCryPak->FClose(file);
				return false;
			}
			
			const char *pChar = (const char*)pBuf;
			for (unsigned int i = 0; i < fetchLength; ++ i, ++ pChar)
			{
				outResult += *pChar;
				outResult += (outResult << 10);
				outResult ^= (outResult >> 6);
			}

			nFileSize-=fetchLength;
		}

		outResult += (outResult << 3);
		outResult ^= (outResult >> 11);
		outResult += (outResult << 15);

		free( pBuf );
		pCryPak->FClose(file);

		return true;
	}

	return false;
}

uint32 CModInfoManager::GetDirectoryHash( const char *pPath )
{
	if (!gEnv || !gEnv->pCryPak)
	{
		assert(0);
		return 0;
	}

	ICryPak* pCryPak = gEnv->pCryPak;

	// Recursively find all files and hash them
	uint32 result = 0;

	CryStackStringT<char, _MAX_PATH*2> searchPath;
	searchPath.Format("%s\\*", pPath);

	_finddata_t fd;
	intptr_t h = pCryPak->FindFirst(searchPath.c_str(), &fd, 0, true);
	if (h != -1)
	{
		CryStackStringT<char, _MAX_PATH*2> path;
		do 
		{
			if ((fd.attrib & _A_SUBDIR) == 0)
			{
				// Add to checksum
				uint32 fileHash = 0;

				CryStackStringT<char, _MAX_PATH*2> modPath;
				modPath.Format("%s\\%s", pPath, fd.name);

				if (GetFileHash(modPath.c_str(), fileHash))
				{
					result += fileHash;
				}

				continue;
			}

			if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0)
				continue;

			path = pPath;
			path += "\\";
			path += fd.name;

			result += GetDirectoryHash(path.c_str());
		}
		while(0 == pCryPak->FindNext(h, &fd));

		pCryPak->FindClose (h);
	}

	return result;
}

CModInfoManager::CModInfoManager()
{
	m_bRunningMod = false;
	m_modCheckSum = 0;

	const ICmdLineArg *pModArg = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"MOD");
	if (pModArg != NULL  && (*pModArg->GetValue() != 0) && (gEnv->pSystem->IsMODValid(pModArg->GetValue())))
	{
		m_bRunningMod = true;
	}

	Refresh();

	if (m_bRunningMod)
	{
		const ModInfo *pModInfo = GetActiveMod();
		if (pModInfo)
		{
			CryStackStringT<char, _MAX_PATH*2> modPath;
			modPath.Format("Mods\\%s", pModInfo->keyName.c_str());

			uint32 result = GetDirectoryHash(modPath.c_str());

			m_modCheckSum = result;
		}
	}
}

void CModInfoManager::Refresh()
{
#ifndef _WIN32
	assert(0 && "Calling Windows-specific code");
	return;
#endif

	if (!gEnv || !gEnv->pCryPak)
	{
		assert(0);
		return;
	}

	ICryPak* pCryPak = gEnv->pCryPak;

	m_mods.clear();

	// look for MODs
	{
		_finddata_t fd;
		intptr_t h = pCryPak->FindFirst ("Mods\\*", &fd, 0, true);
		if (h != -1)
		{
			CryStackStringT<char, _MAX_PATH*2> path;
			do 
			{
				if ((fd.attrib & _A_SUBDIR) == 0)
					continue;
				if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0)
					continue;

				path = "Mods\\";
				path += fd.name;

				ModInfo mod;

				if (ModInfo_LoadFromFile(&mod, (path + "\\info.xml").c_str()) && mod.modType != eMIT_SPLEVEL)
				{
					mod.keyName = fd.name;
					if (mod.displayName.empty())
						mod.displayName = mod.keyName;
					path.replace('\\', '/');
					path.insert(0, '/');
					mod.mainImage = (path + "/modpreview.jpg").c_str();
					mod.logoImage = (path + "/modteamlogo.jpg").c_str();
					m_mods.push_back(mod);
				}
			}
			while(0 == pCryPak->FindNext(h, &fd));

			pCryPak->FindClose (h);
		}
	}

	// look for SP-levels
	{
		CryStackStringT<char, _MAX_PATH*2> path;
		path += PathUtil::GetGameFolder();
		path += "\\Levels\\*";

		_finddata_t fd;
		intptr_t h = pCryPak->FindFirst (path.c_str(), &fd, 0, true);
		if (h != -1)
		{
			do 
			{
				if ((fd.attrib & _A_SUBDIR) == 0)
					continue;
				if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0)
					continue;

				path = PathUtil::GetGameFolder();
				path += "\\Levels\\";
				path += fd.name;

				ModInfo mod;

				if (ModInfo_LoadFromFile(&mod, (path + "\\info.xml").c_str()))
				{
					mod.modType = eMIT_SPLEVEL;
					mod.keyName = fd.name;
					path = "/Levels/";
					path += fd.name;
					mod.mainImage = (path + "/modpreview.jpg").c_str();
					mod.logoImage = (path + "/modteamlogo.jpg").c_str();
					if (mod.displayName.empty())
						mod.displayName = mod.keyName;
					m_mods.push_back(mod);
				}
			}
			while(0 == pCryPak->FindNext(h, &fd));

			pCryPak->FindClose (h);
		}
	}

	// look for MOD levels
	{
		m_modLevels.clear();

		CryStackStringT<char, _MAX_PATH*2> path;

		ILevelSystem* pLevelSystem = g_pGame->GetIGameFramework()->GetILevelSystem();
		size_t levelCount = pLevelSystem->GetLevelCount();
		for (size_t i = 0; i < levelCount; ++i)
		{
			ILevelInfo* pInfo = pLevelSystem->GetLevelInfo(i);
			if (!pInfo)
				continue;
			if (pInfo->GetIsModLevel() && (!pInfo->HasGameRules() || pInfo->SupportsGameType("SinglePlayer")))
			{
				ModInfo mod;
				const char* szLevelInfoDisplayName = pInfo->GetDisplayName();
				if (strcmp(szLevelInfoDisplayName, "") != 0)
				{
					mod.displayName = szLevelInfoDisplayName;
				}
				else // Fallback to directory name
				{
					mod.displayName = pInfo->GetName();
				}

				// load info.xml as well
				path = pInfo->GetPath();
				path.replace('/', '\\');
				if (ModInfo_LoadFromFile(&mod, (path + "\\info.xml").c_str()))
				{
					path.replace('\\', '/');
					path.insert(0, '/');
					mod.mainImage = (path + "/modpreview.jpg").c_str();
					mod.logoImage = (path + "/modteamlogo.jpg").c_str();
				}

				mod.keyName = pInfo->GetName();
				mod.modType = eMIT_SPLEVEL;

				m_modLevels.push_back(mod);
			}
		}
	}
}

void CModInfoManager::Free()
{
	Mods mods;
	m_mods.swap(mods);

	Mods levels;
	m_modLevels.swap(levels);
}

size_t CModInfoManager::GetModCount() const
{
	return m_mods.size();
}


const ModInfo* CModInfoManager::GetModByIndex(size_t index) const
{
	if (index > m_mods.size())
	{
		assert(0 && "Invalid index was used");
		return 0;
	}

	return &m_mods[index];
}

const char* CModInfoManager::GetActiveModName() const
{
	const ICmdLineArg *pModArg = GetISystem()->GetICmdLine()->FindArg(eCLAT_Pre, "MOD");
	if (!pModArg)
	{
		return "";
	}
	return pModArg->GetValue();
}

const ModInfo* CModInfoManager::GetActiveMod() const
{
	const char* activeModName = GetActiveModName();
	if (activeModName[0] == '\0')
		return 0; // no Mod selected

	for (size_t i = 0; i < m_mods.size(); ++i)
	{
		const ModInfo& mod = m_mods[i];
		if (stricmp(mod.keyName.c_str(), activeModName) == 0)
			return &mod;
	}

	// -MOD command line was used, but probably with wrong name
	return 0;
}

size_t CModInfoManager::GetLevelCount() const
{
	return m_modLevels.size();
}

const ModInfo* CModInfoManager::GetLevelByIndex(size_t index) const
{
	if (index > m_modLevels.size())
	{
		assert(0 && "Invalid index was used");
		return 0;
	}

	return &m_modLevels[index];
}

bool CModInfoManager::LoadMod(const char* modName)
{
	CryFixedStringT<256> command;
	command.Format("g_loadMod %s", modName);
	gEnv->pConsole->ExecuteString(command.c_str());
	return true;
}

bool CModInfoManager::UnloadMod()
{
	const char* activeMod = GetActiveModName();
	if (activeMod[0] == '\0')
		return false;

	gEnv->pConsole->ExecuteString("g_unloadMod");
	return true;
}

const ModInfo * CModInfoManager::FindModByDisplayName(const char *pName) const
{
	int numMods = m_mods.size();
	for (int i = 0; i < numMods; ++ i)
	{
		const ModInfo& mod = m_mods[i];
		if (stricmp(mod.displayName.c_str(), pName) == 0)
			return &mod;
	}
	return NULL;
}

const char* CModInfoManager::GetActiveModDisplayName() const
{
	const ModInfo *pModInfo = GetActiveMod();
	if (pModInfo)
	{
		return pModInfo->displayName.c_str();
	}
	return "";
}
