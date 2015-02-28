// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "IMFXEffect.h"
#include "MFXEffect.h"
#include "MFXRandomEffect.h"
#include "MFXSoundEffect.h"
#include "MFXParticleEffect.h"
#include "MFXDecalEffect.h"
#include "MFXFlowGraphEffect.h"
#include "MFXForceFeedbackFX.h"
#include "GameXmlParamReader.h"

IMFXEffect::IMFXEffect()
{

}

IMFXEffect::~IMFXEffect()
{
}

void IMFXEffect::ReadXMLNode(XmlNodeRef& node)
{
	const char *nameattr = node->getAttr("name");
	if (nameattr && *nameattr)
	{
		m_effectParams.name = nameattr;
	}
	else
	{
		m_effectParams.name = node->getTag();
	}
	const char *medium = node->getAttr("medium");
	if (medium && *medium)
	{
		if (!strcmp("zerog", medium))
			m_effectParams.medium = MEDIUM_ZEROG;
		else if (!strcmp("water", medium))
			m_effectParams.medium = MEDIUM_WATER;
		else if (!strcmp("normal", medium))
			m_effectParams.medium = MEDIUM_NORMAL;
		else
			m_effectParams.medium = MEDIUM_ALL;
	}
	else
		m_effectParams.medium = MEDIUM_ALL;

	float delay = 0.0f;
	const char *delayStr = node->getAttr("delay");
	if (delayStr && *delayStr)
		delay = (float)atof(delayStr);
	m_effectParams.delay = delay;
}

void IMFXEffect::AddChild(IMFXEffectPtr newChild)
{
	m_effects.push_back(newChild);
}

void IMFXEffect::MakeDerivative(IMFXEffectPtr parent)
{
	m_effects.reserve(m_effects.size() + parent->m_effects.size());
	std::vector< IMFXEffectPtr >::iterator parentIter = parent->m_effects.begin();
	while (parentIter != parent->m_effects.end())
	{
		IMFXEffectPtr pParentChildEffect = *parentIter;
		if (pParentChildEffect)
		{
			const TMFXNameId& parentChildEffectName = pParentChildEffect->m_effectParams.name;
			bool bDoCopy = true;
			// If any of my effect names match the parent effect, don't copy the parent effect
			std::vector< IMFXEffectPtr >::iterator effectsIter = m_effects.begin();
			std::vector< IMFXEffectPtr >::iterator effectsIterEnd = m_effects.end();
			while (effectsIter != effectsIterEnd)
			{
				IMFXEffectPtr& pEffectPtr = *effectsIter;
				if (pEffectPtr)
				{
					const TMFXNameId& effectName = pEffectPtr->m_effectParams.name;
					if (effectName == parentChildEffectName)
					{
						bDoCopy = false;
						break;
					}
				}
				++effectsIter;
			}
			if (bDoCopy)
			{
				IMFXEffectPtr pClone = pParentChildEffect->Clone();
				AddChild(pClone);
			}
		}
		++parentIter;
	}
}

void IMFXEffect::Build(XmlNodeRef& node)
{
	ReadXMLNode(node);
	
	size_t nEffects = 0;
	const CGameXmlParamReader reader(node);
	const int childCount = reader.GetUnfilteredChildCount();
	for(int i=0; i < childCount; i++)
	{
		XmlNodeRef curNode = reader.GetFilteredChildAt(i);
		if (curNode == (IXmlNode*)NULL)
			continue;
		const char* nodeName = curNode->getTag();
		if (nodeName == 0 || *nodeName==0)    // invalid or empty tag
			continue;
		if (!stricmp("Effect", nodeName) || !stricmp("RandEffect", nodeName) || !stricmp("Particle", nodeName) ||
				!stricmp("Sound", nodeName) || !stricmp("Decal", nodeName) || !stricmp("FlowGraph", nodeName) || !stricmp("ForceFeedback", nodeName))
		{
			++nEffects;
		}
	}	
	m_effects.reserve(nEffects);

	for(int i=0; i < childCount; i++)
	{
		XmlNodeRef curNode = reader.GetFilteredChildAt(i);
		if (curNode == (IXmlNode*)NULL)
			continue;

		const char* nodeName = curNode->getTag();
		if (nodeName == 0 || *nodeName==0)    // invalid or empty tag
			continue;

		if (!stricmp("Effect", nodeName))
		{
			CMFXEffect *effect = new CMFXEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("RandEffect", nodeName))
		{
			CMFXRandomEffect *effect = new CMFXRandomEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("Particle", nodeName))
		{
			CMFXParticleEffect *effect = new CMFXParticleEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("Sound", nodeName))
		{
			CMFXSoundEffect *effect = new CMFXSoundEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("Decal", nodeName))
		{
			CMFXDecalEffect *effect = new CMFXDecalEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("FlowGraph", nodeName))
		{
			CMFXFlowGraphEffect *effect = new CMFXFlowGraphEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
		else if (!stricmp("ForceFeedback", nodeName))
		{
			CMFXForceFeedbackEffect *effect = new CMFXForceFeedbackEffect();
			effect->Build(curNode);
			AddChild(effect);
		}
	}
}

bool IMFXEffect::CanExecute(SMFXRunTimeEffectParams& params)
{
	if ((m_effectParams.medium == MEDIUM_WATER && params.inWater) ||
		(m_effectParams.medium == MEDIUM_ZEROG && params.inZeroG && !params.inWater) ||
		(m_effectParams.medium == MEDIUM_NORMAL && !params.inZeroG && !params.inWater) ||
		(m_effectParams.medium == MEDIUM_ALL))
		return true;
	return false;
}

void IMFXEffect::SetCustomParameter(const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{

}
