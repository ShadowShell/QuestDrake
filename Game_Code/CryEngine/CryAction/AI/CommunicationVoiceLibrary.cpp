// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "CommunicationVoiceLibrary.h"
#include "CommunicationVoiceTestManager.h"

#include <IEditorGame.h>
#include <StringUtils.h>


CommunicationVoiceLibrary::CommunicationVoiceLibrary()
{
	m_testManager.reset(new CommunicationVoiceTestManager());
	m_randomPool.reserve(MaxVariationCount);
}

void CommunicationVoiceLibrary::Reset()
{
	ResetHistory();

	m_testManager->Reset();
}

void CommunicationVoiceLibrary::ResetHistory()
{
	VoiceLibraries::iterator lit = m_libraries.begin();
	VoiceLibraries::iterator lend = m_libraries.end();

	for ( ; lit != lend; ++lit)
	{
		VoiceGroups::iterator git = lit->second.voiceGroups.begin();
		VoiceGroups::iterator gend = lit->second.voiceGroups.end();

		for ( ; git != gend; ++git)
		{
			VoiceGroup& voiceGroup = git->second;

			voiceGroup.history = VoiceGroup::History();
		}
	}
}


void CommunicationVoiceLibrary::Reload()
{
	m_testManager->Reset();
	m_libraries.clear();

	for (uint32 i = 0; i < m_folderNames.size(); ++i)
		ScanFolder(m_folderNames[i].c_str(), false);
}

void CommunicationVoiceLibrary::ScanFolder(const char* folderName, bool recursing)
{
	string folder(PathUtil::MakeGamePath(string(folderName)));
	folder += "/";

	string searchString(folder + "*.xml");

	_finddata_t fd;
	intptr_t handle = 0;

	ICryPak *pPak = gEnv->pCryPak;
	handle = pPak->FindFirst(searchString.c_str(), &fd);

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			if (fd.attrib & _A_SUBDIR)
				ScanFolder(folder + fd.name, true);
			else
				LoadFromFile(folder + fd.name);

		} while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}

	if (!recursing)
		stl::push_back_unique(m_folderNames, folderName);
}

