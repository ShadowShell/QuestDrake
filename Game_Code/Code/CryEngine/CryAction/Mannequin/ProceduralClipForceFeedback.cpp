////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>
#include "IForceFeedbackSystem.h"

struct SForceFeedbackClipParams : public SProceduralParams
{
	float scale;
	float delay;
};

class CProceduralClipForceFeedback : public TProceduralClip<SForceFeedbackClipParams>
{
public:

	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipForceFeedback, "ForceFeedback", 0x08B53675736946B7, 0x97D24942984C285F)

	virtual void OnEnter(float blendTime, float duration, const SForceFeedbackClipParams &params)
	{
		if ( gEnv->IsEditor() && gEnv->pGame->GetIGameFramework()->GetMannequinInterface().IsSilentPlaybackMode() )
			return;

		IForceFeedbackSystem* pForceFeedback = CCryAction::GetCryAction()->GetIForceFeedbackSystem();
		CRY_ASSERT(pForceFeedback);
		ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(params.dataString.c_str());

		if (fxId != InvalidForceFeedbackFxId)
		{
			float actionScale = 0.f;
			float ffScale = params.scale;
			
			if(GetParam("ffScale", actionScale))
			{
				ffScale *= actionScale;
			}
			
			SForceFeedbackRuntimeParams runtimeParams(ffScale, params.delay);
			pForceFeedback->PlayForceFeedbackEffect(fxId, runtimeParams);
		}
	}

virtual void OnExit(float blendTime) {}

virtual void Update(float timePassed) {}
};

CProceduralClipForceFeedback::CProceduralClipForceFeedback()
{
}

CProceduralClipForceFeedback::~CProceduralClipForceFeedback()
{
}

CRYREGISTER_CLASS(CProceduralClipForceFeedback)
