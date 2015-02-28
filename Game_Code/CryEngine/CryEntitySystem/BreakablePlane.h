/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2005.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 26:5:2005   : Created by Anton Knyazyev
*************************************************************************/

#ifndef __BreakablePlane_H__
#define __BreakablePlane_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IIndexedMesh.h"
#include "IPhysics.h"
#include "IDeferredCollisionEvent.h"

struct SProcessImpactIn;
struct SProcessImpactOut;
struct SExtractMeshIslandIn;
struct SExtractMeshIslandOut;

class CBreakablePlane : public IOwnedObject
{
public:
	CBreakablePlane() 
	{ 
		m_pGrid=0; m_pMat=0;
		m_nCells.set(20,20);
		m_cellSize = 1;
		m_maxPatchTris = 20;
		m_jointhresh = 0.3f;
		m_density = 900;
		m_bStatic = 1;
		m_pGeom = 0; m_pSampleRay = 0;
		*m_mtlSubstName = 0;
	}
	~CBreakablePlane()
	{
		if (m_pGrid) m_pGrid->Release();
		if (m_pSampleRay) m_pSampleRay->Release();
		if (m_pGeom) m_pGeom->Release();
	}
	int Release() {	delete this; return 0;	}

	bool SetGeometry(IStatObj *pStatObj, IMaterial *pRenderMat, int bStatic, int seed);
	void FillVertexData(CMesh *pMesh,int ivtx, const vector2df &pos, int iside);
	IStatObj *CreateFlatStatObj(int *&pIdx, vector2df *pt, vector2df *bounds, const Matrix34 &mtxWorld, IParticleEffect *pEffect=0, 
		bool bNoPhys=false, bool bUseEdgeAlpha=false);
	int *Break(const Vec3 &pthit, float r, vector2df *&ptout, int seed, float filterAng,float ry);
	static int ProcessImpact(const SProcessImpactIn& in, SProcessImpactOut& out);
	static void ExtractMeshIsland(const SExtractMeshIslandIn& in, SExtractMeshIslandOut& out);

	float m_cellSize;
	vector2di m_nCells;
	float m_nudge;
	int m_maxPatchTris;
	float m_jointhresh;
	float m_density;
	IBreakableGrid2d *m_pGrid;
	Matrix33 m_R;
	Vec3 m_center;
	int m_bAxisAligned;
	int m_matId;
	IMaterial *m_pMat;
	int m_matSubindex;
	char m_mtlSubstName[64];
	int m_matFlags;
	float m_z[2];
	float m_thicknessOrg;
	float m_refArea[2];
	vector2df m_ptRef[2][3];
	SMeshTexCoord m_texRef[2][3];
	SMeshTangents	m_TangentRef[2][3];
	int m_bStatic;
	int m_bOneSided;
	IGeometry *m_pGeom,*m_pSampleRay;
	static int g_nPieces;
	static float g_maxPieceLifetime;
};

#endif //__BreakablePlane_H__
