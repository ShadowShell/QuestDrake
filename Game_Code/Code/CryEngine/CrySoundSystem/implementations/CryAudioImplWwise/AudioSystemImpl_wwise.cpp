// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "stdafx.h"
#include "AudioSystemImpl_wwise.h"
#include "AudioSystemImplCVars.h"
#include "FileIOHandler_wwise.h"
#include "Common_wwise.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>			// Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>			// Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>				// Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>						// Default memory and stream managers
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>	// AkStreamMgrModule
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkCallback.h>

#include <AK/Plugin/AllPluginsRegistrationHelpers.h>

#include <ICryPak.h>
#include <IAudioSystem.h>

#if !defined(WWISE_FOR_RELEASE)
#include <AK/Comm/AkCommunication.h>	// Communication between Wwise and the game (excluded in release build)
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#endif // WWISE_FOR_RELEASE

char const* const CAudioSystemImpl_wwise::sWwiseEventTag						= "WwiseEvent";
char const* const CAudioSystemImpl_wwise::sWwiseRtpcTag							= "WwiseRtpc";
char const* const CAudioSystemImpl_wwise::sWwiseSwitchTag						= "WwiseSwitch";
char const* const CAudioSystemImpl_wwise::sWwiseStateTag						= "WwiseState";
char const* const CAudioSystemImpl_wwise::sWwiseRtpcSwitchTag				= "WwiseRtpc";
char const* const	CAudioSystemImpl_wwise::sWwiseFileTag							= "WwiseFile";
char const* const	CAudioSystemImpl_wwise::sWwiseAuxBusTag						= "WwiseAuxBus";
char const* const	CAudioSystemImpl_wwise::sWwiseValueTag						= "WwiseValue";
char const* const CAudioSystemImpl_wwise::sWwiseNameAttribute				= "wwise_name";
char const* const CAudioSystemImpl_wwise::sWwiseValueAttribute			= "wwise_value";
char const* const CAudioSystemImpl_wwise::sWwiseMutiplierAttribute	= "atl_mult";
char const* const CAudioSystemImpl_wwise::sWwiseShiftAttribute			=	"atl_shift";
char const* const CAudioSystemImpl_wwise::sWwiseLocalisedAttribute	= "wwise_localised";

/////////////////////////////////////////////////////////////////////////////////
//                              MEMORY HOOKS SETUP
//
//                             ##### IMPORTANT #####
//
// These custom alloc/free functions are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
/////////////////////////////////////////////////////////////////////////////////

namespace AK
{
	void* AllocHook(size_t in_size)
	{
		return g_AudioImplMemoryPool.Allocate<void*>(in_size, AUDIO_MEMORY_ALIGNMENT, "AudioWwise");
	}

	void FreeHook(void* in_ptr)
	{
		g_AudioImplMemoryPool.Free(in_ptr);
	}

	void* VirtualAllocHook(void* in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect)
	{
		return g_AudioImplMemoryPool.Allocate<void*>(in_size, AUDIO_MEMORY_ALIGNMENT, "AudioWwise");
		//return VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
	}

	void VirtualFreeHook(void* in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
	{
		//VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
		g_AudioImplMemoryPool.Free(in_pMemAddress);
	}

#if defined(DURANGO)
	void* APUAllocHook(size_t in_size, unsigned int in_alignment)
	{
		void* pAlloc = NPTR;

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
		size_t const nSecondSize = g_AudioImplMemoryPoolSoundSecondary.MemSize();

		if (nSecondSize > 0)
		{
			size_t const nAllocHandle	= g_AudioImplMemoryPoolSoundSecondary.Allocate<size_t>(in_size, in_alignment);

			if (nAllocHandle > 0)
			{
				pAlloc = g_AudioImplMemoryPoolSoundSecondary.Resolve<void*>(nAllocHandle);
				g_AudioImplMemoryPoolSoundSecondary.Item(nAllocHandle)->Lock();
			}
		}
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

		assert(pAlloc != NPTR);
		return pAlloc;
	}

	void APUFreeHook(void* in_pMemAddress)
	{
		bool bFreed = false;

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
		size_t const nAllocHandle = g_AudioImplMemoryPoolSoundSecondary.AddressToHandle(in_pMemAddress);
		//size_t const nOldSize = g_MemoryPoolSoundSecondary.Size(nAllocHandle);
		bFreed = g_AudioImplMemoryPoolSoundSecondary.Free(nAllocHandle);
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

		assert(bFreed);
	}
#endif // DURANGO
}

// AK callbacks
void EndEventCallback(AkCallbackType eType, AkCallbackInfo* pCallbackInfo)
{
	if (eType == AK_EndOfEvent)
	{
		SATLEventData_wwise* const pEventData = static_cast<SATLEventData_wwise* const>(pCallbackInfo->pCookie);

		if (pEventData != NPTR)
		{
			SAudioRequest oRequest;
			SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> oRequestData(pEventData->nATLID, true);
			oRequest.pData = &oRequestData;

			gEnv->pAudioSystem->PushRequest(oRequest);
		}
	}
}

void PrepareEventCallback(
	AkUniqueID nAkEventID,
	void const* pBankPtr,
	AKRESULT eLoadResult,
	AkMemPoolId nMenmPoolID,
	void* pCookie)
{
	SATLEventData_wwise* const pEventData = static_cast<SATLEventData_wwise* const>(pCookie);

	if (pEventData != NPTR)
	{
		pEventData->nAKID = nAkEventID;

		SAudioRequest oRequest;
		SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> oRequestData(pEventData->nATLID, eLoadResult ==  AK_Success);
		oRequest.pData = &oRequestData;

		gEnv->pAudioSystem->PushRequest(oRequest);
	}
}

#if !defined(WWISE_FOR_RELEASE)
static void ErrorMonitorCallback(
	AK::Monitor::ErrorCode in_eErrorCode,	///< Error code number value
	const AkOSChar* in_pszError,	///< Message or error string to be displayed
	AK::Monitor::ErrorLevel in_eErrorLevel,	///< Specifies whether it should be displayed as a message or an error
	AkPlayingID in_playingID,   ///< Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
	AkGameObjectID in_gameObjID ///< Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
	)
{
	char* sTemp = NPTR;
	CONVERT_OSCHAR_TO_CHAR(in_pszError, sTemp);
	g_AudioImplLogger.Log(
		((in_eErrorLevel & AK::Monitor::ErrorLevel_Error) != 0) ? eALT_ERROR : eALT_COMMENT,
		"<Wwise> %s ErrorCode: %d PlayingID: %u GameObjID: %" PRISIZE_T, sTemp, in_eErrorCode, in_playingID, in_gameObjID);
}
#endif // WWISE_FOR_RELEASE

///////////////////////////////////////////////////////////////////////////
CAudioSystemImpl_wwise::CAudioSystemImpl_wwise()
	: m_nDummyGameObjectID(static_cast<AkGameObjectID>(-2))
	, m_nInitBankID(AK_INVALID_BANK_ID)
#if !defined(WWISE_FOR_RELEASE)
	,	m_bCommSystemInitialized(false)
#endif // !WWISE_FOR_RELEASE
{
	string sGameFolder = PathUtil::GetGameFolder().c_str();

	if (sGameFolder.empty())
	{
		CryFatalError("<Audio>: Needs a valid game folder to proceed!");
	}

	m_sRegularSoundBankFolder = sGameFolder.c_str();
	m_sRegularSoundBankFolder += WWISE_IMPL_BANK_FULL_PATH;

	m_sLocalizedSoundBankFolder = m_sRegularSoundBankFolder;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	m_sFullImplString = WWISE_IMPL_INFO_STRING " (";
	m_sFullImplString += sGameFolder + WWISE_IMPL_BASE_PATH ")";
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
CAudioSystemImpl_wwise::~CAudioSystemImpl_wwise()
{
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::Update(float fUpdateIntervalMS)
{
	if (AK::SoundEngine::IsInitialized())
	{
		AK::SoundEngine::RenderAudio();
	}
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::Init()
{
	AkMemSettings oMemSettings;
	oMemSettings.uMaxNumPools = 20;

	AKRESULT eResult = AK::MemoryMgr::Init(&oMemSettings);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "AK::MemoryMgr::Init() returned AKRESULT %d", eResult);

		return eARS_FAILURE;
	}

	AkMemPoolId const nPrepareMemPoolID = AK::MemoryMgr::CreatePool(NPTR, 2*1024*1024, 16, AkMalloc, 16);

	if (nPrepareMemPoolID == AK_INVALID_POOL_ID)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "AK::MemoryMgr::CreatePool() failed \n");

		return eARS_FAILURE;
	}

