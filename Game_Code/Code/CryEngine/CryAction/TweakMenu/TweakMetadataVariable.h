/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for Cvar tweak item

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/


#ifndef __CTWEAKMETADATAVARIABLE_H__
#define __CTWEAKMETADATAVARIABLE_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakMetadata.h"

//-------------------------------------------------------------------------

class CTweakMetadataVariable : public CTweakMetadata
{
public:
	
	CTweakMetadataVariable(CTweakMenuController* pController, const string& command);
	~CTweakMetadataVariable() {};

	void Init(int* pValue, float min, float max, float delta);
	void Init(float* pValue, float min, float max, float delta);

	string GetValue(void);

	bool DecreaseValue(void) { return ChangeValue(false); }
	bool IncreaseValue(void) { return ChangeValue(true); }

protected:

	// Increment/decrement the value
	bool ChangeValue(bool bIncrement);

	int* m_pIntVariable;
	float* m_pFloatVariable;
};

#endif // __CTWEAKMETADATAVARIABLE_H__
