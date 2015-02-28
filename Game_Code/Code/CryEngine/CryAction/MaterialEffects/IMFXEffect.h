////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   IMFXEffect.h
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Virtual base class for all derived effects
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __IMFXEFFECT_H__
#define __IMFXEFFECT_H__

#pragma once

#include <vector>
#include "Cry_Vector3.h"
#include "IMaterialEffects.h"

class IMFXEffect;
typedef _smart_ptr<IMFXEffect> IMFXEffectPtr;
typedef string TMFXNameId; // we use strings, and with clever assigning we minimize duplicates

class IMFXEffect : public _reference_target_t
{
public:
	enum MFXEffectMedium {
		MEDIUM_WATER = 0,
		MEDIUM_ZEROG,
		MEDIUM_NORMAL,
		MEDIUM_ALL
	};

	IMFXEffect();
	virtual ~IMFXEffect();

	virtual void Execute(SMFXRunTimeEffectParams& params) = 0;
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual void AddChild(IMFXEffectPtr newChild);
	virtual IMFXEffectPtr Clone() = 0;
	virtual void MakeDerivative(IMFXEffectPtr parent);
	virtual void Build(XmlNodeRef& node);
	virtual bool CanExecute(SMFXRunTimeEffectParams& params);
	virtual void SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue);
	virtual void GetResources(SMFXResourceList& rlist) = 0;
	virtual void PreLoadAssets()
	{
		for (std::vector<IMFXEffectPtr>::iterator iter = m_effects.begin(); iter != m_effects.end(); ++iter)
			(*iter)->PreLoadAssets();
	}
	virtual void ReleasePreLoadAssets()
	{
		for (std::vector<IMFXEffectPtr>::iterator iter = m_effects.begin(); iter != m_effects.end(); ++iter)
			(*iter)->ReleasePreLoadAssets();
	}
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_effectParams.name);
		pSizer->AddObject(m_effectParams.libName);
		pSizer->AddObject(m_effects);		
	}
	
public:
	struct SMFXEffectParams
	{
		SMFXEffectParams ()
		{
			effectId = InvalidEffectId;
			delay = 0.0f;
		}
		TMFXNameId libName;     // only top level effects have valid lib name
		TMFXNameId name;        // only top level effects have valid effect name
		float delay;
		MFXEffectMedium medium;
		TMFXEffectId effectId; // only top level effects have valid ids!
	};

	SMFXEffectParams m_effectParams;
	std::vector<IMFXEffectPtr> m_effects;
};

#endif
