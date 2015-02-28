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


#ifndef __CTWEAKMETADATACVAR_H__
#define __CTWEAKMETADATACVAR_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakMetadata.h"

//-------------------------------------------------------------------------

class CTweakMetadataCVAR : public CTweakMetadata
{
public:
	
	CTweakMetadataCVAR(CTweakMenuController* pController, const string& command);
	~CTweakMetadataCVAR() {};

	void InitItem(XmlNodeRef xml);

	string GetValue(void);

	bool DecreaseValue(void) { return ChangeValue(false); }

	bool IncreaseValue(void) { return ChangeValue(true); }

protected:

	// Increment/decrement the value
	bool ChangeValue(bool bIncrement);

	// Wraps fetching the CVAR
	ICVar * GetCVar(void) const;

	// Type of the CVAR
	int m_CVarType;

	// Should CVAR be propagated over using rcon
	bool m_bUseRconCommand;
};

#endif // __CTWEAKMETADATACVAR_H__
