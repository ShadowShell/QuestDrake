////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MFXEffect.h
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Top level effect, only containing child effects
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __MFXEFFECT_H__
#define __MFXEFFECT_H__

#include "IMFXEffect.h"
#pragma once

class CMFXEffect :
	public IMFXEffect
{
public:
	CMFXEffect();
	virtual ~CMFXEffect();
	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue);
	virtual IMFXEffectPtr Clone();
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual void GetResources(SMFXResourceList& rlist);
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const;
};

#endif
