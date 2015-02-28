/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2011.
-------------------------------------------------------------------------

Description: 
Header for gametoken tweak item

*************************************************************************/


#ifndef __CTWEAKMETADATAGAMETOKEN_H__
#define __CTWEAKMETADATAGAMETOKEN_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakMetadata.h"

//-------------------------------------------------------------------------

struct IGameToken;

//-------------------------------------------------------------------------

class CTweakMetadataGameToken : public CTweakMetadata
{
public:

	CTweakMetadataGameToken(CTweakMenuController* pController, const string& command);
	~CTweakMetadataGameToken() {};

	void InitItem(XmlNodeRef xml);

	string GetValue(void);

	bool DecreaseValue(void) { return ChangeValue(false); }
	bool IncreaseValue(void) { return ChangeValue(true); }

protected:

	// Increment/decrement the value
	bool ChangeValue(bool bIncrement);

	// Wraps fetching the gametoken
	IGameToken* GetGameToken(void) const;
};

#endif // __CTWEAKMETADATAGAMETOKEN_H__
