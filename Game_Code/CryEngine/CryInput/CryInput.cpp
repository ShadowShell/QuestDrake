// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include "BaseInput.h"

#include "Synergy/SynergyContext.h"
#include "Synergy/SynergyKeyboard.h"
#include "Synergy/SynergyMouse.h"

#if defined(WIN32)
#ifndef _LIB
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}
#endif
#endif // WIN32


//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryInput : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryInput, "EngineModule_CryInput", 0x3cc0516071bb44f6, 0xae525949f30277f9)

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() { return "CryInput"; };
	virtual const char *GetCategory() { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams )
	{
		ISystem* pSystem = env.pSystem;

		IInput *pInput = 0;
		if (!gEnv->IsDedicated())
		{
#if defined(USE_DXINPUT)
			pInput = new CDXInput(pSystem, (HWND) initParams.hWnd);
#elif defined(USE_DURANGOINPUT)
			pInput = new CDurangoInput(pSystem);
#elif defined(USE_LINUXINPUT)
			pInput = new CLinuxInput(pSystem);
#elif defined(USE_ORBIS_INPUT)
			pInput = new COrbisInput(pSystem);
#else
			pInput = new CBaseInput();
#endif
		}
		else
			pInput = new CBaseInput();

		if (!pInput->Init())
		{
			delete pInput;
			return false;
		}

#ifdef USE_SYNERGY_INPUT
		const char *pServer=g_pInputCVars->i_synergyServer->GetString();
		if (pServer && pServer[0]!='\0')
		{
			_smart_ptr<CSynergyContext> pContext=new CSynergyContext(g_pInputCVars->i_synergyScreenName->GetString(), pServer);
			pInput->AddInputDevice(new CSynergyKeyboard(*pInput, pContext));
			pInput->AddInputDevice(new CSynergyMouse(*pInput, pContext));
		}
#endif

		env.pInput = pInput;

		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryInput)

CEngineModule_CryInput::CEngineModule_CryInput()
{
};

CEngineModule_CryInput::~CEngineModule_CryInput()
{
};

#include <CrtDebugStats.h>

