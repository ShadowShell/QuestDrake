// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "stdafx.h"
#include "FileIOHandler_wwise.h"
#include <IAudioInterfacesCommonData.h>
#include <ICryPak.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    AKTEXT("%u.bnk")
#define ID_TO_STRING_FORMAT_WEM     AKTEXT("%u.wem")
#define MAX_EXTENSION_SIZE          (4)     // .xxx
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE+MAX_EXTENSION_SIZE+1)   // null-terminated

//////////////////////////////////////////////////////////////////////////
AkFileHandle GetFileHandle(FILE* const pFile)
{
#if defined(WIN32) || defined(WIN64) || defined(DURANGO)
	return reinterpret_cast<AkFileHandle>(pFile);
#elif defined(ORBIS) 
	return static_cast<AkFileHandle>(reinterpret_cast<INT_PTR>(pFile));
#elif defined(LINUX) || defined(APPLE)
	return static_cast<AkFileHandle>(0); // TODO!
#else
#error "Undefined platform!"
#endif
}

//////////////////////////////////////////////////////////////////////////
CFileIOHandler_wwise::CFileIOHandler_wwise()
: m_bAsyncOpen(false)
{
	memset(m_sBankPath, 0, AK_MAX_PATH * sizeof(AkOSChar));
	memset(m_sLanguageFolder, 0, AK_MAX_PATH * sizeof(AkOSChar));
}

