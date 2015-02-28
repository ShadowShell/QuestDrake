////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>

class CProceduralClipEvent : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipEvent, "ActionEvent", 0x7ACD41CACFD4477C, 0x83F2FC8EA4C118E9)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		SendActionEvent(params.dataCRC.crc);
	}

	virtual void OnExit(float blendTime) {}

	virtual void Update(float timePassed) {}

};

CProceduralClipEvent::CProceduralClipEvent()
{
}

CProceduralClipEvent::~CProceduralClipEvent()
{
}

CRYREGISTER_CLASS(CProceduralClipEvent)
