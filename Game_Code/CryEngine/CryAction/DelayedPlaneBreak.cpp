// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "CryAction.h"
#include "IGameRulesSystem.h"
#include "ActionGame.h"
#include "DelayedPlaneBreak.h"
#include "ParticleParams.h"
#include "IBreakableManager.h"

void CDelayedPlaneBreak::OnUpdate()
{
	m_islandIn.bCreateIsle = (m_islandIn.processFlags & ePlaneBreak_AutoSmash) == 0;
	if (!m_bMeshPrepOnly)
		gEnv->pEntitySystem->GetBreakableManager()->ExtractPlaneMeshIsland(m_islandIn, m_islandOut);
	else 
	{
		m_islandOut.pStatObj = m_islandIn.pStatObj;
		m_islandOut.pStatObj->GetIndexedMesh(true);
	}
	if (m_threadTaskInfo.m_pThread)
		gEnv->pSystem->GetIThreadTaskManager()->UnregisterTask(this);

	CDelayedPlaneBreak *pdpb = this - m_idx;
	if (!(m_islandIn.pStatObj->GetFlags() & STATIC_OBJECT_CLONE) &&
			 (m_islandOut.pStatObj->GetFlags() & STATIC_OBJECT_CLONE))
		for(int i=0; i < m_count; ++i) 
			if (pdpb[i].m_status != NONE &&
					i !=m_idx &&
					pdpb[i].m_islandOut.pStatObj == m_islandIn.pStatObj &&
					pdpb[i].m_epc.pEntity[1] == m_epc.pEntity[1] &&
					pdpb[i].m_epc.partid[1] == m_epc.partid[1])
			{
				IStatObj* oldSrc = pdpb[i].m_islandIn.pStatObj;
				pdpb[i].m_islandIn.pStatObj = m_islandOut.pStatObj;
				pdpb[i].m_islandOut.pStatObj = m_islandOut.pStatObj;
				m_islandOut.pStatObj->AddRef();
				oldSrc->Release();
			}
	m_status = DONE;
}
