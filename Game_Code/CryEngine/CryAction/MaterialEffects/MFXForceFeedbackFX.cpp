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

#include "StdAfx.h"
#include "MFXForceFeedbackFX.h"
#include "CryAction.h"
#include "IForceFeedbackSystem.h"


CMFXForceFeedbackEffect::CMFXForceFeedbackEffect()
{
}

CMFXForceFeedbackEffect::~CMFXForceFeedbackEffect()
{

}

void CMFXForceFeedbackEffect::ReadXMLNode(XmlNodeRef &node)
{
	IMFXEffect::ReadXMLNode(node);

	m_forceFeedbackParams.forceFeedbackEventName = node->getAttr("name");

	float minFallOffDistance = 0.0f;
	node->getAttr("minFallOffDistance", minFallOffDistance);
	float maxFallOffDistance = 5.0f;
	node->getAttr("maxFallOffDistance", maxFallOffDistance);

	m_forceFeedbackParams.intensityFallOffMinDistanceSqr = minFallOffDistance * minFallOffDistance;
	m_forceFeedbackParams.intensityFallOffMaxDistanceSqr = maxFallOffDistance * maxFallOffDistance;
}

IMFXEffectPtr CMFXForceFeedbackEffect::Clone()
{
	CMFXForceFeedbackEffect* pClone = new CMFXForceFeedbackEffect();
	pClone->m_forceFeedbackParams = m_forceFeedbackParams;
	pClone->m_effectParams = m_effectParams;
	return pClone;
}

void CMFXForceFeedbackEffect::Execute(SMFXRunTimeEffectParams& params)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	if (params.playflags & MFX_PLAY_FORCEFEEDBACK)
	{
		float distanceToPlayerSqr = FLT_MAX;
		IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if (pClientActor)
		{
			distanceToPlayerSqr = (pClientActor->GetEntity()->GetWorldPos() - params.pos).GetLengthSquared();
		}

		const float testDistanceSqr = clamp_tpl(distanceToPlayerSqr, m_forceFeedbackParams.intensityFallOffMinDistanceSqr, m_forceFeedbackParams.intensityFallOffMaxDistanceSqr);
		const float minMaxDiffSqr = m_forceFeedbackParams.intensityFallOffMaxDistanceSqr - m_forceFeedbackParams.intensityFallOffMinDistanceSqr;
		
		float effectIntensity = (float)__fsel(-minMaxDiffSqr, 0.0f, 1.0f - (testDistanceSqr - m_forceFeedbackParams.intensityFallOffMinDistanceSqr) / (minMaxDiffSqr + FLT_EPSILON));
		effectIntensity *= effectIntensity;
		if (effectIntensity > 0.01f)
		{
			IForceFeedbackSystem* pForceFeedback = CCryAction::GetCryAction()->GetIForceFeedbackSystem();
			assert(pForceFeedback);
			ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(m_forceFeedbackParams.forceFeedbackEventName.c_str());
			pForceFeedback->PlayForceFeedbackEffect(fxId, SForceFeedbackRuntimeParams(effectIntensity, 0.0f));
		}
	}

}


void CMFXForceFeedbackEffect::GetResources(SMFXResourceList& rlist)
{
	SMFXForceFeedbackListNode *listNode = SMFXForceFeedbackListNode::Create();
	listNode->m_forceFeedbackParams.forceFeedbackEventName = m_forceFeedbackParams.forceFeedbackEventName.c_str();
	listNode->m_forceFeedbackParams.intensityFallOffMinDistanceSqr = m_forceFeedbackParams.intensityFallOffMinDistanceSqr;
	listNode->m_forceFeedbackParams.intensityFallOffMaxDistanceSqr = m_forceFeedbackParams.intensityFallOffMaxDistanceSqr;

	SMFXForceFeedbackListNode* next = rlist.m_forceFeedbackList;

	if (!next)
		rlist.m_forceFeedbackList = listNode;
	else
	{ 
		while (next->pNext)
			next = next->pNext;

		next->pNext = listNode;
	}  
}