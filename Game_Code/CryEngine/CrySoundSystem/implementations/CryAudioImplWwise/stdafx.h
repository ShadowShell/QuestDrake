// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#if !defined(AFX_STDAFX_H__1E4C7FAC_937A_454F_BF0E_ABFBD056EEE5__INCLUDED_)
#define AFX_STDAFX_H__1E4C7FAC_937A_454F_BF0E_ABFBD056EEE5__INCLUDED_

#include <CryModuleDefs.h>
#include <platform.h>
#include <StlUtils.h>
#include <ProjectDefines.h>

#include <SoundAllocator.h>

#if !defined(_RELEASE)
// Define this to enable logging via CAudioLogger.
// We disable logging for Release builds
#define ENABLE_AUDIO_LOGGING
#endif // _RELEASE

#include <AudioLogger.h>

extern CSoundAllocator g_AudioImplMemoryPool;
extern CAudioLogger g_AudioImplLogger;

#define AUDIO_ALLOCATOR_MEMORY_POOL g_AudioImplMemoryPool
#include <STLSoundAllocator.h>

#if !defined(_RELEASE)
#define INCLUDE_WWISE_IMPL_PRODUCTION_CODE
#endif // _RELEASE

#if defined(DURANGO)
#define PROVIDE_WWISE_IMPL_SECONDARY_POOL
#endif

// Memory Allocation
#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
#include <CryPool/PoolAlloc.h>

typedef NCryPoolAlloc::CThreadSafe< NCryPoolAlloc::CBestFit< NCryPoolAlloc::CReferenced< NCryPoolAlloc::CMemoryDynamic, 4*1024, true >, NCryPoolAlloc::CListItemReference > > tMemoryPoolReferenced;

extern tMemoryPoolReferenced g_AudioImplMemoryPoolSoundSecondary;

//////////////////////////////////////////////////////////////////////////
inline void* Secondary_Allocate(size_t const nSize)
{
	// Secondary Memory is Referenced. To not loose the handle, more memory is allocated
	// and at the beginning the handle is saved.

	/* Allocate in Referenced Secondary Pool */
	uint32 const nAllocHandle = g_AudioImplMemoryPoolSoundSecondary.Allocate<uint32>(nSize, AUDIO_MEMORY_ALIGNMENT);
	assert(nAllocHandle > 0);
	void* pAlloc = NULL;

	if (nAllocHandle > 0)
	{
		pAlloc = g_AudioImplMemoryPoolSoundSecondary.Resolve<void*>(nAllocHandle);
	}

	return pAlloc;
}

//////////////////////////////////////////////////////////////////////////
inline bool Secondary_Free(void* pFree)
{
	// Secondary Memory is Referenced. To not loose the handle, more memory is allocated
	// and at the beginning the handle is saved.

	// retrieve handle
	bool bFreed = (pFree == NULL);//true by default when passing NULL
#ifdef PS3
	if(((uint32)pFree >= g_MemoryPoolSecondaryStart) && ((uint32)pFree < g_MemoryPoolSecondaryEnd))
#endif
	{
		uint32 const nAllocHandle = g_AudioImplMemoryPoolSoundSecondary.AddressToHandle(pFree);

		if (nAllocHandle > 0)
		{
			bFreed = g_AudioImplMemoryPoolSoundSecondary.Free(nAllocHandle);
		}
	}
	return bFreed;
}
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

// Win32 or Win64 or Durango
//////////////////////////////////////////////////////////////////////////
#if (defined(WIN32) || defined(WIN64) || defined(DURANGO)) && !defined(XENON) && !defined(PS3)
#include <windows.h>
#endif

// Win32
//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && !defined(WIN64) && !defined(XENON) && !defined(PS3) && !defined(DURANGO) && !defined(ORBIS) && !defined(CAFE)	
#endif // Win32

// Win64 (Note: WIN32 is automatically defined as well!)
//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && defined(WIN64) && !defined(XENON) && !defined(PS3) && !defined(DURANGO) && !defined(ORBIS) && !defined(CAFE)	
#endif // Win64

// DURANGO
//////////////////////////////////////////////////////////////////////////
#if defined(DURANGO)
//#include <xdk.h>
#endif // DURANGO

//////////////////////////////////////////////////////////////////////////
#if defined(ORBIS)
#define AK_PS4
#endif // ORBIS

// CAFE
//////////////////////////////////////////////////////////////////////////
#if defined(CAFE)
#endif // CAFE

// Mac
//////////////////////////////////////////////////////////////////////////
#if defined(MAC)
#endif

// Android
//////////////////////////////////////////////////////////////////////////
#if defined(ANDROID)
#endif

// iOS
//////////////////////////////////////////////////////////////////////////
#if defined(IOS)
#endif

// Linux
//////////////////////////////////////////////////////////////////////////
#if defined(LINUX) && !defined(ANDROID)
#endif // LINUX

#endif // AFX_STDAFX_H__1E4C7FAC_937A_454F_BF0E_ABFBD056EEE5__INCLUDED_
