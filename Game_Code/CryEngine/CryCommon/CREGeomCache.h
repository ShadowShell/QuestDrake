////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// ------------------------------------------------------------------------
//  File name:   CREGeomCache.h
//  Created:     17/10/2012 by Axel Gneiting
//  Description: Backend part of geometry cache rendering
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CREGEOMCACHE_
#define _CREGEOMCACHE_

#pragma once

#if defined(USE_GEOM_CACHES)

class CREGeomCache : public CRendElementBase
{
public:
	struct SMeshInstance
	{
		AABB m_aabb;
		Matrix34 m_matrix;
		Matrix34 m_prevMatrix;
	};

	struct SMeshRenderData
	{
		DynArray<SMeshInstance> m_instances;
		_smart_ptr<IRenderMesh> m_pRenderMesh;
	};

public:
	CREGeomCache();
	~CREGeomCache();

	bool Update(const int flags, const bool bTesselation);
	static void UpdateModified();

	// CRendElementBase interface
	virtual bool mfUpdate(EVertexFormat eVertFormat, int Flags, bool bTessellation);
	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	// CREGeomCache interface
	virtual void InitializeRenderElement(const uint numMeshes, _smart_ptr<IRenderMesh> *pMeshes, uint16 materialId);
	virtual void SetupMotionBlur(CRenderObject *pRenderObject, const SRenderingPassInfo &passInfo);
	
	virtual volatile int *SetAsyncUpdateState(int &threadId);
	virtual DynArray<SMeshRenderData> *GetMeshFillDataPtr();
	virtual DynArray<SMeshRenderData> *GetRenderDataPtr();
	virtual void DisplayFilledBuffer(const int threadId);

private:	
	uint16 m_materialId;	
	volatile bool m_bUpdateFrame[2];
	volatile int m_transformUpdateState[2];
	DynArray<SMeshRenderData> m_meshFillData[2];		
	DynArray<SMeshRenderData> m_meshRenderData;

	static CryCriticalSection ms_updateListCS[2];
	static std::vector<CREGeomCache*> ms_updateList[2];
};

#endif
#endif
