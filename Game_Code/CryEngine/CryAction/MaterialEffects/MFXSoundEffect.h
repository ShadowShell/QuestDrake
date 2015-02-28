// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#include "IMFXEffect.h"
#pragma once
#include "ISystem.h"

struct SMFXSoundParams {
	string name;
	bool bIgnoreDistMult;
};

class CMFXSoundEffect :
	public IMFXEffect
{
public:
	CMFXSoundEffect();
	virtual ~CMFXSoundEffect();
	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void GetResources(SMFXResourceList& rlist);
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual IMFXEffectPtr Clone();
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{		
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_soundParams.name);
		IMFXEffect::GetMemoryUsage(pSizer);
	}
protected:
	SMFXSoundParams m_soundParams;
};
