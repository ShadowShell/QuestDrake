// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "ScriptBind_Sound.h"
#include <IAudioSystem.h>

CScriptBind_Sound::CScriptBind_Sound(IScriptSystem* pScriptSystem, ISystem* pSystem)
{
	CScriptableBase::Init(pScriptSystem, pSystem);
	SetGlobalName("Sound");

#undef SCRIPT_REG_CLASSNAME 
#define SCRIPT_REG_CLASSNAME &CScriptBind_Sound::

	// AudioSystem
	SCRIPT_REG_TEMPLFUNC(GetAudioTriggerID, "sTriggerName");
	SCRIPT_REG_TEMPLFUNC(GetAudioSwitchID, "sSwitchName");
	SCRIPT_REG_TEMPLFUNC(GetAudioSwitchStateID, "hSwitchID, sStateName");
	SCRIPT_REG_TEMPLFUNC(GetAudioRtpcID, "sRtpcName");
	SCRIPT_REG_TEMPLFUNC(GetAudioEnvironmentID, "sEnvironmentName");

}

//////////////////////////////////////////////////////////////////////////
CScriptBind_Sound::~CScriptBind_Sound()
{
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioTriggerID(IFunctionHandler* pH, char const* const sTriggerName)
{
	if ((sTriggerName != NPTR) && (sTriggerName[0] != '\0'))
	{
		TAudioControlID nTriggerID = INVALID_AUDIO_CONTROL_ID;
		if (gEnv->pAudioSystem->GetAudioTriggerID(sTriggerName, nTriggerID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nTriggerID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchID(IFunctionHandler* pH, char const* const sSwitchName)
{
	if ((sSwitchName != NPTR) && (sSwitchName[0] != '\0'))
	{
		TAudioControlID nSwitchID = INVALID_AUDIO_CONTROL_ID;
		if (gEnv->pAudioSystem->GetAudioSwitchID(sSwitchName, nSwitchID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nSwitchID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioSwitchStateID(IFunctionHandler* pH, ScriptHandle const hSwitchID, char const* const sSwitchStateName)
{
	if ((sSwitchStateName != NPTR) && (sSwitchStateName[0] != '\0'))
	{
		TAudioSwitchStateID nSwitchStateID = INVALID_AUDIO_SWITCH_STATE_ID;
		TAudioControlID nSwitchID = HandleToInt<TAudioControlID>(hSwitchID);
		if (gEnv->pAudioSystem->GetAudioSwitchStateID(nSwitchID, sSwitchStateName, nSwitchStateID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nSwitchStateID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioRtpcID(IFunctionHandler* pH, char const* const sRtpcName)
{
	if ((sRtpcName != NPTR) && (sRtpcName[0] != '\0'))
	{
		TAudioControlID nRtpcID = INVALID_AUDIO_CONTROL_ID;
		if (gEnv->pAudioSystem->GetAudioRtpcID(sRtpcName, nRtpcID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nRtpcID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Sound::GetAudioEnvironmentID(IFunctionHandler* pH, char const* const sEnvironmentName)
{
	if ((sEnvironmentName != NPTR) && (sEnvironmentName[0] != '\0'))
	{
		TAudioEnvironmentID nEnvironmentID = INVALID_AUDIO_ENVIRONMENT_ID;
		if (gEnv->pAudioSystem->GetAudioEnvironmentID(sEnvironmentName, nEnvironmentID))
		{
			// ID retrieved successfully
			return pH->EndFunction(IntToHandle(nEnvironmentID));
		}
		else
		{
			return pH->EndFunction();
		}
	}

	return pH->EndFunction();
}
