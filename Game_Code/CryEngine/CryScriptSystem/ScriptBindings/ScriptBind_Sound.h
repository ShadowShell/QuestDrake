// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef SCRIPTBIND_SOUND_H_INCLUDED
#define SCRIPTBIND_SOUND_H_INCLUDED

#include <IScriptSystem.h>

struct IMusicSystem;
struct IAudioSystem;

/*! <remarks>These function will never be called from C-Code. They're script-exclusive.</remarks>*/
class CScriptBind_Sound : public CScriptableBase
{
public:

	CScriptBind_Sound(IScriptSystem* pScriptSystem, ISystem* pSystem);
	virtual ~CScriptBind_Sound();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	//! <code>Sound.GetAudioTriggerID(char const* const sTriggerName)</code>
	//! <description>Get the trigger TAudioControlID (wrapped into a ScriptHandle).</description>
	//!		<param name="sTriggerName">unique name of an audio trigger</param>
	//! <returns>ScriptHandle with the TAudioControlID value, or nil if the sTriggerName is not found.</returns>
	int GetAudioTriggerID(IFunctionHandler* pH, char const* const sTriggerName);

	//! <code>Sound.GetAudioSwitchID(char const* const sSwitchName)</code>
	//! <description>Get the switch TAudioControlID (wrapped into a ScriptHandle).</description>
	//!		<param name="sSwitchName">unique name of an audio switch</param>
	//! <returns>ScriptHandle with the TAudioControlID value, or nil if the sSwitchName is not found.</returns>
	int GetAudioSwitchID(IFunctionHandler* pH, char const* const sSwitchName);

	//! <code>Sound.GetAudioSwitchStateID(ScriptHandle const hSwitchID, char const* const sSwitchStateName)</code>
	//! <description>Get the SwitchState TAudioSwitchStatelID (wrapped into a ScriptHandle).</description>
	//!		<param name="sSwitchStateName">unique name of an audio switch state</param>
	//! <returns>ScriptHandle with the TAudioSwitchStateID value, or nil if the sSwitchStateName is not found.</returns>
	int GetAudioSwitchStateID(IFunctionHandler* pH, ScriptHandle const hSwitchID, char const* const sSwitchStateName);

	//! <code>Sound.GetAudioRtpcID(char const* const sRtpcName)</code>
	//! <description>Get the RTPC TAudioControlID (wrapped into a ScriptHandle)
	//!		<param name="sRtpcName">unique name of an audio RTPC</param>
	//! <returns>ScriptHandle with the TAudioControlID value, or nil if the sRtpcName is not found.</returns>
	int GetAudioRtpcID(IFunctionHandler* pH, char const* const sRtpcName);

	//! <code>Sound.GetAudioEnvironmentID(char const* const sEnvironmentName)</code>
	//! <description>Get the Audio Environment TAudioEnvironmentID (wrapped into a ScriptHandle).</description>
	//!		<param name="sEnvironmentName">unique name of an Audio Environment</param>
	//! <returns>ScriptHandle with the TAudioEnvironmentID value, or nil if the sEnvironmentName is not found.</returns>
	int GetAudioEnvironmentID(IFunctionHandler* pH, char const* const sEnvironmentName);
};

#endif // __ScriptBind_Sound_h__