////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   MFXForceFeedbackEffect.h
//  Version:     v1.00
//  Created:     8/4/2010 by Benito Gangoso Rodriguez
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MFXFORCEFEEDBACKEFFECT_H__
#define __MFXFORCEFEEDBACKEFFECT_H__

#include "IMFXEffect.h"

struct SMFXForceFeedbackParams 
{
	string forceFeedbackEventName;
	float intensityFallOffMinDistanceSqr;
	float intensityFallOffMaxDistanceSqr;

	SMFXForceFeedbackParams()
		: intensityFallOffMinDistanceSqr(0.0f)
		, intensityFallOffMaxDistanceSqr(25.0f)
	{

	}
};

class CMFXForceFeedbackEffect :
	public IMFXEffect
{
public:
	CMFXForceFeedbackEffect();
	virtual ~CMFXForceFeedbackEffect();
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void GetResources(SMFXResourceList& rlist);
	virtual IMFXEffectPtr Clone();
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
		s->Add(m_forceFeedbackParams.forceFeedbackEventName);
		IMFXEffect::GetMemoryUsage(s);
	}
protected:
	SMFXForceFeedbackParams m_forceFeedbackParams;
};

#endif