	AkStreamMgrSettings oStreamSettings;
	AK::StreamMgr::GetDefaultSettings(oStreamSettings);

	if (AK::StreamMgr::Create(oStreamSettings) == NPTR)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "AK::StreamMgr::Create() failed\n");
		
		return eARS_FAILURE;
	}

	AkDeviceSettings oDeviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(oDeviceSettings);
	//oDeviceSettings.threadProperties.dwAffinityMask = 1;

	eResult = m_oFileIOHandler.Init(oDeviceSettings);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "m_oFileIOHandler.Init() returned AKRESULT %d", eResult);

		return eARS_FAILURE;
	}

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sTemp(PathUtil::GetGameFolder().c_str());
	sTemp.append(WWISE_IMPL_BASE_PATH);
	AkOSChar const* pTemp = NPTR;
	CONVERT_CHAR_TO_OSCHAR(sTemp.c_str(), pTemp);
	m_oFileIOHandler.SetBankPath(pTemp);

	AkInitSettings oInitSettings;
	AK::SoundEngine::GetDefaultInitSettings(oInitSettings);
	oInitSettings.uDefaultPoolSize = 16*1024*1024;
	oInitSettings.uPrepareEventMemoryPoolID = nPrepareMemPoolID;
	oInitSettings.bEnableGameSyncPreparation = false;//TODO: ???
	AkPlatformInitSettings oPlatformInitSettings;
	AK::SoundEngine::GetDefaultPlatformInitSettings(oPlatformInitSettings);
	oPlatformInitSettings.uLEngineDefaultPoolSize = 16*1024*1024;

#if defined(WIN32) || defined(WIN64)
	oPlatformInitSettings.threadBankManager.dwAffinityMask	= 0;
	oPlatformInitSettings.threadLEngine.dwAffinityMask			= 0;
	oPlatformInitSettings.threadMonitor.dwAffinityMask			= 0;
#elif defined(DURANGO)
#elif defined(ORBIS)
#elif defined(MAC)
#elif defined(LINUX)
#elif defined(CAFE)
#else
#error "Undefined platform."
#endif
	
	eResult = AK::SoundEngine::Init(&oInitSettings, &oPlatformInitSettings);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "AK::SoundEngine::Init() returned AKRESULT %d", eResult);
		
		return eARS_FAILURE;
	}

	AkMusicSettings oMusicInit;
	AK::MusicEngine::GetDefaultInitSettings(oMusicInit);

	eResult = AK::MusicEngine::Init(&oMusicInit);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "AK::MusicEngine::Init() returned AKRESULT %d", eResult);
		
		return eARS_FAILURE;
	}

