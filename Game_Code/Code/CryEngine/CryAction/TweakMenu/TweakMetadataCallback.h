/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2011.
-------------------------------------------------------------------------

Description: 
	Header for tweak item that calls a user-supplied function

*************************************************************************/

#ifndef __TWEAKMETADATACALLBACK_H__
#define __TWEAKMETADATACALLBACK_H__

#pragma once

#include "TweakMetadata.h"

class CTweakMetadataCallback : public CTweakMetadata
{
public:
	CTweakMetadataCallback(CTweakMenuController* pMenuController, const string& command);
	~CTweakMetadataCallback() {}

	void Init(TweakCallbackFunction func, void* pUserData)
	{
		m_pFunction = func;
		m_pUserData = pUserData;
	}

	string GetValue(void) { return "";}

	bool DecreaseValue(void) { return ChangeValue(false); }
	bool IncreaseValue(void) { return ChangeValue(true); }

protected:

	// Increment/decrement the value
	bool ChangeValue(bool bIncrement);

	TweakCallbackFunction m_pFunction;
	void* m_pUserData;
};

#endif