bool CommunicationVoiceLibrary::LoadFromFile(const char* fileName)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Communication Voice Library" );
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Voice Lib: %s",fileName );

	XmlNodeRef root = GetISystem()->LoadXmlFromFile(fileName);
	if (!root)
		return false;

	XmlNodeRef nodeWorksheet = root->findChild("Worksheet");
	if (!nodeWorksheet)
		return false;

	XmlNodeRef nodeTable = nodeWorksheet->findChild("Table");
	if (!nodeTable)
		return false;

	stack_string libName(PathUtil::GetFileName(fileName));

	VoiceLibraryID libraryID = GetVoiceLibraryID(libName.c_str());

	std::pair<VoiceLibraries::iterator, bool> iresult = m_libraries.insert(
		VoiceLibraries::value_type(libraryID, VoiceLibrary()));

	if (!iresult.second)
	{
		if (iresult.first->second.name == libName.c_str())
		{
			AIWarningID("<AICommunicationVoiceLibrary::LoadFromFile> ", 
				"Duplicate voice library '%s'!", libName.c_str());

			return false;
		}
		else
		{
			AIWarningID("<AICommunicationVoiceLibrary::LoadFromFile> ", 
				"Hash collision for voice library name '%s' and '%s'!", libName.c_str(), iresult.first->second.name.c_str());

			return false;
		}
	}

	VoiceLibrary& library = iresult.first->second;
	library.name = string(libName);
	
	VoiceGroup* voiceGroup = 0;
		
	string signalName;
	string lastSignalName;
	string voiceName;

	for (int rowCntr = 0, childN = 0; childN < nodeTable->getChildCount(); ++childN)
	{
		XmlNodeRef nodeRow = nodeTable->getChild(childN);
		if (!nodeRow->isTag("Row"))
			continue;

		++rowCntr;
		if (rowCntr == 1) // skip language
			continue;

		if (rowCntr == 2) // path
		{
			int cellN = 0;
			for (int childrenCntr = 0; childrenCntr < nodeRow->getChildCount(); ++childrenCntr)
			{
				XmlNodeRef nodeCell = nodeRow->getChild(childrenCntr);
				if (!nodeCell->isTag("Cell"))
					continue;

				++cellN;
				if (cellN == 2)
				{
					XmlNodeRef nodeCellData = nodeCell->findChild("Data");
					if (!nodeCellData)
						break;

					library.base = PathUtil::GetLocalizationFolder() + nodeCellData->getContent();
					if (!library.base.empty())
					{
						library.base.replace("\\", "/");
						if (library.base[library.base.length()-1] != '/')
							library.base.append("/");
					}
					break;
				}
			}
			continue;
		}

		if (rowCntr == 3) // headers
			continue;

		signalName.clear();
		voiceName.clear();
		
		for (int childrenCntr = 0, cellIndex = 1; childrenCntr < nodeRow->getChildCount(); ++childrenCntr, ++cellIndex)
		{
			XmlNodeRef nodeCell = nodeRow->getChild(childrenCntr);
			if (!nodeCell->isTag("Cell"))
				continue;

			if (nodeCell->haveAttr("ss:Index"))
			{
				const char* strIdx = nodeCell->getAttr("ss:Index");
				if (sscanf(strIdx, "%d", &cellIndex) != 1)
					continue;
			}

			XmlNodeRef nodeCellData = nodeCell->findChild("Data");
			if (!nodeCellData)
				continue;

			switch (cellIndex)
			{
			case 1:
				signalName = nodeCellData->getContent();
				break;
			case 2:
				voiceName = nodeCellData->getContent();
				break;
			}
		}

		if (!signalName.empty())
		{
			signalName.MakeLower();
			std::pair<VoiceGroups::iterator, bool> itresult = library.voiceGroups.insert(
				VoiceGroups::value_type(signalName, VoiceGroup()));

			voiceGroup = &itresult.first->second;
			// The 20 here comes from inspection of the resulting contents in memreplay
			voiceGroup->variations.reserve(20);

			if (!itresult.second)
			{
				if (lastSignalName != signalName)
					AIWarningID("<AICommunicationVoiceLibrary::LoadFromFile> ", 
						"Duplicate voice signal '%s' in file '%s'.", signalName.c_str(), libName.c_str());
			}

			lastSignalName = signalName;
		}

		if (!voiceGroup || voiceName.empty())
			continue;

		if ((library.base.find_first_of(':') == string::npos) && (voiceName.find_first_of(':') == string::npos))
			voiceName.append(".wav");

		if (voiceGroup->variations.size() < MaxVariationCount)
			voiceGroup->variations.push_back(voiceName);
		else
			AIWarningID("<AICommunicationVoiceLibrary::LoadFromFile> ", 
			"Too many voice variations for signal '%s' in file '%s'. Limit is 32.", signalName.c_str(), libName.c_str());
	}

	return true;
}

void CommunicationVoiceLibrary::InitEditor(IGameToEditorInterface* pGameToEditor)
{
	if (m_libraries.empty())
		return;

	uint32 libCount = m_libraries.size();
	const char** libNames = new const char*[libCount];

	VoiceLibraries::iterator it = m_libraries.begin();
	VoiceLibraries::iterator end = m_libraries.end();

	uint lib = 0;
	for ( ; it != end; ++it)
	{
		PREFAST_SUPPRESS_WARNING(6386)
		libNames[lib++] = it->second.name.c_str();
	}

	pGameToEditor->SetUIEnums("Voice", libNames, libCount);

	delete[] libNames;
}

bool CommunicationVoiceLibrary::GetVoice(VoiceLibraryID libraryID, const char* name, const char*& libraryPath,
																					 const char*& voiceName)
{
	stack_string variationName(name);
	variationName.MakeLower();
	VoiceLibraries::iterator it = m_libraries.find(libraryID);
	if (it == m_libraries.end())
		return false;

	VoiceLibrary& library = it->second;
	
	VoiceGroups::iterator git = library.voiceGroups.find(variationName.c_str());
	if (git == library.voiceGroups.end())
		return false;

	VoiceGroup& voiceGroup = git->second;
	
	uint32 variationCount = voiceGroup.variations.size();
	if (!variationCount)
		return false;

	uint32 variation = 0;

	if (variationCount > 1)
	{
		m_randomPool.clear();

		for (uint32 i = 0; i < variationCount; ++i)
		{
			if ((voiceGroup.history.played & (1 << i)) == 0)
				m_randomPool.push_back(i);
		}

		uint32 size = m_randomPool.size();
		assert(!m_randomPool.empty());
		variation = m_randomPool[Random(size)];
		assert(variation < voiceGroup.variations.size());

		voiceGroup.history.played |= 1 << variation;

		if ((voiceGroup.history.played & ((1 << variationCount) - 1)) == voiceGroup.history.played)
			voiceGroup.history.played = 1 << variation;
	}

	if (!library.base.empty())
		libraryPath = library.base.c_str();
	
	const string& variationVoiceName = voiceGroup.variations[variation];
	
	if (!variationVoiceName.empty())
		voiceName = variationVoiceName.c_str();

	return true;
}