#if !defined(WWISE_FOR_RELEASE)
	if (g_AudioWwiseImplCVars.m_nEnableCommSystem == 1)
	{
		m_bCommSystemInitialized = true;
		AkCommSettings oCommSettings;
		AK::Comm::GetDefaultInitSettings(oCommSettings);

		eResult = AK::Comm::Init(oCommSettings);

		if (eResult != AK_Success)
		{
			g_AudioImplLogger.Log(eALT_ERROR, "AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible\n", eResult);
			m_bCommSystemInitialized = false;
		}

		eResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

		if (eResult != AK_Success)
		{
			AK::Comm::Term();
			g_AudioImplLogger.Log(eALT_ERROR, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", eResult);
			m_bCommSystemInitialized = false;
		}
	}
#endif // !WWISE_FOR_RELEASE

	//
	// Register plugins
	/// Note: This a convenience method for rapid prototyping. 
	/// To reduce executable code size register/link only the plug-ins required by your game 
	eResult = AK::SoundEngine::RegisterAllPlugins();

	/*eResult = AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, 
		AKEFFECTID_CONVOLUTIONREVERB,
		CreateConvolutionReverbFX,
		CreateConvolutionReverbFXParams);*/

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::RegisterAllPlugins() returned AKRESULT %d", eResult);
	}

	REINST("Register Global Callback")

	//eResult = AK::SoundEngine::RegisterGlobalCallback(GlobalCallback);
	//if (eResult != AK_Success)
	//{
	//	g_AudioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::RegisterGlobalCallback() returned AKRESULT %d", eResult);
	//}

	// Register the DummyGameObject used for the events that don't need a location in the game world
	eResult = AK::SoundEngine::RegisterGameObj(m_nDummyGameObjectID, "DummyObject");

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::RegisterGameObject() failed for the Dummyobject with AKRESULT %d", eResult);
	}

	// Load Init.bnk before making the system available to the users
	eResult = AK::SoundEngine::LoadBank(L"Init.bnk", AK_DEFAULT_POOL_ID, m_nInitBankID);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", eResult);
		return eARS_FAILURE;
	}

	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::ShutDown()
{
	if (AK::SoundEngine::IsInitialized())
	{
		AKRESULT eResult = AK_Fail;

		// UnRegister the DummyGameObject
		eResult = AK::SoundEngine::UnregisterGameObj(m_nDummyGameObjectID);

		if (eResult != AK_Success)
		{
			g_AudioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::UnregisterGameObject() failed for the Dummyobject with AKRESULT %d", eResult);
		}

		eResult = AK::SoundEngine::ClearBanks();

		if (eResult != AK_Success)
		{
			g_AudioImplLogger.Log(eALT_ERROR, "Failed to clear banks\n");
		}

#if !defined(WWISE_FOR_RELEASE)
		if (m_bCommSystemInitialized)
		{
			AK::Comm::Term();

			eResult = AK::Monitor::SetLocalOutput(0, NPTR);

			if (eResult != AK_Success)
			{
				g_AudioImplLogger.Log(eALT_WARNING, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", eResult);
			}

			m_bCommSystemInitialized = false;
		}
#endif // !WWISE_FOR_RELEASE

		AK::MusicEngine::Term();

		REINST("Unregister global callback")
		//eResult = AK::SoundEngine::UnregisterGlobalCallback(GlobalCallback);
		//ASSERT_WWISE_OK(eResult);

		//if (eResult != AK_Success)
		//{
		//	g_AudioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::UnregisterGlobalCallback() returned AKRESULT %d", eResult);
		//}

		AK::SoundEngine::Term();

		// Terminate the streaming device and streaming manager
		// CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device 
		// that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
		if (AK::IAkStreamMgr::Get())
		{
			m_oFileIOHandler.ShutDown();
			AK::IAkStreamMgr::Get()->Destroy();
		}

		// Terminate the Memory Manager
		if (AK::MemoryMgr::IsInitialized())
		{
			eResult = AK::MemoryMgr::DestroyPool(0);
			AK::MemoryMgr::Term();
		}
	}

	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::Release()
{
	POOL_FREE(this);

	// Freeing Memory Pool Memory again
	uint8* pMemSystem = g_AudioImplMemoryPool.Data();
	g_AudioImplMemoryPool.UnInitMem();

	if (pMemSystem)
	{
		delete[] (uint8*)(pMemSystem);
	}

	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::OnLoseFocus()
{
	// With Wwise we drive this via events.
	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::OnGetFocus()
{
	// With Wwise we drive this via events.
	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::MuteAll()
{
	// With Wwise we drive this via events.
	// Note: Still, make sure to return eARS_SUCCESS to signal the ATL.
	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UnmuteAll()
{
	// With Wwise we drive this via events.
	// Note: Still, make sure to return eARS_SUCCESS to signal the ATL.
	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::StopAllSounds()
{
	AK::SoundEngine::StopAll();

	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::RegisterAudioObject(IATLAudioObjectData* const pObjectData)
{
	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pObjectData);

	AKRESULT const eAKResult = AK::SoundEngine::RegisterGameObj(pAKObjectData->nAKID);

	bool const bAKSuccess = IS_WWISE_OK(eAKResult);

	if (!bAKSuccess)
	{
		g_AudioImplLogger.Log(eALT_WARNING, "Wwise RegisterGameObj failed with AKRESULT: %d", eAKResult);
	}

	return BoolToARS(bAKSuccess);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::RegisterAudioObject(
	IATLAudioObjectData* const pObjectData, 
	char const* const sObjectName)
{
	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pObjectData);

	AKRESULT const eAKResult = AK::SoundEngine::RegisterGameObj(pAKObjectData->nAKID, sObjectName);

	bool const bAKSuccess = IS_WWISE_OK(eAKResult);

	if (!bAKSuccess)
	{
		g_AudioImplLogger.Log(eALT_WARNING, "Wwise RegisterGameObj failed with AKRESULT: %d", eAKResult);
	}

	return BoolToARS(bAKSuccess);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterAudioObject(IATLAudioObjectData* const pObjectData)
{
	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pObjectData);

	AKRESULT const eAKResult = AK::SoundEngine::UnregisterGameObj(pAKObjectData->nAKID);

	bool const bAKSuccess = IS_WWISE_OK(eAKResult);

	if (!bAKSuccess)
	{
		g_AudioImplLogger.Log(eALT_WARNING, "Wwise UnregisterGameObj failed with AKRESULT: %d", eAKResult);
	}

	return BoolToARS(bAKSuccess);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::ResetAudioObject(IATLAudioObjectData* const pObjectData)
{
	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pObjectData);

	pAKObjectData->cEnvironemntImplAmounts.clear();
	pAKObjectData->bNeedsToUpdateEnvironments = false;

	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UpdateAudioObject(IATLAudioObjectData* const pObjectData)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pObjectData);

	if (pAKObjectData->bNeedsToUpdateEnvironments)
	{
		eResult = PostEnvironmentAmounts(pAKObjectData);
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerSync(
	IATLAudioObjectData* const pAudioObjectData,
	IATLTriggerImplData const* const pTriggerData)
{
	return PrepUnprepTriggerSync(pTriggerData, true);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerSync(
	IATLAudioObjectData* const pAudioObjectData,
	IATLTriggerImplData const* const pTriggerData)
{
	return PrepUnprepTriggerSync(pTriggerData, false);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::PrepareTriggerAsync(
	IATLAudioObjectData* const pAudioObjectData,
	IATLTriggerImplData const* const pTriggerData,
	IATLEventData* const pEventData)
{
	return PrepUnprepTriggerAsync(pTriggerData, pEventData, true);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UnprepareTriggerAsync(
	IATLAudioObjectData* const pAudioObjectData,
	IATLTriggerImplData const* const pTriggerData,
	IATLEventData* const pEventData)
{
	return PrepUnprepTriggerAsync(pTriggerData, pEventData, false);
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::ActivateTrigger(
	IATLAudioObjectData* const pAudioObjectData,
	IATLTriggerImplData const* const pTriggerData,
	IATLEventData* const pEventData)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);
	SATLTriggerImplData_wwise const* const pAKTriggerImplData = static_cast<SATLTriggerImplData_wwise const* const>(pTriggerData);
	SATLEventData_wwise* const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

	if ((pAKObjectData != NPTR) && (pAKTriggerImplData != NPTR) && (pAKEventData != NPTR))
	{
		AkGameObjectID nAKObjectID = AK_INVALID_GAME_OBJECT;

		if (pAKObjectData->bHasPosition)
		{
			nAKObjectID = pAKObjectData->nAKID;
			PostEnvironmentAmounts(pAKObjectData);
		}
		else
		{
			nAKObjectID = m_nDummyGameObjectID;
		}

		AkPlayingID const nAKPlayingID = AK::SoundEngine::PostEvent(
			pAKTriggerImplData->nAKID, 
			nAKObjectID, 
			AK_EndOfEvent,
			&EndEventCallback,
			pAKEventData);

		if (nAKPlayingID != AK_INVALID_PLAYING_ID)
		{
			pAKEventData->eAKType = eWET_PLAY;
			pAKEventData->nAKID = nAKPlayingID;
			eResult = eARS_SUCCESS;
		}
		else
		{
			// if Posting an Event failed, try to prepare it, if it isn't prepared already
			g_AudioImplLogger.Log(eALT_WARNING, "Failed to Post Wwise event %" PRISIZE_T, pAKEventData->nAKID);
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ActivateTrigger.");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::StopEvent(
	IATLAudioObjectData* const pAudioObjectData,
	IATLEventData const* const pEventData)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLEventData_wwise const* const pAKEventData = static_cast<SATLEventData_wwise const*>(pEventData);

	if (pAKEventData != NPTR)
	{
		switch(pAKEventData->eAKType)
		{
		case eWET_PLAY:
			{
				AK::SoundEngine::StopPlayingID(pAKEventData->nAKID, 10);
				eResult = eARS_SUCCESS;

				break;
			}
		default:
			{
				assert(false);//Stopping an event of this type is not supported yet
			}
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid EventData passed to the Wwise implementation of StopEvent.");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::StopAllEvents(IATLAudioObjectData* const pAudioObjectData)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);

	if (pAKObjectData != NPTR)
	{
		AkGameObjectID const nAKObjectID = pAKObjectData->bHasPosition ? pAKObjectData->nAKID : m_nDummyGameObjectID;

		AK::SoundEngine::StopAll(nAKObjectID);

		eResult = eARS_SUCCESS;
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of StopAllEvents.");
	}
	return eARS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetPosition(
	IATLAudioObjectData* const pAudioObjectData,
	SATLWorldPosition const& sWorldPosition)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);

	if (pAKObjectData != NPTR)
	{
		AkSoundPosition sAkSoundPos;
		FillAKObjectPosition(sWorldPosition, sAkSoundPos);

		AKRESULT const eAKResult	= AK::SoundEngine::SetPosition(pAKObjectData->nAKID, sAkSoundPos);

		if (IS_WWISE_OK(eAKResult)) 
		{
			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(eALT_WARNING, "Wwise SetPosition failed with AKRESULT: %d", eAKResult);
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetPosition.");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetEnvironment(
	IATLAudioObjectData* const pAudioObjectData,
	IATLEnvironmentImplData const* const pEnvironmentImplData,
	float const fAmount)
{
	static float const sEnvEpsilon = 0.0001f;
	
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);
	SATLEnvironmentImplData_wwise const* const pAKEnvironmentData = static_cast<SATLEnvironmentImplData_wwise const* const>(pEnvironmentImplData);

	if ((pAKObjectData != NPTR) && (pAKEnvironmentData != NPTR))
	{
		switch (pAKEnvironmentData->eType)
		{
		case eWAET_AUX_BUS:
			{
				float const fCurrentAmount = stl::find_in_map(
					pAKObjectData->cEnvironemntImplAmounts,
					pAKEnvironmentData->nAKBusID,
					-1.0f);

				if ((fCurrentAmount == -1.0f) || (fabs(fCurrentAmount - fAmount) > sEnvEpsilon))
				{
					pAKObjectData->cEnvironemntImplAmounts[pAKEnvironmentData->nAKBusID] = fAmount;
					pAKObjectData->bNeedsToUpdateEnvironments = true;
				}

				eResult = eARS_SUCCESS;

				break;
			}
		case eWAET_RTPC:
			{
				AkRtpcValue fAKValue = static_cast<AkRtpcValue>(pAKEnvironmentData->fMult * fAmount + pAKEnvironmentData->fShift);

				AKRESULT const eAKResult	= AK::SoundEngine::SetRTPCValue(pAKEnvironmentData->nAKRtpcID, fAKValue, pAKObjectData->nAKID);

				if (IS_WWISE_OK(eAKResult)) 
				{
					eResult = eARS_SUCCESS;
				}
				else
				{
					g_AudioImplLogger.Log(
						eALT_WARNING,
						"Wwise failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
						pAKEnvironmentData->nAKRtpcID,
						fAKValue,
						pAKObjectData->nAKID);
				}

				break;
			}
		default:
			{
				assert(false);//Unknown AudioEnvironmentImplementation type
			}

		}

	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData or EnvironmentData passed to the Wwise implementation of SetEnvironment");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetRtpc(
	IATLAudioObjectData* const pAudioObjectData,
	IATLRtpcImplData const* const pRtpcData, 
	float const fValue)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);
	SATLRtpcImplData_wwise const* const pAKRtpcData = static_cast<SATLRtpcImplData_wwise const* const>(pRtpcData);

	if ((pAKObjectData != NPTR) && (pAKRtpcData != NPTR))
	{
		AkRtpcValue fAKValue = static_cast<AkRtpcValue>(pAKRtpcData->m_fMult * fValue + pAKRtpcData->m_fShift);
		
		AKRESULT const eAKResult	= AK::SoundEngine::SetRTPCValue(pAKRtpcData->nAKID, fAKValue, pAKObjectData->nAKID);

		if (IS_WWISE_OK(eAKResult)) 
		{
			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(
				eALT_WARNING, 
				"Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T, 
				pAKRtpcData->nAKID, 
				static_cast<AkRtpcValue>(fValue),
				pAKObjectData->nAKID);
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetSwitchState(
	IATLAudioObjectData* const pAudioObjectData,
	IATLSwitchStateImplData const* const pSwitchStateData)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);
	SATLSwitchStateImplData_wwise const* const pAKSwitchStateData = static_cast<SATLSwitchStateImplData_wwise const* const>(pSwitchStateData);

	if ((pAKObjectData != NPTR) && (pAKSwitchStateData != NPTR))
	{
		switch (pAKSwitchStateData->eType)
		{
		case eWST_SWITCH:
			{
				AkGameObjectID const nAKObjectID = pAKObjectData->bHasPosition ? pAKObjectData->nAKID : m_nDummyGameObjectID;

				AKRESULT const eAKResult = AK::SoundEngine::SetSwitch(
					pAKSwitchStateData->nAKSwitchlID,
					pAKSwitchStateData->nAKStateID,
					nAKObjectID);

				if (IS_WWISE_OK(eAKResult))
				{
					eResult = eARS_SUCCESS;
				}
				else
				{
					g_AudioImplLogger.Log(
						eALT_WARNING,
						"Wwise failed to set the switch group %" PRISIZE_T " to state %" PRISIZE_T " on object %" PRISIZE_T,
						pAKSwitchStateData->nAKSwitchlID,
						pAKSwitchStateData->nAKStateID,
						nAKObjectID);
				}

				break;
			}
		case eWST_STATE:
			{
				AKRESULT const eAKResult = AK::SoundEngine::SetState(
					pAKSwitchStateData->nAKSwitchlID,
					pAKSwitchStateData->nAKStateID);

				if (IS_WWISE_OK(eAKResult))
				{
					eResult = eARS_SUCCESS;
				}
				else
				{
					g_AudioImplLogger.Log(
						eALT_WARNING,
						"Wwise failed to set the state group %" PRISIZE_T "to state %" PRISIZE_T,
						pAKSwitchStateData->nAKSwitchlID,
						pAKSwitchStateData->nAKStateID);
				}

				break;
			}
		case eWST_RTPC:
			{
				AkGameObjectID const nAKObjectID = pAKObjectData->nAKID;

				AKRESULT const eAKResult = AK::SoundEngine::SetRTPCValue(
					pAKSwitchStateData->nAKSwitchlID,
					static_cast<AkRtpcValue>(pAKSwitchStateData->fRtpcValue),
					nAKObjectID);

				if (IS_WWISE_OK(eAKResult))
				{
					eResult = eARS_SUCCESS;
				}
				else
				{
					g_AudioImplLogger.Log(
						eALT_WARNING, 
						"Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T, 
						pAKSwitchStateData->nAKSwitchlID,
						static_cast<AkRtpcValue>(pAKSwitchStateData->fRtpcValue),
						nAKObjectID);
				}

				break;
			}
		case eWST_NONE:
			{
				break;
			}
		default:
			{
				assert(false); // unknown EWwiseSwitchType
				g_AudioImplLogger.Log(eALT_WARNING, "Unknown EWwiseSwitchType: %" PRISIZE_T, pAKSwitchStateData->eType);

				break;
			}
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData or RtpcData passed to the Wwise implementation of SetRtpc");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetObstructionOcclusion(
	IATLAudioObjectData* const pAudioObjectData, 
	float const fObstruction, 
	float const fOcclusion)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);

	if (pAKObjectData != NPTR)
	{
		AKRESULT const eAKResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
			pAKObjectData->nAKID, 
			0,// only set the obstruction/occlusion for the default listener for now 
			static_cast<AkReal32>(fObstruction), 
			static_cast<AkReal32>(fOcclusion));

		if (IS_WWISE_OK(eAKResult))
		{
			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(
				eALT_WARNING, 
				"Wwise failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
				fObstruction,
				fOcclusion,
				pAKObjectData->nAKID);
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioObjectData passed to the Wwise implementation of SetObjectObstructionAndOcclusion");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::SetListenerPosition(
	IATLListenerData* const pListenerData,
	SATLWorldPosition const& oNewPosition)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLListenerData_wwise* const pAKListenerData = static_cast<SATLListenerData_wwise* const>(pListenerData);

	if (pAKListenerData != NPTR)
	{
		AkListenerPosition oAKListenerPos;
		FillAKListenerPosition(oNewPosition, oAKListenerPos);
		AKRESULT const eAKResult	= AK::SoundEngine::SetListenerPosition(oAKListenerPos, pAKListenerData->nAKID);

		if (IS_WWISE_OK(eAKResult))
		{
			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(eALT_WARNING, "Wwise SetListenerPosition failed with AKRESULT: %" PRISIZE_T, eAKResult);
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Invalid ATLListenerData passed to the Wwise implementation of SetListenerPosition");
	}
	return eResult;
}

//////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::RegisterInMemoryFile(SATLAudioFileEntryInfo* const pFileEntryInfo)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	if (pFileEntryInfo != NPTR)
	{
		SATLAudioFileEntryData_wwise* const pFileDataWwise = static_cast<SATLAudioFileEntryData_wwise*>(pFileEntryInfo->pImplData);

		if (pFileDataWwise != NPTR)
		{
			AkBankID nBankID = AK_INVALID_BANK_ID;

			AKRESULT const eAKResult = AK::SoundEngine::LoadBank(
				pFileEntryInfo->pFileData,
				static_cast<AkUInt32>(pFileEntryInfo->nSize),
				nBankID);

			if (IS_WWISE_OK(eAKResult))
			{
				pFileDataWwise->nAKBankID = nBankID;
				eResult = eARS_SUCCESS;
			}
			else
			{
				pFileDataWwise->nAKBankID = AK_INVALID_BANK_ID;
				g_AudioImplLogger.Log(eALT_ERROR, "Failed to load file %s\n", pFileEntryInfo->sFileName);
			}
		}
		else
		{
			g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to the Wwise implementation of RegisterInMemoryFile");
		}
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pFileEntryInfo)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	if (pFileEntryInfo != NPTR)
	{
		SATLAudioFileEntryData_wwise* const pFileDataWwise = static_cast<SATLAudioFileEntryData_wwise*>(pFileEntryInfo->pImplData);
		
		if (pFileDataWwise != NPTR)
		{
			AKRESULT const eAKResult = AK::SoundEngine::UnloadBank(pFileDataWwise->nAKBankID, pFileEntryInfo->pFileData);
			
			if (IS_WWISE_OK(eAKResult))
			{
				eResult = eARS_SUCCESS;
			}
			else
			{
				g_AudioImplLogger.Log(eALT_ERROR, "Wwise Failed to unregister in memory file %s\n", pFileEntryInfo->sFileName);
			}
		}
		else
		{
			g_AudioImplLogger.Log(eALT_ERROR, "Invalid AudioFileEntryData passed to the Wwise implementation of UnregisterInMemoryFile");
		}
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SATLAudioFileEntryInfo* const pFileEntryInfo)
{
	static CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sPath;

	EAudioRequestStatus eResult = eARS_FAILURE;

	if ((_stricmp(pAudioFileEntryNode->getTag(), sWwiseFileTag) == 0) && (pFileEntryInfo != NPTR))
	{
		char const* const sWwiseAudioFileEntryName = pAudioFileEntryNode->getAttr(sWwiseNameAttribute);

		if (sWwiseAudioFileEntryName != NPTR && sWwiseAudioFileEntryName[0] != '\0')
		{
			char const* const sWwiseLocalised = pAudioFileEntryNode->getAttr(sWwiseLocalisedAttribute);
			pFileEntryInfo->bLocalized = (sWwiseLocalised != NPTR) && (_stricmp(sWwiseLocalised, "true") == 0);

			pFileEntryInfo->sFileName = sWwiseAudioFileEntryName;

			pFileEntryInfo->nMemoryBlockAlignment	= AK_BANK_PLATFORM_DATA_ALIGNMENT;

			POOL_NEW(SATLAudioFileEntryData_wwise, pFileEntryInfo->pImplData);

			eResult = eARS_SUCCESS;
		}
		else
		{
			pFileEntryInfo->sFileName = NPTR;
			pFileEntryInfo->nMemoryBlockAlignment	= 0;
			pFileEntryInfo->pImplData = NPTR;
		}
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntry)
{
	POOL_FREE(pOldAudioFileEntry);
}

//////////////////////////////////////////////////////////////////////////
char const* const CAudioSystemImpl_wwise::GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo)
{
	char const* sResult = NPTR;

	if (pFileEntryInfo != NPTR)
	{
		sResult = pFileEntryInfo->bLocalized ? m_sLocalizedSoundBankFolder.c_str() : m_sRegularSoundBankFolder.c_str();
	}

	return sResult;
}

///////////////////////////////////////////////////////////////////////////
SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewGlobalAudioObjectData(TAudioObjectID const nObjectID)
{
	POOL_NEW_CREATE(SATLAudioObjectData_wwise, pNewObjectData)(AK_INVALID_GAME_OBJECT, false);
	return pNewObjectData;
}

///////////////////////////////////////////////////////////////////////////
SATLAudioObjectData_wwise* CAudioSystemImpl_wwise::NewAudioObjectData(TAudioObjectID const nObjectID)
{
	POOL_NEW_CREATE(SATLAudioObjectData_wwise, pNewObjectData)(static_cast<AkGameObjectID>(nObjectID), true);
	return pNewObjectData;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData)
{
	POOL_FREE(pOldObjectData);
}

///////////////////////////////////////////////////////////////////////////
SATLListenerData_wwise* CAudioSystemImpl_wwise::NewDefaultAudioListenerObjectData()
{
	POOL_NEW_CREATE(SATLListenerData_wwise, pNewObject)(0);
	return pNewObject;
}

///////////////////////////////////////////////////////////////////////////
SATLListenerData_wwise* CAudioSystemImpl_wwise::NewAudioListenerObjectData(uint const nIndex)
{
	POOL_NEW_CREATE(SATLListenerData_wwise, pNewObject)(nIndex);
	return pNewObject;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData)
{
	POOL_FREE(pOldListenerData);
}

//////////////////////////////////////////////////////////////////////////
SATLEventData_wwise* CAudioSystemImpl_wwise::NewAudioEventData(TAudioEventID const nEventID)
{
	POOL_NEW_CREATE(SATLEventData_wwise, pNewEvent)(nEventID);
	return pNewEvent;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioEventData(IATLEventData* const pOldEventData)
{
	POOL_FREE(pOldEventData);
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::ResetAudioEventData(IATLEventData* const pEventData)
{
	SATLEventData_wwise* const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

	if (pAKEventData != NPTR)
	{
		pAKEventData->eAKType = eWET_NONE;
		pAKEventData->nAKID = AK_INVALID_UNIQUE_ID;
	}
}

///////////////////////////////////////////////////////////////////////////
SATLTriggerImplData_wwise const* CAudioSystemImpl_wwise::NewAudioTriggerImplData(XmlNodeRef const pAudioTriggerImplNode)
{
	SATLTriggerImplData_wwise* pNewTriggerImpl = NPTR;

	if (_stricmp(pAudioTriggerImplNode->getTag(), sWwiseEventTag) == 0)
	{
		char const* const sWwiseEventName = pAudioTriggerImplNode->getAttr(sWwiseNameAttribute);
		AkUniqueID const nAKID = AK::SoundEngine::GetIDFromString(sWwiseEventName);//Does not check if the string represents an event!!!!

		if (nAKID != AK_INVALID_UNIQUE_ID)
		{
			POOL_NEW(SATLTriggerImplData_wwise, pNewTriggerImpl)(nAKID);
		}
		else
		{
			assert(false);
		}
	}

	return pNewTriggerImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioTriggerImplData(IATLTriggerImplData const* const pOldTriggerImplData)
{
	POOL_FREE_CONST(pOldTriggerImplData);
}

///////////////////////////////////////////////////////////////////////////
SATLRtpcImplData_wwise const* CAudioSystemImpl_wwise::NewAudioRtpcImplData(XmlNodeRef const pAudioRtpcNode)
{
	SATLRtpcImplData_wwise* pNewRtpcImpl = NPTR;

	AkRtpcID nAKRtpcID = AK_INVALID_RTPC_ID;
	float fMult = 1.0f;
	float fShift = 0.0f;

	ParseRtpcImpl(pAudioRtpcNode, nAKRtpcID, fMult, fShift);

	if (nAKRtpcID != AK_INVALID_RTPC_ID)
	{
		POOL_NEW(SATLRtpcImplData_wwise, pNewRtpcImpl)(nAKRtpcID, fMult, fShift);
	}

	return pNewRtpcImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioRtpcImplData(IATLRtpcImplData const* const pOldRtpcImplData)
{
	POOL_FREE_CONST(pOldRtpcImplData);
}

///////////////////////////////////////////////////////////////////////////
SATLSwitchStateImplData_wwise const* CAudioSystemImpl_wwise::NewAudioSwitchStateImplData(XmlNodeRef const pAudioSwitchNode)
{
	char const* const sWwiseSwitchNodeTag = pAudioSwitchNode->getTag();
	SATLSwitchStateImplData_wwise const* pNewSwitchImpl = NPTR;

	if (_stricmp(sWwiseSwitchNodeTag, sWwiseSwitchTag) == 0)
	{
		pNewSwitchImpl = ParseWwiseSwitchOrState(pAudioSwitchNode, eWST_SWITCH);
	}
	else if (_stricmp(sWwiseSwitchNodeTag, sWwiseStateTag) == 0)
	{
		pNewSwitchImpl = ParseWwiseSwitchOrState(pAudioSwitchNode, eWST_STATE);
	}
	else if (_stricmp(sWwiseSwitchNodeTag, sWwiseRtpcSwitchTag) == 0)
	{
		pNewSwitchImpl = ParseWwiseRtpcSwitch(pAudioSwitchNode);
	}
	else 
	{
		// Unknown Wwise switch tag!
		assert(false);
	}

	return pNewSwitchImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioSwitchStateImplData(IATLSwitchStateImplData const* const pOldAudioSwitchStateImplNode)
{
	POOL_FREE_CONST(pOldAudioSwitchStateImplNode);
}

///////////////////////////////////////////////////////////////////////////
SATLEnvironmentImplData_wwise const* CAudioSystemImpl_wwise::NewAudioEnvironmentImplData(XmlNodeRef const pAudioEnvironmentNode)
{
	SATLEnvironmentImplData_wwise* pNewEnvironmentImpl = NPTR;

	if (_stricmp(pAudioEnvironmentNode->getTag(), sWwiseAuxBusTag) == 0)
	{
		char const* const sWwiseAuxBusName = pAudioEnvironmentNode->getAttr(sWwiseNameAttribute);
		AkUniqueID const nAKBusID = AK::SoundEngine::GetIDFromString(sWwiseAuxBusName);//Does not check if the string represents an event!!!!

		if (nAKBusID != AK_INVALID_AUX_ID)
		{
			POOL_NEW(SATLEnvironmentImplData_wwise, pNewEnvironmentImpl)(eWAET_AUX_BUS, static_cast<AkAuxBusID>(nAKBusID));
		}
		else
		{
			assert(false);// unknown Aux Bus
		}
	}
	else if (_stricmp(pAudioEnvironmentNode->getTag(), sWwiseRtpcTag) == 0)
	{
		AkRtpcID nAKRtpcID = AK_INVALID_RTPC_ID;
		float fMult = 1.0f;
		float fShift = 0.0f;

		ParseRtpcImpl(pAudioEnvironmentNode, nAKRtpcID, fMult, fShift);

		if (nAKRtpcID != AK_INVALID_RTPC_ID)
		{
			POOL_NEW(SATLEnvironmentImplData_wwise, pNewEnvironmentImpl)(eWAET_RTPC, nAKRtpcID, fMult, fShift);
		}
		else
		{
			assert(false); // Unknown RTPC
		}
	}

	return pNewEnvironmentImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::DeleteAudioEnvironmentImplData(IATLEnvironmentImplData const* const pOldEnvironmentImplData)
{
	POOL_FREE_CONST(pOldEnvironmentImplData);
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioSystemImpl_wwise::GetImplementationNameString() const
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	return m_sFullImplString.c_str();
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	return NPTR;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const
{
	oMemoryInfo.nPrimaryPoolSize				= g_AudioImplMemoryPool.MemSize();
	oMemoryInfo.nPrimaryPoolUsedSize		= oMemoryInfo.nPrimaryPoolSize - g_AudioImplMemoryPool.MemFree();
	oMemoryInfo.nPrimaryPoolAllocations	= g_AudioImplMemoryPool.FragmentCount();

	oMemoryInfo.nBucketUsedSize					= g_AudioImplMemoryPool.GetSmallAllocsSize();
	oMemoryInfo.nBucketAllocations			= g_AudioImplMemoryPool.GetSmallAllocsCount();

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
	oMemoryInfo.nSecondaryPoolSize				= g_AudioImplMemoryPoolSoundSecondary.MemSize();
	oMemoryInfo.nSecondaryPoolUsedSize		= oMemoryInfo.nSecondaryPoolSize - g_AudioImplMemoryPoolSoundSecondary.MemFree();
	oMemoryInfo.nSecondaryPoolAllocations	= g_AudioImplMemoryPoolSoundSecondary.FragmentCount();
#else
	oMemoryInfo.nSecondaryPoolSize				= 0;
	oMemoryInfo.nSecondaryPoolUsedSize		= 0;
	oMemoryInfo.nSecondaryPoolAllocations	= 0;
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL
}

//////////////////////////////////////////////////////////////////////////
bool CAudioSystemImpl_wwise::SEnvPairCompare::operator() (std::pair<AkAuxBusID, float> const& oPair1, std::pair<AkAuxBusID, float> const& oPair2) const
{
	return (oPair1.second > oPair2.second);
}

//////////////////////////////////////////////////////////////////////////
SATLSwitchStateImplData_wwise const* CAudioSystemImpl_wwise::ParseWwiseSwitchOrState(
	XmlNodeRef pNode,
	EWwiseSwitchType eType)
{
	SATLSwitchStateImplData_wwise* pSwitchStateImpl = NPTR;

	char const* const sWwiseSwitchNodeName = pNode->getAttr(sWwiseNameAttribute);
	if ((sWwiseSwitchNodeName != NPTR) && (sWwiseSwitchNodeName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pValueNode(pNode->getChild(0));

		if (pValueNode && _stricmp(pValueNode->getTag(), sWwiseValueTag) == 0)
		{
			char const* sWwiseSwitchStateName	= pValueNode->getAttr(sWwiseNameAttribute);

			if ((sWwiseSwitchStateName != NPTR) && (sWwiseSwitchStateName[0] != 0))
			{
				AkUniqueID const nAKSwitchID = AK::SoundEngine::GetIDFromString(sWwiseSwitchNodeName);
				AkUniqueID const nAKSwitchStateID = AK::SoundEngine::GetIDFromString(sWwiseSwitchStateName);
				POOL_NEW(SATLSwitchStateImplData_wwise, pSwitchStateImpl)(eType, nAKSwitchID, nAKSwitchStateID);
			}
		}
	}
	else
	{
		g_AudioImplLogger.Log(
			eALT_WARNING,
			"A Wwise Switch or State %s inside ATLSwitchState needs to have exactly one WwiseValue.",
			sWwiseSwitchNodeName);
	}

	return pSwitchStateImpl;
}

///////////////////////////////////////////////////////////////////////////
SATLSwitchStateImplData_wwise const* CAudioSystemImpl_wwise::ParseWwiseRtpcSwitch(XmlNodeRef pNode)
{
	SATLSwitchStateImplData_wwise* pSwitchStateImpl = NPTR;

	char const* const sWwiseRtpcNodeName = pNode->getAttr(sWwiseNameAttribute);
	if ((sWwiseRtpcNodeName != NPTR) && (sWwiseRtpcNodeName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pValueNode(pNode->getChild(0));

		if (pValueNode && _stricmp(pValueNode->getTag(), sWwiseValueTag) == 0)
		{
			char const* sWwiseSwitchStateName	= pValueNode->getAttr(sWwiseNameAttribute);

			float fRtpcValue = 0.0f;

			if (pValueNode->getAttr(sWwiseValueAttribute, fRtpcValue))
			{
				AkUniqueID const nAKRtpcID = AK::SoundEngine::GetIDFromString(sWwiseRtpcNodeName);
				POOL_NEW(SATLSwitchStateImplData_wwise, pSwitchStateImpl)(eWST_RTPC, nAKRtpcID, nAKRtpcID, fRtpcValue);
			}
		}
	}
	else
	{
		g_AudioImplLogger.Log(
			eALT_WARNING,
			"A Wwise Rtpc %s inside ATLSwitchState needs to have exactly one WwiseValue.",
			sWwiseRtpcNodeName);
	}

	return pSwitchStateImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::ParseRtpcImpl(XmlNodeRef pNode, AkRtpcID& rAkRtpcID, float& rMult, float& rShift)
{
	SATLRtpcImplData_wwise* pNewRtpcImpl = NPTR;

	if (_stricmp(pNode->getTag(), sWwiseRtpcTag) == 0)
	{
		char const* const sWwiseRtpcName = pNode->getAttr(sWwiseNameAttribute);
		rAkRtpcID = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(sWwiseRtpcName));

		if (rAkRtpcID != AK_INVALID_RTPC_ID)
		{
			//the Wwise name is supplied 
			pNode->getAttr(sWwiseMutiplierAttribute, rMult);
			pNode->getAttr(sWwiseShiftAttribute, rShift);
		}
		else
		{
			// Invalid Wwise RTPC name!
			assert(false);
		}
	}
	else
	{
		// Unknown Wwise RTPC tag!
		assert(false);
	}
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerSync(
	IATLTriggerImplData const* const pTriggerData,
	bool bPrepare)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLTriggerImplData_wwise const* const pAKTriggerImplData = static_cast<SATLTriggerImplData_wwise const* const>(pTriggerData);

	if (pAKTriggerImplData != NPTR)
	{
		AkUniqueID nImplAKID = pAKTriggerImplData->nAKID;

		AKRESULT const eAKResult = AK::SoundEngine::PrepareEvent(
			bPrepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
			&nImplAKID,
			1);

		if (IS_WWISE_OK(eAKResult)) 
		{
			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(
				eALT_WARNING,
				"Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %" PRISIZE_T,
				bPrepare ? "Preparation_Load" : "Preparation_Unload",
				nImplAKID,
				eAKResult);
		}

	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, 
			"Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerSync",
			bPrepare ? "Prepare" : "Unprepare");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::PrepUnprepTriggerAsync(
	IATLTriggerImplData const* const pTriggerData,
	IATLEventData* const pEventData,
	bool bPrepare)
{
	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLTriggerImplData_wwise const* const pAKTriggerImplData = static_cast<SATLTriggerImplData_wwise const* const>(pTriggerData);
	SATLEventData_wwise* const pAKEventData = static_cast<SATLEventData_wwise*>(pEventData);

	if ((pAKTriggerImplData != NPTR) && (pAKEventData != NPTR))
	{
		AkUniqueID nImplAKID = pAKTriggerImplData->nAKID;

		AKRESULT const eAKResult = AK::SoundEngine::PrepareEvent(
			bPrepare ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
			&nImplAKID,
			1,
			&PrepareEventCallback,
			pAKEventData);

		if (IS_WWISE_OK(eAKResult)) 
		{
			pAKEventData->nAKID = nImplAKID;
			pAKEventData->eAKType = eWET_LOAD_UNLOAD;

			eResult = eARS_SUCCESS;
		}
		else
		{
			g_AudioImplLogger.Log(
				eALT_WARNING,
				"Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d", 
				bPrepare ? "Preparation_Load" : "Preparation_Unload",
				nImplAKID,
				eAKResult);	
		}
	}
	else
	{
		g_AudioImplLogger.Log(eALT_ERROR, 
			"Invalid ATLTriggerData or EventData passed to the Wwise implementation of %sTriggerAsync",
			bPrepare ? "Prepare" : "Unprepare");
	}

	return eResult;
}

///////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioSystemImpl_wwise::PostEnvironmentAmounts(IATLAudioObjectData* const pAudioObjectData)
{

	EAudioRequestStatus eResult = eARS_FAILURE;

	SATLAudioObjectData_wwise* const pAKObjectData = static_cast<SATLAudioObjectData_wwise* const>(pAudioObjectData);

	if (pAKObjectData != NPTR)
	{
		AkAuxSendValue aAuxValues[AK_MAX_AUX_PER_OBJ];
		uint32 nAuxCount = 0;

		SATLAudioObjectData_wwise::TEnvironmentImplMap::iterator iEnvPair = pAKObjectData->cEnvironemntImplAmounts.begin();
		SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator const iEnvStart = pAKObjectData->cEnvironemntImplAmounts.begin();
		SATLAudioObjectData_wwise::TEnvironmentImplMap::const_iterator const iEnvEnd = pAKObjectData->cEnvironemntImplAmounts.end();

		if (pAKObjectData->cEnvironemntImplAmounts.size() <= AK_MAX_AUX_PER_OBJ)
		{		
			for (; iEnvPair != iEnvEnd; ++nAuxCount)
			{
				float const fAmount = iEnvPair->second;

				aAuxValues[nAuxCount].auxBusID = iEnvPair->first;
				aAuxValues[nAuxCount].fControlValue = fAmount;

				// If an amount is zero, we still want to send it to the middleware, but we also want to remove it from the map.
				if (fAmount == 0.0f)
				{
					pAKObjectData->cEnvironemntImplAmounts.erase(iEnvPair++);
				}
				else
				{
					++iEnvPair;
				} 
			}
		}
		else
		{
			// sort the environments in order of decreasing amounts and take the first AK_MAX_AUX_PER_OBJ worth
			typedef std::set<std::pair<AkAuxBusID, float>, SEnvPairCompare> TEnvPairSet;
			TEnvPairSet cEnvPairs(iEnvStart, iEnvEnd);

			TEnvPairSet::const_iterator iSortedEnvPair = cEnvPairs.begin();
			TEnvPairSet::const_iterator const iSortedEnvEnd = cEnvPairs.end();

			for (; (iSortedEnvPair != iSortedEnvEnd) && (nAuxCount < AK_MAX_AUX_PER_OBJ); ++iSortedEnvPair, ++nAuxCount)
			{
				aAuxValues[nAuxCount].auxBusID = iSortedEnvPair->first;
				aAuxValues[nAuxCount].fControlValue = iSortedEnvPair->second;
			}

			//remove all Environments with 0.0 amounts
			while (iEnvPair != iEnvEnd)
			{
				if (iEnvPair->second == 0.0f)
				{
					pAKObjectData->cEnvironemntImplAmounts.erase(iEnvPair++);
				}
				else
				{
					++iEnvPair;
				}
			}
		}

		assert(nAuxCount <= AK_MAX_AUX_PER_OBJ);

		AKRESULT const eAKResult = AK::SoundEngine::SetGameObjectAuxSendValues(pAKObjectData->nAKID, aAuxValues, nAuxCount);
		bool bSuccess = IS_WWISE_OK(eAKResult);

		if (!bSuccess)
		{
			g_AudioImplLogger.Log(eALT_WARNING, 
				"Wwise SetGameObjectAuxSendValues failed on object %" PRISIZE_T " with AKRESULT: %d",
				pAKObjectData->nAKID,
				eAKResult);
		}

		pAKObjectData->bNeedsToUpdateEnvironments = false;
	}

	return eResult;
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::OnAudioSystemRefresh()
{
	AKRESULT eResult = AK::SoundEngine::UnloadBank(m_nInitBankID, NPTR);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Wwise failed to unload Init.bnk, returned the AKRESULT: %d", eResult);
		assert(false);
	}

	eResult = AK::SoundEngine::LoadBank(L"Init.bnk", AK_DEFAULT_POOL_ID, m_nInitBankID);

	if (eResult != AK_Success)
	{
		g_AudioImplLogger.Log(eALT_ERROR, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", eResult);
		assert(false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemImpl_wwise::SetLanguage(char const* const sLanguage)
{
	if (sLanguage != NPTR)
	{
		CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> sLanguageFolder;

		if (_stricmp(sLanguage, "english") == 0)
		{
			sLanguageFolder = "english(us)";
		}
		else
		{
			sLanguageFolder = sLanguage;// TODO: handle the other possible language variations
		}

		sLanguageFolder += "/";

		m_sLocalizedSoundBankFolder = m_sRegularSoundBankFolder;
		m_sLocalizedSoundBankFolder += sLanguageFolder.c_str();

		AkOSChar const* pTemp = NPTR;
		CONVERT_CHAR_TO_OSCHAR(sLanguageFolder.c_str(), pTemp);
		m_oFileIOHandler.SetLanguageFolder(pTemp);
	}
}
