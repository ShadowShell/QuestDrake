////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MFXDecalEffect.cpp
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: Decal effect
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "MFXDecalEffect.h"

CMFXDecalEffect::CMFXDecalEffect() : m_material(0)
{
}

CMFXDecalEffect::~CMFXDecalEffect()
{
	ReleaseMaterial();
}

void CMFXDecalEffect::ReadXMLNode(XmlNodeRef &node)
{
	IMFXEffect::ReadXMLNode(node);
	
  XmlNodeRef material = node->findChild("Material");	
	if (material)
	{
		m_decalParams.material = material->getContent();

		// preloading is done during level loading itself
	}
   	
  m_decalParams.minscale = 1.f;
	m_decalParams.maxscale = 1.f;
	m_decalParams.rotation = -1.f;
	m_decalParams.growTime = 0.f;
	m_decalParams.assemble = false;
	m_decalParams.lifetime = 10.0f;
	m_decalParams.forceedge = false;
  
  XmlNodeRef scalenode = node->findChild("Scale");
	if (scalenode)
	{
		m_decalParams.minscale = (float)atof(scalenode->getContent());
    m_decalParams.maxscale = m_decalParams.minscale;
	}	  
  
	node->getAttr("minscale", m_decalParams.minscale);
	node->getAttr("maxscale", m_decalParams.maxscale);

	node->getAttr("rotation", m_decalParams.rotation);
	m_decalParams.rotation = DEG2RAD(m_decalParams.rotation);

	node->getAttr("growTime", m_decalParams.growTime);
	node->getAttr("assembledecals", m_decalParams.assemble);
	node->getAttr("forceedge", m_decalParams.forceedge);
	node->getAttr("lifetime", m_decalParams.lifetime);
}

IMFXEffectPtr CMFXDecalEffect::Clone()
{
	CMFXDecalEffect* clone = new CMFXDecalEffect();
	clone->m_decalParams.material = m_decalParams.material;
	clone->m_decalParams.minscale = m_decalParams.minscale;
	clone->m_decalParams.maxscale = m_decalParams.maxscale;
	clone->m_decalParams.rotation = m_decalParams.rotation;
	clone->m_decalParams.growTime = m_decalParams.growTime;
	clone->m_decalParams.assemble = m_decalParams.assemble;
	clone->m_decalParams.forceedge = m_decalParams.forceedge;
	clone->m_decalParams.lifetime = m_decalParams.lifetime;
	clone->m_effectParams = m_effectParams;
	return clone;
}

void CMFXDecalEffect::PreLoadAssets()
{
	IMFXEffect::PreLoadAssets();

	if (m_decalParams.material.c_str())
	{
		// store as smart pointer
		m_material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(
			m_decalParams.material.c_str(),false);
	}
}

void CMFXDecalEffect::ReleasePreLoadAssets()
{
	IMFXEffect::ReleasePreLoadAssets();

	ReleaseMaterial();
}

void CMFXDecalEffect::ReleaseMaterial()
{
	// Release material (smart pointer)
	m_material = 0;
}

void CMFXDecalEffect::Execute(SMFXRunTimeEffectParams &params)
{
  FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	if (!(params.playflags & MFX_PLAY_DECAL))
		return;

	// not on a static object or entity
	
  const float angle = (params.angle != MFX_INVALID_ANGLE) ? params.angle : Random(0.f, gf_PI2);
  
	if (!params.trgRenderNode && !params.trg)
	{
    CryEngineDecalInfo terrainDecal;

    { // 2d terrain
      const float terrainHeight( gEnv->p3DEngine->GetTerrainElevation(params.pos.x, params.pos.y) );
      const float terrainDelta( params.pos.z - terrainHeight );  

		  if (terrainDelta > 2.0f || terrainDelta < -0.5f)
			  return;

		  terrainDecal.vPos = Vec3(params.decalPos.x, params.decalPos.y, terrainHeight);
    }

		terrainDecal.vNormal = params.normal;
		terrainDecal.vHitDirection = params.dir[0].GetNormalized();
		terrainDecal.fLifeTime = m_decalParams.lifetime;
		terrainDecal.fGrowTime = m_decalParams.growTime;

		if (!m_decalParams.material.empty())
			strcpy(terrainDecal.szMaterialName, m_decalParams.material.c_str());
		else
      CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "CMFXDecalEffect::Execute: Decal material name is not specified");

		terrainDecal.fSize = Random(m_decalParams.minscale, m_decalParams.maxscale);

		if(m_decalParams.rotation>=0.f)
			terrainDecal.fAngle = m_decalParams.rotation;
		else
			terrainDecal.fAngle = angle;

		if(terrainDecal.fSize <= params.fDecalPlacementTestMaxSize)
			gEnv->p3DEngine->CreateDecal(terrainDecal);
	}
	else
	{
		CryEngineDecalInfo decal;

		IEntity *pEnt = gEnv->pEntitySystem->GetEntity(params.trg);
		IRenderNode* pRenderNode = NULL;
		if (pEnt)
		{
			IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEnt->GetProxy(ENTITY_PROXY_RENDER);
			if (pRenderProxy)
				pRenderNode = pRenderProxy->GetRenderNode();
		}
		else
		{
			pRenderNode = params.trgRenderNode;
		}

		// filter out ropes
		if (pRenderNode && pRenderNode->GetRenderNodeType() == eERType_Rope)
			return;

		decal.ownerInfo.pRenderNode = pRenderNode;

		decal.vPos = params.pos;
		decal.vNormal = params.normal;
		decal.vHitDirection = params.dir[0].GetNormalized();
		decal.fLifeTime = m_decalParams.lifetime;
		decal.fGrowTime = m_decalParams.growTime;
		decal.bAssemble = m_decalParams.assemble;
		decal.bForceEdge = m_decalParams.forceedge;

		if (!m_decalParams.material.empty())
			strcpy(decal.szMaterialName, m_decalParams.material.c_str());
		else
      CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "CMFXDecalEffect::Execute: Decal material name is not specified");

		decal.fSize = Random(m_decalParams.minscale, m_decalParams.maxscale);
		if(m_decalParams.rotation>=0.f)
			decal.fAngle = m_decalParams.rotation;
		else
			decal.fAngle = angle;

		if(decal.fSize <= params.fDecalPlacementTestMaxSize)
			gEnv->p3DEngine->CreateDecal(decal);
	}
}

void CMFXDecalEffect::GetResources(SMFXResourceList &rlist)
{
	SMFXDecalListNode *listNode = SMFXDecalListNode::Create();
	listNode->m_decalParams.material = m_decalParams.material.c_str();
	listNode->m_decalParams.minscale = m_decalParams.minscale;
	listNode->m_decalParams.maxscale = m_decalParams.maxscale;
	listNode->m_decalParams.rotation = m_decalParams.rotation;
	listNode->m_decalParams.assemble = m_decalParams.assemble;
	listNode->m_decalParams.forceedge = m_decalParams.forceedge;
	listNode->m_decalParams.lifetime = m_decalParams.lifetime;

  SMFXDecalListNode* next = rlist.m_decalList;

  if (!next)
    rlist.m_decalList = listNode;
  else
  { 
    while (next->pNext)
      next = next->pNext;

    next->pNext = listNode;
  }  
}