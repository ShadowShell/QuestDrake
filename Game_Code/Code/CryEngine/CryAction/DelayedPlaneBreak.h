// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#pragma once

#include "IIndexedMesh.h"
#include "IPhysics.h"
#include "IDeferredCollisionEvent.h"
#include "IEntitySystem.h"
#include "IBreakableManager.h"

class CDelayedPlaneBreak : public IDeferredPhysicsEvent
{
public:
	enum EStatus
	{
		NONE,
		STARTED,
		DONE,
	};

	CDelayedPlaneBreak() { m_status=NONE; m_threadTaskInfo.m_pThread=0; m_count=0; m_bMeshPrepOnly=false; }
	CDelayedPlaneBreak(const CDelayedPlaneBreak &src) 
	: m_status(src.m_status)
	, m_islandIn(src.m_islandIn)
	, m_islandOut(src.m_islandOut)
	, m_iBreakEvent(src.m_iBreakEvent)
	, m_idx(src.m_idx)
	, m_count(src.m_count)
	, m_bMeshPrepOnly(src.m_bMeshPrepOnly)
	, m_epc(src.m_epc)
	{
	}
	CDelayedPlaneBreak& operator=(const CDelayedPlaneBreak &src)
	{
		m_status = src.m_status;
		m_islandIn = src.m_islandIn;
		m_islandOut = src.m_islandOut;
		m_iBreakEvent = src.m_iBreakEvent;
		m_idx = src.m_idx;
		m_count = src.m_count;
		m_bMeshPrepOnly = src.m_bMeshPrepOnly;
		m_epc = src.m_epc;
		return *this;
	}

	virtual void Start() {}
	virtual int Result(EventPhys*) { return 0; }
	virtual void Sync() {}
	virtual bool HasFinished() { return true; }
	virtual DeferredEventType GetType() const { return PhysCallBack_OnCollision; }
	virtual EventPhys* PhysicsEvent() { return &m_epc; }

	virtual void OnUpdate();
	virtual void Stop() {}
	virtual SThreadTaskInfo* GetTaskInfo() { return &m_threadTaskInfo; }

	volatile int m_status;
	SExtractMeshIslandIn m_islandIn;
	SExtractMeshIslandOut m_islandOut;
	int m_iBreakEvent;
	int m_idx;
	int m_count;

	bool m_bMeshPrepOnly;

	EventPhysCollision m_epc;
	SThreadTaskInfo m_threadTaskInfo;	
};
