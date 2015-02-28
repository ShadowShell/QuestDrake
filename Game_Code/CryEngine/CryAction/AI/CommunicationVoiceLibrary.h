// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CommunicationVoiceLibrary_h__
#define __CommunicationVoiceLibrary_h__

#pragma once

#include <ICommunicationVoiceLibrary.h>


typedef uint32 VoiceLibraryID;
class CommunicationVoiceTestManager;

class CommunicationVoiceLibrary : public ICommunicationVoiceLibrary
{
	enum
	{
		MaxVariationCount = 32,
	};
public:
	CommunicationVoiceLibrary();

	void Reset();
	void ResetHistory();

	void Reload();
	void ScanFolder(const char* folderName, bool recursing = false);
	bool LoadFromFile(const char* fileName);

	void InitEditor(IGameToEditorInterface* pGameToEditor);

	bool GetVoice(VoiceLibraryID libraryID, const char* name, const char*& libraryPath, const char*& voiceName);
	bool GetVoiceVariation(VoiceLibraryID libraryID, const char* name, uint32 variationID, const char*& libraryPath,
		const char*& voiceName);

	VoiceLibraryID GetVoiceLibraryID(const char* name) const;
	const char* GetVoiceLibraryName(const VoiceLibraryID& libraryID) const;

	virtual uint32 GetGroupNames(const char* libraryName, uint32 maxCount, const char** names) const;
	const char* GetGroupName(const VoiceLibraryID& libraryID, uint32 index) const;
	uint32 GetGroupCount(const VoiceLibraryID& libraryID) const;

	CommunicationVoiceTestManager& GetTestManager();

	void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		SIZER_COMPONENT_NAME(pSizer, "CommunicationVoiceLibrary");

		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_folderNames);
		pSizer->AddObject(m_randomPool);
		pSizer->AddObject(m_libraries);		
	}
private:
	typedef std::vector<string> FolderNames;
	FolderNames m_folderNames;

	typedef std::vector<string> VoiceVariations;
	struct VoiceGroup
	{
		struct History
		{
			History()
				: played(0)
			{
			}

			uint32 played;
		};

		History history;

		VoiceVariations variations;

		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(variations);
		}
	};

	typedef std::map<string, VoiceGroup> VoiceGroups;
	struct VoiceLibrary
	{
		string name;
		string base;

		VoiceGroups voiceGroups;
		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(name);
			pSizer->AddObject(base);
			pSizer->AddObject(voiceGroups);
		}
	};

	typedef std::map<VoiceLibraryID, VoiceLibrary> VoiceLibraries;
	VoiceLibraries m_libraries;

	std::vector<uint8> m_randomPool;

	std::auto_ptr<CommunicationVoiceTestManager> m_testManager;
};


#endif