// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AudioSystemImplCVars.h"
#include "AudioSystemImpl_wwise.h"
#include <IAudioSystemImplementation.h>
#include <SoundAllocator.h>
#include <IAudioSystem.h>
#include <platform_impl.h>
#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#if defined(DURANGO)
#include <apu.h>
#include <shapexmacontext.h>
#endif // DURANGO

// Define global objects.
CAudioWwiseImplCVars	g_AudioWwiseImplCVars;
CSoundAllocator				g_AudioImplMemoryPool;
CAudioLogger					g_AudioImplLogger;

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
tMemoryPoolReferenced g_AudioImplMemoryPoolSoundSecondary;
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplWwise : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplWwise, "AudioModule_CryAudioImpl", 0xb4971e5dd02442c5, 0xb34a9ac0b4abfffd)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() {return "CryAudioImplWwise";}
	virtual const char *GetCategory() {return "CryAudio";}

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment &env,const SSystemInitParams &initParams)
	{
		bool bSuccess = false;

		// initialize memory pools

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Wwise Audio Implementation Memory Pool Primary");
		size_t const nPoolSize		= g_AudioWwiseImplCVars.m_nPrimaryPoolSize << 10;
		uint8* const pPoolMemory	= new uint8[nPoolSize];
		g_AudioImplMemoryPool.InitMem(nPoolSize, pPoolMemory, "Wwise Implementation Audio Pool");


#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
		size_t nSecondarySize = 0;
		void* pSecondaryMemory = NPTR;

#if defined(DURANGO)
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Wwise Implementation Audio Pool Secondary");
		nSecondarySize = g_AudioWwiseImplCVars.m_nSecondaryPoolSize << 10;

		APU_ADDRESS nTemp;
		HRESULT const eResult = ApuAlloc(&pSecondaryMemory, &nTemp, nSecondarySize, SHAPE_XMA_INPUT_BUFFER_ALIGNMENT);
		assert(eResult == S_OK);
#endif // DURANGO

		g_AudioImplMemoryPoolSoundSecondary.InitMem(nSecondarySize, (uint8*)pSecondaryMemory);
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

		POOL_NEW_CREATE(CAudioSystemImpl_wwise, pImpl);

		if (pImpl	!= NPTR)
		{
			g_AudioImplLogger.Log(eALT_ALWAYS, "<Audio> CryAudioImplWwise loaded");

			env.pAudioSystem->Initialize(pImpl);

			bSuccess = true;
		}
		else
		{
			g_AudioImplLogger.Log(eALT_ALWAYS, "<Audio> CryAudioImplWwise failed to load");
		}

		return bSuccess;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplWwise)

CEngineModule_CryAudioImplWwise::CEngineModule_CryAudioImplWwise()
{
	g_AudioWwiseImplCVars.RegisterVariables();
}

CEngineModule_CryAudioImplWwise::~CEngineModule_CryAudioImplWwise()
{
}

#include <CrtDebugStats.h>