bool CommunicationVoiceLibrary::GetVoiceVariation(VoiceLibraryID libraryID, const char* name, uint32 variation,
																									const char*& libraryPath, const char*& voiceName)
{
	VoiceLibraries::iterator it = m_libraries.find(libraryID);
	if (it == m_libraries.end())
		return false;

	VoiceLibrary& library = it->second;

	VoiceGroups::iterator git = library.voiceGroups.find(name);
	if (git == library.voiceGroups.end())
		return false;

	VoiceGroup& voiceGroup = git->second;

	uint32 variationCount = voiceGroup.variations.size();
	if (variation >= variationCount)
		return false;

	if (!library.base.empty())
		libraryPath = library.base.c_str();

	const string& variationVoiceName = voiceGroup.variations[variation];

	if (!variationVoiceName.empty())
		voiceName = variationVoiceName.c_str();

	return true;
}

VoiceLibraryID CommunicationVoiceLibrary::GetVoiceLibraryID(const char* name) const
{
	return CryStringUtils::CalculateHashLowerCase(name);
}

const char* CommunicationVoiceLibrary::GetVoiceLibraryName(const VoiceLibraryID& libraryID) const
{
	VoiceLibraries::const_iterator libraryIt = m_libraries.find(libraryID);
	if (libraryIt == m_libraries.end())
		return 0;

	return libraryIt->second.name.c_str();
}

uint32 CommunicationVoiceLibrary::GetGroupNames(const char* libraryName, uint32 maxCount, const char** names) const
{
	VoiceLibraryID libraryID = GetVoiceLibraryID(libraryName);

	VoiceLibraries::const_iterator libraryIt = m_libraries.find(libraryID);
	if (libraryIt == m_libraries.end())
		return 0;

	const VoiceLibrary& library = libraryIt->second;

	uint32 count = static_cast<uint32>(library.voiceGroups.size());

	if (count > maxCount)
	{
		gEnv->pLog->LogError(
			"CommunicationVoiceLibrary::GetGroupNames:"
			"There are %d voice groups in library %s but"
			"the caller only passed in an array that can fit %d. Truncating.",
			count, libraryName, maxCount);

		count = maxCount;
	}

	VoiceGroups::const_iterator it = library.voiceGroups.begin();
	VoiceGroups::const_iterator end = library.voiceGroups.end();

	assert(names);
	for (uint32 i = 0; i < count && it != end; ++i, ++it)
		names[i] = it->first.c_str();

	return count;
}

const char* CommunicationVoiceLibrary::GetGroupName(const VoiceLibraryID& libraryID, uint32 index) const
{
	VoiceLibraries::const_iterator libraryIt = m_libraries.find(libraryID);
	if (libraryIt == m_libraries.end())
		return 0;

	const VoiceLibrary& library = libraryIt->second;

	VoiceGroups::const_iterator it = library.voiceGroups.begin();
	VoiceGroups::const_iterator end = library.voiceGroups.end();

	if (index >= library.voiceGroups.size())
		return 0;

	std::advance(it, (int32)index);

	return it->first.c_str();
}

uint32 CommunicationVoiceLibrary::GetGroupCount(const VoiceLibraryID& libraryID) const
{
	VoiceLibraries::const_iterator libraryIt = m_libraries.find(libraryID);
	if (libraryIt == m_libraries.end())
		return 0;

	const VoiceLibrary& library = libraryIt->second;

	return static_cast<uint32>(library.voiceGroups.size());
}

CommunicationVoiceTestManager& CommunicationVoiceLibrary::GetTestManager()
{
	return *m_testManager;
}