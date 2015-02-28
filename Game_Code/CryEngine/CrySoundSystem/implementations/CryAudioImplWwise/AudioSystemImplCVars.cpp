// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "stdafx.h"
#include "AudioSystemImplCVars.h"
#include <IConsole.h>

//////////////////////////////////////////////////////////////////////////
CAudioWwiseImplCVars::CAudioWwiseImplCVars()
	:	m_nPrimaryPoolSize(0)
	,	m_nSecondaryPoolSize(0)
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	, m_nEnableCommSystem(0)
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
{
}

//////////////////////////////////////////////////////////////////////////
CAudioWwiseImplCVars::~CAudioWwiseImplCVars()
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioWwiseImplCVars::RegisterVariables()
{
#if defined(WIN32) || defined(WIN64)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(DURANGO)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 32<<10;		// 32 MiB
#elif defined(ORBIS)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(MAC)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(LINUX)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(CAFE)
	m_nPrimaryPoolSize		= 128<<10;	// 128 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(IOS)
	m_nPrimaryPoolSize		= 8<<10;		// 8 MiB
	m_nSecondaryPoolSize	= 0;
#elif defined(ANDROID)
	m_nPrimaryPoolSize		= 8<<10;		// 8 MiB
	m_nSecondaryPoolSize	= 0;
#else
#error "Undefined platform."
#endif

	REGISTER_CVAR2("s_WwisePrimaryPoolSize", &m_nPrimaryPoolSize, m_nPrimaryPoolSize, VF_REQUIRE_APP_RESTART,
		"Specifies the size (in KiB) of the memory pool to be used by the Wwise audio system implementation.\n"
		"Usage: s_WwisePrimaryPoolSize [0/...]\n"
		"Default PC: 131072 (128 MiB), Xbox360: 131072 (128 MiB), XboxOne: 131072 (128 MiB), PS3: 131072 (128 MiB), PS4: 131072 (128 MiB), MAC: 131072 (128 MiB), Linux: 131072 (128 MiB), WiiU: 131072 (128 MiB), IOS: 8192 (8 MiB), Android: 8192 (8 MiB)\n");

	REGISTER_CVAR2("s_WwiseSecondaryPoolSize", &m_nSecondaryPoolSize, m_nSecondaryPoolSize, VF_REQUIRE_APP_RESTART,
		"Specifies the size (in KiB) of the memory pool to be used by the Wwise audio system implementation.\n"
		"Usage: s_WwiseSecondaryPoolSize [0/...]\n"
		"Default PC: 0, Xbox360: 32768 (32 MiB), XboxOne: 32768 (32 MiB), PS3: 0, PS4: 0, MAC: 0, Linux: 0, WiiU: 0, IOS: 0, Android: 0\n");

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	REGISTER_CVAR2("s_WwiseEnableCommSystem", &m_nEnableCommSystem, 0, VF_REQUIRE_APP_RESTART,
		"Specifies whether Wwise should initialize using its Comm system or not.\n"
		"This cvar is only available in non-release builds.\n"
		"Usage: s_WwiseEnableCommSystem [0/1]\n"
		"Default: 0 (off)\n");
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CAudioWwiseImplCVars::UnregisterVariables()
{
	IConsole* const pConsole = gEnv->pConsole;
	assert(pConsole);

	pConsole->UnregisterVariable("s_WwisePrimaryPoolSize");
	pConsole->UnregisterVariable("s_WwiseSecondaryPoolSize");

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	pConsole->UnregisterVariable("s_WwiseEnableCommSystem");
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
