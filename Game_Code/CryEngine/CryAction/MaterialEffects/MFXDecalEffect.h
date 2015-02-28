////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   IMFXEffect.h
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Decal effect
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __MFXDECALEFFECT_H__
#define __MFXDECALEFFECT_H__

#pragma once

#include "IMFXEffect.h"

struct SMFXDecalParams 
{
	string material;
	float minscale;
	float maxscale;
	float rotation;
	float lifetime;
	float growTime;
	bool assemble;
	bool forceedge;
};

class CMFXDecalEffect :
	public IMFXEffect
{
public:
	CMFXDecalEffect();
	virtual ~CMFXDecalEffect();
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void GetResources(SMFXResourceList& rlist);
	virtual void PreLoadAssets();
	virtual void ReleasePreLoadAssets();
	virtual IMFXEffectPtr Clone();
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_decalParams.material);
		IMFXEffect::GetMemoryUsage(pSizer);
	}
protected:

	void ReleaseMaterial();

	SMFXDecalParams m_decalParams;
	_smart_ptr<IMaterial> m_material;
};

#endif
