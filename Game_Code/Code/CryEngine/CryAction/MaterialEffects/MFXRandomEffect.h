////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MFXRandomEffect.h
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Random effect (randomly plays one of its child effects)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __MFXRANDOMEFFECT_H__
#define __MFXRANDOMEFFECT_H__

#pragma once

#include "IMFXEffect.h"

struct SMFXRandomParams
{
  SMFXParticleEffectParams particleParams;
};

class CMFXRandomEffect :
	public IMFXEffect
{
public:
	static void StaticReset();

public:
	CMFXRandomEffect();
	virtual ~CMFXRandomEffect();

	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void GetResources(SMFXResourceList& rlist);
	virtual IMFXEffectPtr Clone();
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{		
		IMFXEffect::GetMemoryUsage(pSizer);
	}

	size_t static GetMemoryUsage();
private:
	IMFXEffectPtr ChooseCandidate(SMFXRunTimeEffectParams& params);
  SMFXRandomParams m_params;

private:
	static DynArray<int> s_tmpCandidatesArray;
};

#endif