//////////////////////////////////////////////////////////////////////////
CFileIOHandler_wwise::~CFileIOHandler_wwise()
{
	bool const bStop = true;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Init(AkDeviceSettings const& rDeviceSettings, bool const bAsyncOpen /* = false */)
{
	AKRESULT eResult = AK_Fail;

	if (rDeviceSettings.uSchedulerTypeFlags == AK_SCHEDULER_BLOCKING)
	{
		m_bAsyncOpen = bAsyncOpen;

		// If the Stream Manager's File Location Resolver was not set yet, set this object as the 
		// File Location Resolver (this I/O hook is also able to resolve file location).
		if (!AK::StreamMgr::GetFileLocationResolver())
		{
			AK::StreamMgr::SetFileLocationResolver(this);
		}

		// Create a device in the Stream Manager, specifying this as the hook.
		m_nDeviceID = AK::StreamMgr::CreateDevice(rDeviceSettings, this);

		if (m_nDeviceID != AK_INVALID_DEVICE_ID)
		{
			eResult = AK_Success;
		}
	}
	else
	{
		AKASSERT(!"CAkDefaultIOHookBlocking I/O hook only works with AK_SCHEDULER_BLOCKING devices");
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
void CFileIOHandler_wwise::ShutDown()
{
	if (AK::StreamMgr::GetFileLocationResolver() == this)
	{
		AK::StreamMgr::SetFileLocationResolver(NPTR);
	}

	AK::StreamMgr::DestroyDevice(m_nDeviceID);
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Open(AkOSChar const* sFileName, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc)
{
	AKRESULT eResult = AK_Fail;

	if (rSyncOpen || !m_bAsyncOpen)
	{
		rSyncOpen = true;
		AkOSChar sFinalFilePath[AK_MAX_PATH] = {'\0'};
		AKPLATFORM::SafeStrCat(sFinalFilePath, m_sBankPath, AK_MAX_PATH);

		if (pFlags != NPTR && eOpenMode == AK_OpenModeRead)
		{
			// Add language folder if the file is localized.
			if (pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && pFlags->uCodecID == AKCODECID_BANK && pFlags->bIsLanguageSpecific)
			{
				AKPLATFORM::SafeStrCat(sFinalFilePath, m_sLanguageFolder, AK_MAX_PATH);
			}
		}

		AKPLATFORM::SafeStrCat(sFinalFilePath, sFileName, AK_MAX_PATH);

		char* sTemp = NPTR;
		CONVERT_OSCHAR_TO_CHAR(sFinalFilePath, sTemp);

		size_t const nFileSize = gEnv->pCryPak->FGetSize(sTemp);

		if (nFileSize > 0)
		{
			FILE* const pFile = gEnv->pCryPak->FOpen(sTemp, "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);

			if (pFile != NPTR)
			{
				rFileDesc.iFileSize					= static_cast<AkInt64>(nFileSize);
				rFileDesc.hFile							= GetFileHandle(pFile);
				rFileDesc.uSector						= 0;
				rFileDesc.deviceID					= m_nDeviceID;
				rFileDesc.pCustomParam			= NPTR;
				rFileDesc.uCustomParamSize	= 0;

				eResult = AK_Success;
			}
		}
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Open(AkFileID nFileID, AkOpenMode eOpenMode, AkFileSystemFlags* pFlags, bool& rSyncOpen, AkFileDesc& rFileDesc)
{
	AKRESULT eResult = AK_Fail;

	if (rSyncOpen || !m_bAsyncOpen)
	{
		rSyncOpen = true;
		AkOSChar sFinalFilePath[AK_MAX_PATH] = {'\0'};
		AKPLATFORM::SafeStrCat(sFinalFilePath, m_sBankPath, AK_MAX_PATH);

		if (pFlags != NPTR && eOpenMode == AK_OpenModeRead)
		{
			// Add language folder if the file is localized.
			if (pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && pFlags->bIsLanguageSpecific)
			{
				AKPLATFORM::SafeStrCat(sFinalFilePath, m_sLanguageFolder, AK_MAX_PATH);
			}
		}

		AkOSChar sFileName[MAX_FILETITLE_SIZE] = {'\0'};

		AkOSChar const* const sFilenameFormat = pFlags->uCodecID == AKCODECID_BANK ? ID_TO_STRING_FORMAT_BANK : ID_TO_STRING_FORMAT_WEM;

		AK_OSPRINTF(sFileName, MAX_FILETITLE_SIZE, sFilenameFormat, static_cast<int unsigned>(nFileID));

		AKPLATFORM::SafeStrCat(sFinalFilePath, sFileName, AK_MAX_PATH);

		char* sTemp = NPTR;
		CONVERT_OSCHAR_TO_CHAR(sFinalFilePath, sTemp);

		size_t const nFileSize = gEnv->pCryPak->FGetSize(sTemp);

		if (nFileSize > 0)
		{
			FILE* const pFile = gEnv->pCryPak->FOpen(sTemp, "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);

			if (pFile != NPTR)
			{
				rFileDesc.iFileSize					= static_cast<AkInt64>(nFileSize);
				rFileDesc.hFile							= GetFileHandle(pFile);
				rFileDesc.uSector						= 0;
				rFileDesc.deviceID					= m_nDeviceID;
				rFileDesc.pCustomParam			= NPTR;
				rFileDesc.uCustomParamSize	= 0;

				eResult = AK_Success;
			}
		}
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Read(AkFileDesc& rFileDesc, AkIoHeuristics const& rHeuristics, void* pBuffer, AkIOTransferInfo& rTransferInfo)
{
	AKASSERT(pBuffer != NPTR && rFileDesc.hFile != AkFileHandle(INVALID_HANDLE_VALUE));

	FILE* const pFile = reinterpret_cast<FILE*>(rFileDesc.hFile);
	long const nCurrentFileReadPos	= gEnv->pCryPak->FTell(pFile);
	long const nWantedFileReadPos		= static_cast<long>(rTransferInfo.uFilePosition);

	if (nCurrentFileReadPos != nWantedFileReadPos)
	{
		gEnv->pCryPak->FSeek(pFile, nWantedFileReadPos, SEEK_SET);
	}

	size_t const nBytesRead = gEnv->pCryPak->FReadRaw(pBuffer, 1, rTransferInfo.uRequestedSize, pFile);
	AKASSERT(nBytesRead == static_cast<size_t>(rTransferInfo.uRequestedSize));

	return (nBytesRead > 0) ? AK_Success : AK_Fail;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Write(AkFileDesc& in_fileDesc, AkIoHeuristics const& in_heuristics, void* in_pData, AkIOTransferInfo& io_transferInfo)
{
	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CFileIOHandler_wwise::Close(AkFileDesc& rFileDesc)
{
	AKRESULT eResult = AK_Fail;

	if (!gEnv->pCryPak->FClose(reinterpret_cast<FILE*>(rFileDesc.hFile)))
	{
		eResult = AK_Success;
	}
	else
	{
		AKASSERT(!"Failed to close file handle");
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
AkUInt32 CFileIOHandler_wwise::GetBlockSize(AkFileDesc& in_fileDesc)
{
	// No constraint on block size (file seeking).
	return 1;
}

//////////////////////////////////////////////////////////////////////////
void CFileIOHandler_wwise::GetDeviceDesc(AkDeviceDesc& out_deviceDesc)
{
	bool const bStop = true;
}

//////////////////////////////////////////////////////////////////////////
AkUInt32 CFileIOHandler_wwise::GetDeviceData()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
void CFileIOHandler_wwise::SetBankPath(AkOSChar const* const sBankPath)
{
	AKPLATFORM::SafeStrCpy(m_sBankPath, sBankPath, AK_MAX_PATH);
}

//////////////////////////////////////////////////////////////////////////
void CFileIOHandler_wwise::SetLanguageFolder(AkOSChar const* const sLanguageFolder)
{
	AKPLATFORM::SafeStrCpy(m_sLanguageFolder, sLanguageFolder, AK_MAX_PATH);
}
