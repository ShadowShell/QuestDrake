////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MaterialEffects.cpp
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include <limits.h>
#include <CryPath.h>
#include "MaterialEffectsCVars.h"
#include "MaterialEffects.h"
#include "MFXEffect.h"
#include "MFXRandomEffect.h"
#include "MFXSoundEffect.h"
#include "MFXParticleEffect.h"
#include "MFXDecalEffect.h"
#include "MFXRandomEffect.h"
#include "MaterialFGManager.h"
#include "PoolAllocator.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_VISUAL_MFX_SYSTEM

void SDebugVisualMFXSystem::AddEffectDebugVisual(const TMFXEffectId effectId, const SMFXRunTimeEffectParams& runtimeParams)
{
	//Only add, if hint search matches (this allows to filter effects invoked by name from game, and get all info we need to display)
	if (effectId == m_lastSearchHint.fxId)
	{
		if (m_nextHit >= MAX_DEBUG_VISUAL_MFX_ENTRIES)
		{
			m_nextHit = 0;
		}

		const char* debugFilter = CMaterialEffectsCVars::Get().mfx_DebugVisualFilter->GetString();
		assert(debugFilter);
		bool ignoreFilter = (strlen(debugFilter) == 0) || (strcmp(debugFilter, "0") == 0);
		bool addToDebugList = ignoreFilter || (stricmp(debugFilter, m_lastSearchHint.materialName1.c_str()) == 0);

		if (addToDebugList)
		{
			m_effectList[m_nextHit].fxPosition = runtimeParams.pos;
			m_effectList[m_nextHit].fxDirection = (runtimeParams.normal.IsZero() == false) ? runtimeParams.normal : Vec3(0.0f, 0.0f, 1.0f);
			m_effectList[m_nextHit].lifeTime = DEFAULT_DEBUG_VISUAL_MFX_LIFETIME;
			m_effectList[m_nextHit].fxId = effectId;
			m_effectList[m_nextHit].materialName1 = m_lastSearchHint.materialName1.c_str();
			m_effectList[m_nextHit].materialName2 = m_lastSearchHint.materialName2.c_str();

			m_nextHit++;
		}
	}
}

void SDebugVisualMFXSystem::AddLastSearchHint(const TMFXEffectId effectId, const char *customName, int surfaceIndex2)
{
	m_lastSearchHint.Reset();

	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	assert(pSurfaceTypeManager);

	m_lastSearchHint.materialName1 = customName;
	m_lastSearchHint.materialName2 = pSurfaceTypeManager->GetSurfaceType(surfaceIndex2)->GetName();
	m_lastSearchHint.fxId = effectId;
}


void SDebugVisualMFXSystem::AddLastSearchHint(const TMFXEffectId effectId, IEntityClass* pEntityClass, int surfaceIndex2)
{
	m_lastSearchHint.Reset();

	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	assert(pSurfaceTypeManager);
	assert(pEntityClass);

	m_lastSearchHint.materialName1 = pEntityClass->GetName();
	m_lastSearchHint.materialName2 = pSurfaceTypeManager->GetSurfaceType(surfaceIndex2)->GetName();
	m_lastSearchHint.fxId = effectId;
}

void SDebugVisualMFXSystem::AddLastSearchHint(const TMFXEffectId effectId, int surfaceIndex1, int surfaceIndex2)
{
	m_lastSearchHint.Reset();

	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	assert(pSurfaceTypeManager);

	m_lastSearchHint.materialName1 = pSurfaceTypeManager->GetSurfaceType(surfaceIndex1)->GetName();
	m_lastSearchHint.materialName2 = pSurfaceTypeManager->GetSurfaceType(surfaceIndex2)->GetName();
	m_lastSearchHint.fxId = effectId;
}


void SDebugVisualMFXSystem::Update(float frameTime)
{
	IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

	SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
	SAuxGeomRenderFlags newFlags = e_Def3DPublicRenderflags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetDepthTestFlag(e_DepthTestOff);
	newFlags.SetCullMode(e_CullModeNone); 
	pRenderAux->SetRenderFlags(newFlags);

	const float baseDebugTimeOut = DEFAULT_DEBUG_VISUAL_MFX_LIFETIME;

	bool extendedDebugInfo = (CMaterialEffectsCVars::Get().mfx_DebugVisual == 2);

	for (int i = 0; i < MAX_DEBUG_VISUAL_MFX_ENTRIES; ++i)
	{
		SDebugVisualEntry& currentFX = m_effectList[i];

		if (currentFX.lifeTime <= 0.0f)
		{
			continue;
		}

		currentFX.lifeTime -= frameTime;

		IMFXEffectPtr pEffect = m_pMaterialFX->InternalGetEffect(currentFX.fxId);
		if (pEffect)
		{
			const float alpha = clamp_tpl(powf(((currentFX.lifeTime + 2.0f) / baseDebugTimeOut), 3.0f), 0.0f, 1.0f);
			const ColorB blue(0, 0, 255, (uint8)(192 * alpha));
			const Vec3 coneBase = currentFX.fxPosition + (currentFX.fxDirection * 0.4f);
			const Vec3 lineEnd = currentFX.fxPosition;
			pRenderAux->DrawCone(coneBase, currentFX.fxDirection, 0.12f, 0.2f, blue);
			pRenderAux->DrawLine(coneBase, blue, lineEnd, blue, 3.0f);

			const Vec3 baseText = coneBase + (0.2f * currentFX.fxDirection);
			const Vec3 textLineOffset(0.0f, 0.0f, 0.14f);
			const float textColorOk[4] = {1.0f, 1.0f, 1.0f, alpha};
			const float textColorError[4] = {1.0f, 0.0f, 0.0f, alpha};
			const float titleColor[4] = {1.0f, 1.0f, 0.0f, alpha};

			bool matDefaultDetected = ((stricmp(currentFX.materialName1.c_str(), "mat_default") == 0) ||
																	(stricmp(currentFX.materialName2.c_str(), "mat_default") == 0));

			const float *textColor = matDefaultDetected ? textColorError : textColorOk;

			if (matDefaultDetected)
				gEnv->pRenderer->DrawLabelEx(baseText, 1.75f, textColor, true, false, "FIX ME (mat_default)!");

			gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 2.0f), 1.25f, textColor, true, false, "%s / %s", currentFX.materialName1.c_str(), currentFX.materialName2.c_str());
			gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * 3.0f), 1.25f, textColor, true, false, "Lib: %s, FX: %s", pEffect->m_effectParams.libName.c_str(), pEffect->m_effectParams.name.c_str());
			
			if (extendedDebugInfo)
			{
				float textOffsetCount = 5.0f;
				SMFXResourceListPtr pFxResources = m_pMaterialFX->GetResources(currentFX.fxId);
				
				//Particles
				gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.35f, titleColor, true, false, "** Particles **");
				SMFXParticleListNode *pParticleNext = pFxResources->m_particleList;
				while (pParticleNext)
				{
					textOffsetCount += 1.0f;
					gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.25f, textColor, true, false, "  %s", pParticleNext->m_particleParams.name);
					pParticleNext = pParticleNext->pNext;
				}

				//Sound
				textOffsetCount += 1.0f;
				gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.35f, titleColor, true, false, "** Sound **");
				SMFXSoundListNode *pSoundNext = pFxResources->m_soundList;
				while (pSoundNext)
				{
					textOffsetCount += 1.0f;
					gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.25f, textColor, true, false, "  %s", pSoundNext->m_soundParams.name);
					pSoundNext = pSoundNext->pNext;
				}

				//Decals
				textOffsetCount += 1.0f;
				gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.35f, titleColor, true, false, "** Decals **");
				SMFXDecalListNode *pDecalNext = pFxResources->m_decalList;
				while (pDecalNext)
				{
					textOffsetCount += 1.0f;
					gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.25f, textColor, true, false, "  Mat: %s / Tex: %s", pDecalNext->m_decalParams.material, pDecalNext->m_decalParams.filename);
					pDecalNext = pDecalNext->pNext;
				}

				//Flow graphs
				textOffsetCount += 1.0f;
				gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.35f, titleColor, true, false, "** Flow graphs **");
				SMFXFlowGraphListNode *pFGNext = pFxResources->m_flowGraphList;
				while (pFGNext)
				{
					textOffsetCount += 1.0f;
					gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.25f, textColor, true, false, "  %s", pFGNext->m_flowGraphParams.name);
					pFGNext = pFGNext->pNext;
				}

				//Force feedback
				textOffsetCount += 1.0f;
				gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.35f, titleColor, true, false, "** Force feedback **");
				SMFXForceFeedbackListNode *pFFNext = pFxResources->m_forceFeedbackList;
				while (pFFNext)
				{
					textOffsetCount += 1.0f;
					gEnv->pRenderer->DrawLabelEx(baseText - (textLineOffset * textOffsetCount), 1.25f, textColor, true, false, "  %s", pFFNext->m_forceFeedbackParams.forceFeedbackEventName);
					pFFNext = pFFNext->pNext;
				}


			}
		}
	}

	pRenderAux->SetRenderFlags(oldFlags);
}

#endif //DEBUG_VISUAL_MFX_SYSTEM
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define IMPL_POOL_RETURNING(type, rtype) \
	static stl::PoolAllocatorNoMT<sizeof(type)> m_pool_##type; \
	rtype type::Create() \
	{ \
		return new (m_pool_##type.Allocate()) type(); \
	} \
	void type::Destroy() \
	{ \
		this->~type(); \
		m_pool_##type.Deallocate(this); \
	} \
	void type::FreePool() \
	{ \
		m_pool_##type.FreeMemory(); \
	}
#define IMPL_POOL(type) IMPL_POOL_RETURNING(type, type*)

IMPL_POOL_RETURNING(SMFXResourceList, SMFXResourceListPtr);
IMPL_POOL(SMFXFlowGraphListNode);
IMPL_POOL(SMFXDecalListNode);
IMPL_POOL(SMFXParticleListNode);
IMPL_POOL(SMFXSoundListNode);
IMPL_POOL(SMFXForceFeedbackListNode);


CMaterialEffects::CMaterialEffects()
: m_bDataInitialized(false)
{
	m_pSystem = gEnv->pSystem;
	m_bUpdateMode = false;
	m_defaultSurfaceId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_default")->GetId();
	m_canopySurfaceId = m_defaultSurfaceId;
	m_pMaterialFGManager = new CMaterialFGManager();

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	m_debugVisual.InitWith(this);
#endif
}

CMaterialEffects::~CMaterialEffects()
{
	// smart pointers will automatically release.
	// the clears will immediately delete all effects while CMaterialEffects is still exisiting
	m_mfxLibraries.clear();
	m_delayedEffects.clear();
	m_effectVec.clear();
	SAFE_DELETE(m_pMaterialFGManager);
}

void CMaterialEffects::LoadFXLibraries()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "MaterialEffects");

	m_mfxLibraries.clear();
	m_effectVec.clear();
	m_effectVec.push_back(0); // 0 -> invalid effect id

	ICryPak *pak = gEnv->pCryPak;
	_finddata_t fd;

	intptr_t handle = pak->FindFirst("Libs/MaterialEffects/FXLibs/*.xml", &fd);
	int res = 0;
	if (handle != -1)
	{
		do 
		{
			LoadFXLibrary(fd.name);
			res = pak->FindNext(handle, &fd);
			SLICE_AND_SLEEP();
		} while(res >= 0);
		pak->FindClose(handle);
	}
	m_canopySurfaceId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_canopy")->GetId();
}

void CMaterialEffects::LoadFXLibrary(const char *name)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "FX Library XML (%s)", name);

	string path = PathUtil::Make("Libs/MaterialEffects/FXLibs", name);
	string fileName = name;
	
	int period = fileName.find(".");
	string libName = fileName.substr(0, period);

	XmlNodeRef fxlib = m_pSystem->LoadXmlFromFile(path);
	if (fxlib != 0)
  {
		// we can store plain ptrs here, because both CMFXEffect* and const char* are still valid
		typedef std::pair<CMFXEffect*, const char*> ChildEffectParentNamePair;
		std::vector<ChildEffectParentNamePair> delayedChildEffects;
		delayedChildEffects.reserve(fxlib->getChildCount());
		
		// create library
		TFXLibrariesMap::iterator libIter = m_mfxLibraries.find(libName);
		if (libIter == m_mfxLibraries.end())
		{
			std::pair<TFXLibrariesMap::iterator, bool> iterPair = 
				m_mfxLibraries.insert(TFXLibrariesMap::value_type(libName, TFXLibrary()));
			assert (iterPair.second == true);
			libIter = iterPair.first;
			assert (libIter != m_mfxLibraries.end());
		}
		const TMFXNameId& libNameId = libIter->first; // uses CryString's ref-count feature
		TFXLibrary& library = libIter->second;

    for (int i = 0; i < fxlib->getChildCount(); ++i)
    {
			XmlNodeRef curEffectNode = fxlib->getChild(i);
			if (!curEffectNode)
				continue;

			// create base effect
			CMFXEffect *pEffect = new CMFXEffect();

			// build from XML description
      pEffect->Build(curEffectNode);

			// assign library name to effect (mosty for debugging)
			// uses CryString's ref-count feature
			pEffect->m_effectParams.libName = libNameId;

			// get effect name
			const TMFXNameId& effectName = pEffect->m_effectParams.name;

			// check if effect is derived
      const char *parentName = curEffectNode->getAttr("parent");
      if (parentName && *parentName)
      {
				IMFXEffectPtr pParentEffect = stl::find_in_map(library, parentName, 0);
				// does the parent effect already exist?
        if (pParentEffect)
				{
          pEffect->MakeDerivative(pParentEffect);
				}
				else // does not exist, so mark up for later
				{
					// CryCryComment("[MFX] Parent effect '%s:%s' for Effect '%s:%s' not yet defined.", libName.c_str(), parentName, libName.c_str(), effectName.c_str());
					delayedChildEffects.push_back(ChildEffectParentNamePair(pEffect, parentName));
				}
      }
			TFXLibrary::iterator found = library.find(effectName);
			if (found == library.end())
			{
				pEffect->m_effectParams.effectId = m_effectVec.size();	
				library.insert (TFXLibrary::value_type(effectName, pEffect));
				m_effectVec.push_back(pEffect);
			}
			else
			{
				GameWarning("[MFX] Effect '%s:%s' already present. Overriding.", libName.c_str(), effectName.c_str());
				pEffect->m_effectParams.effectId = found->second->m_effectParams.effectId;
	      found->second = pEffect;
				assert (pEffect->m_effectParams.effectId < m_effectVec.size());
				m_effectVec[pEffect->m_effectParams.effectId] = pEffect;
			}
    }
		
		// now derive delayed effects from their parents
		std::vector<ChildEffectParentNamePair>::iterator iter = delayedChildEffects.begin();
		std::vector<ChildEffectParentNamePair>::iterator iterEnd = delayedChildEffects.end();
		while (iter != iterEnd)
		{
			CMFXEffect* pEffect = iter->first;
			const char* parentName = iter->second;
			IMFXEffectPtr pParentEffect = stl::find_in_map(library, parentName, 0);
			if (pParentEffect)
				pEffect->MakeDerivative(pParentEffect);
			else
				GameWarning("[MFX] Parent effect '%s:%s' for Effect '%s:%s' does not exist!", libName.c_str(), parentName, libName.c_str(), pEffect->m_effectParams.name.c_str());
			++iter;
		}
  }
  else
  {
    CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "[MatFX]: Failed to load library %s", path.c_str());
  }
 
}

bool CMaterialEffects::ExecuteEffect(TMFXEffectId effectId, SMFXRunTimeEffectParams& params)
{
  FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);
	
  if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return false;

	bool success = false;
	IMFXEffectPtr pEffect = InternalGetEffect(effectId);
	if (pEffect)
	{
		const float delay = pEffect->m_effectParams.delay;
		if (delay > 0.0f && !(params.playflags & MFX_DISABLE_DELAY))
			TimedEffect(pEffect, params);
		else
			pEffect->Execute(params);
		success = true;

#ifdef DEBUG_VISUAL_MFX_SYSTEM
		if (CMaterialEffectsCVars::Get().mfx_DebugVisual)
		{
			if (effectId != InvalidEffectId)
			{
				m_debugVisual.AddEffectDebugVisual(effectId, params);
			}
		}
#endif

	}

	return success;
}

void CMaterialEffects::StopEffect(TMFXEffectId effectId)
{
	IMFXEffectPtr pEffect = InternalGetEffect(effectId);
	if (pEffect)
	{
		SMFXResourceListPtr resources = SMFXResourceList::Create();
		pEffect->GetResources(*resources);

		SMFXFlowGraphListNode *pNext=resources->m_flowGraphList;
		while (pNext)
		{
			GetFGManager()->EndFGEffect(pNext->m_flowGraphParams.name);
			pNext=pNext->pNext;
		}
	}
}

void CMaterialEffects::SetCustomParameter(TMFXEffectId effectId, const char* customParameter, const SMFXCustomParamValue& customParameterValue)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return;

	IMFXEffectPtr pEffect = InternalGetEffect(effectId);
	if (pEffect)
	{
		pEffect->SetCustomParameter(customParameter, customParameterValue);	
	}
}

namespace
{
struct CConstCharArray
{
	const char* ptr;
	size_t count;
	CConstCharArray()
		: ptr("")
		, count(0)
	{
	}
};

struct less_CConstCharArray
{
	bool operator()(const CConstCharArray& s0, const CConstCharArray& s1) const
	{
		const size_t minCount = (s0.count < s1.count) ? s0.count : s1.count;
		const int result = memicmp(s0.ptr, s1.ptr, minCount);
		return result ? (result < 0) : (s0.count < s1.count);
	}
};

void ToEffectString(const string& effectString, string& libName, string& effectName)
{
	size_t colon = effectString.find(':');
	if (colon != string::npos)
	{
		libName    = effectString.substr(0, colon);
		effectName = effectString.substr(colon + 1, effectString.length() - colon - 1);
	}
	else
	{
		libName = effectString;
		effectName = effectString;
	}
}
}

bool CMaterialEffects::Load(const char* filenameSpreadsheet)
{
	m_bDataInitialized = true;

	// reloading with ""
	if (filenameSpreadsheet && *filenameSpreadsheet == 0 && m_filename.empty() == false)
		filenameSpreadsheet = m_filename.c_str();

	Reset(false);

	// Create a valid path to the XML file.
	string sPath = "Libs/MaterialEffects/";
	sPath+=filenameSpreadsheet;

	// Init messages
	CryComment("[MFX] Init");

	IXmlTableReader* const pXmlTableReader = m_pSystem->GetXmlUtils()->CreateXmlTableReader();
	if (!pXmlTableReader)
	{
		GameWarning("[MFX] XML system failure");
		return false;
	}

	// The root node. 
	XmlNodeRef root = m_pSystem->LoadXmlFromFile( sPath );
	if (!root)
	{
		// The file wasn't found, or the wrong file format was used
		GameWarning("[MFX] File not found or wrong file type: %s", sPath.c_str());
		pXmlTableReader->Release();
		return false;
	}

	CryComment("[MFX] Loaded: %s", sPath.c_str());

	m_filename = filenameSpreadsheet;

	if (!pXmlTableReader->Begin(root))
	{
		GameWarning("[MFX] Table not found");
		pXmlTableReader->Release();
		return false;
	}

	// temporary multimap: we store effectstring -> [TIndexPairs]+ there
	typedef std::vector<TIndexPair> TIndexPairVec;
	typedef std::map<CConstCharArray, TIndexPairVec, less_CConstCharArray> TEffectStringToIndexPairVecMap;
	TEffectStringToIndexPairVecMap tmpEffectStringToIndexPairVecMap;

	int rowCount = 0;
	int warningCount = 0;

	CConstCharArray cell;
	string cellString; // temporary string

	// When we've gone down more rows than we have columns, we've entered special object space
	int maxCol = 0;
	std::vector<CConstCharArray> colOrder;
	std::vector<CConstCharArray> rowOrder;

	m_surfaceIdToMatrixEntry.resize(0);
	m_surfaceIdToMatrixEntry.resize(MAX_SURFACE_COUNT, TIndex(0));
	m_ptrToMatrixEntryMap.clear();
	m_customConvert.clear();

	// Iterate through the table's rows
	for (;;)
	{
		int nRowIndex = -1;
		if (!pXmlTableReader->ReadRow(nRowIndex))
			break;

		// Iterate through the row's columns
		for (;;)
		{
			int colIndex = -1;
			if (!pXmlTableReader->ReadCell(colIndex, cell.ptr, cell.count))
				break;

			if (cell.count <= 0)
				continue;

			cellString.assign(cell.ptr, cell.count);

			if (rowCount == 0 && colIndex > 0)
			{
				const int matId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName(cellString.c_str(), "MFX", true)->GetId();
				if (matId != 0 || /* matId == 0 && */ stricmp(cellString.c_str(), "mat_default") == 0) // if matId != 0 or it's the mat_default name
				{
					// CryLogAlways("[MFX] Material found: %s [ID=%d] [mapping to row/col=%d]", cellString.c_str(), matId, colCount);
					if (m_surfaceIdToMatrixEntry.size() < matId)
					{
						m_surfaceIdToMatrixEntry.resize(matId+1);
						if (matId >= MAX_SURFACE_COUNT)
						{
							assert (false && "MaterialEffects.cpp: SurfaceTypes exceeds 256. Reconsider implementation.");
							CryLogAlways("MaterialEffects.cpp: SurfaceTypes exceeds %d. Reconsider implementation.", MAX_SURFACE_COUNT);
						}
					}
					m_surfaceIdToMatrixEntry[matId] = colIndex;
				}
				else
				{
					GameWarning("MFX WARNING: Material not found: %s", cellString.c_str());
					++warningCount;
				}
				colOrder.push_back(cell);
			} 
			else if (rowCount > 0 && colIndex > 0)
			{
				//CryLog("[MFX] Event found: %s", cellString.c_str());
				tmpEffectStringToIndexPairVecMap[cell].push_back(TIndexPair(rowCount, colIndex));
			} 
			else if (rowCount > maxCol && colIndex == 0)
			{	
				IEntityClass *pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(cellString.c_str());
				//CryLog("[MFX] Object class ID: %d", (int)pEntityClass);
				if (pEntityClass)
				{
					// CryComment("[MFX] Found EntityClass based entry: %s [mapping to row/col=%d]", cellString.c_str(), rowCount);
					m_ptrToMatrixEntryMap[pEntityClass] = rowCount;
				}
				else
				{
					// CryComment("[MFX] Found Custom entry: %s [mapping to row/col=%d]", cellString.c_str(), rowCount);
					cellString.MakeLower();
					m_customConvert[cellString] = rowCount;
					++warningCount;
				}
			}
			else if (rowCount > 0 && colIndex == 0)
			{
				rowOrder.push_back(cell);
			}
			// Heavy-duty debug info
			//CryLog("[MFX] celldata = %s at (%d, %d) rowCount=%d colIndex=%d maxCol=%d", curCellData->getContent(), i, j, rowCount, colIndex, maxCol);

			// Check if this is the furthest column we've seen thus far
			if (colIndex > maxCol)
				maxCol = colIndex;

			SLICE_AND_SLEEP();
		}
		// Increment row counter
		++rowCount;
	}

	// now postprocess the tmpEffectStringIndexPairVecMap and generate the m_matmatArray
	{
		// create the matmat array.
		// +1, because index pairs are in range [1..rowCount][1..maxCol]
		m_matmatArray.Create(rowCount+1,maxCol+1);
		TEffectStringToIndexPairVecMap::const_iterator iter = tmpEffectStringToIndexPairVecMap.begin();
		TEffectStringToIndexPairVecMap::const_iterator iterEnd = tmpEffectStringToIndexPairVecMap.end();
		string libName;
		string effectName;
		string tmpString;
		while (iter != iterEnd)
		{
			// lookup effect
			tmpString.assign(iter->first.ptr, iter->first.count);
			ToEffectString(tmpString, libName, effectName);
			IMFXEffectPtr pEffect = InternalGetEffect(libName, effectName);
			TMFXEffectId effectId = pEffect ? pEffect->m_effectParams.effectId : InvalidEffectId;
			TIndexPairVec::const_iterator vecIter = iter->second.begin();
			TIndexPairVec::const_iterator vecIterEnd = iter->second.end();
			while (vecIter != vecIterEnd)
			{
				const TIndexPair& indexPair = *vecIter;
				// CryLogAlways("[%d,%d]->%d '%s'", indexPair.first, indexPair.second, effectId, tmpString.c_str());
				m_matmatArray(indexPair.first,indexPair.second) = effectId;
				++vecIter;
			}
			++iter;
		}
	}

	if (CMaterialEffectsCVars::Get().mfx_Debug > 0)
	{
		CryLogAlways("[MFX] RowCount=%d MaxCol=%d (*=%d)", rowCount, maxCol,rowCount*maxCol);
		for (int y=0; y<m_matmatArray.m_nRows; ++y)
		{
			for (int x=0; x<m_matmatArray.m_nCols; ++x)
			{
				TMFXEffectId idRowCol = m_matmatArray.GetValue(y,x, USHRT_MAX);
				assert (idRowCol != USHRT_MAX);
				IMFXEffectPtr pEffectRowCol = InternalGetEffect(idRowCol);
				if (pEffectRowCol)
				{
					CryLogAlways("[%d,%d] -> %d '%s:%s'", y, x, idRowCol, pEffectRowCol->m_effectParams.libName.c_str(), pEffectRowCol->m_effectParams.name.c_str());
					if (y<m_matmatArray.m_nCols)
					{
						TMFXEffectId idColRow = m_matmatArray.GetValue(x,y, USHRT_MAX);
						assert (idColRow != USHRT_MAX);
						IMFXEffectPtr pEffectColRow = InternalGetEffect(idColRow);
						if (idRowCol != idColRow)
						{
							if (pEffectColRow)
							{
								GameWarning("[MFX] Identity mismatch: ExcelRowCol %d:%d: %s:%s != %s:%s", y+1, x+1, 
									pEffectRowCol->m_effectParams.libName.c_str(), pEffectRowCol->m_effectParams.name.c_str(),
									pEffectColRow->m_effectParams.libName.c_str(), pEffectColRow->m_effectParams.name.c_str());
							}
							else
							{
								GameWarning("[MFX] Identity mismatch: ExcelRowCol %d:%d: %s:%s != [not found]", y+1, x+1, 
									pEffectRowCol->m_effectParams.libName.c_str(), pEffectRowCol->m_effectParams.name.c_str());
							}
						}
					}
				}
			}
		}
	}

	// check that we have the same number of columns and rows
	if (colOrder.size() > rowOrder.size())
	{
		GameWarning("[MFX] Found %d Columns, but not enough rows specified (%d)", (int32)colOrder.size(), (int32)rowOrder.size());
	}

	// check that column order matches row order
	if (CMaterialEffectsCVars::Get().mfx_Debug > 0)
	{
		string colName;
		string rowName;
		for (int i=0;i<colOrder.size(); ++i)
		{
			colName.assign(colOrder[i].ptr, colOrder[i].count);
			if (i < rowOrder.size())
			{
				rowName.assign(rowOrder[i].ptr, rowOrder[i].count);
			}
			else
			{
				rowName.clear();
			}
			// CryLogAlways("ExcelColRow=%d col=%s row=%s", i+2, colName.c_str(), rowName.c_str());
			if (colName != rowName)
			{
				GameWarning("ExcelColRow=%d: %s != %s", i+2, colName.c_str(), rowName.c_str());
			}
		}
	}

	pXmlTableReader->Release();

	return true;
}

void CMaterialEffects::PreLoadAssets()
{
	LOADING_TIME_PROFILE_SECTION;

	for (TMFXEffectId id = 0; id < m_effectVec.size(); ++id)
		if (m_effectVec[id])
			m_effectVec[id]->PreLoadAssets();

	if (m_pMaterialFGManager)
		return m_pMaterialFGManager->PreLoad();
}

bool CMaterialEffects::LoadFlowGraphLibs()
{
	if (m_pMaterialFGManager)
		return m_pMaterialFGManager->LoadLibs();
	return false;
}

TMFXEffectId CMaterialEffects::GetEffectIdByName(const char* libName, const char* effectName)
{
	if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return InvalidEffectId;

	// FIXME
	const IMFXEffectPtr pEffectPtr = InternalGetEffect(libName, effectName);
	if (pEffectPtr)
		return pEffectPtr->m_effectParams.effectId;
	return InvalidEffectId;
}

TMFXEffectId CMaterialEffects::GetEffectId(int surfaceIndex1, int surfaceIndex2)
{
	if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return InvalidEffectId;

	
	// Map surface IDs to internal matmat indices
	const TIndex idx1 = SurfaceIdToMatrixEntry(surfaceIndex1);
	const TIndex idx2 = SurfaceIdToMatrixEntry(surfaceIndex2);

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	TMFXEffectId effectId =  InternalGetEffectId(idx1, idx2);

	if (CMaterialEffectsCVars::Get().mfx_DebugVisual)
	{
		if (effectId != InvalidEffectId)
		{
			m_debugVisual.AddLastSearchHint(effectId, surfaceIndex1, surfaceIndex2);
		}
	}

	return effectId;
#else
	return InternalGetEffectId(idx1, idx2);
#endif

	
}

TMFXEffectId CMaterialEffects::GetEffectId(const char *customName, int surfaceIndex2)
{
	if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return InvalidEffectId;

	const TIndex idx1 = stl::find_in_map(m_customConvert, CONST_TEMP_STRING(customName), 0);
	const TIndex idx2 = SurfaceIdToMatrixEntry(surfaceIndex2);

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	TMFXEffectId effectId = InternalGetEffectId(idx1, idx2);

	if (CMaterialEffectsCVars::Get().mfx_DebugVisual)
	{
		if (effectId != InvalidEffectId)
		{
			m_debugVisual.AddLastSearchHint(effectId, customName, surfaceIndex2);
		}
	}

	return effectId;
#else
	return InternalGetEffectId(idx1, idx2);
#endif

}

// Get the cell contents that these parameters equate to
TMFXEffectId CMaterialEffects::GetEffectId(IEntityClass* pEntityClass, int surfaceIndex2)
{
	if (!CMaterialEffectsCVars::Get().mfx_Enable)
		return InvalidEffectId;

	// Map material IDs to effect indexes
	const TIndex idx1 = stl::find_in_map(m_ptrToMatrixEntryMap, pEntityClass, 0);
	const TIndex idx2 = SurfaceIdToMatrixEntry(surfaceIndex2);

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	TMFXEffectId effectId = InternalGetEffectId(idx1, idx2);

	if (CMaterialEffectsCVars::Get().mfx_DebugVisual)
	{
		if (effectId != InvalidEffectId)
		{
			m_debugVisual.AddLastSearchHint(effectId, pEntityClass, surfaceIndex2);
		}
	}

	return effectId;
#else
	return  InternalGetEffectId(idx1, idx2);
#endif

}

SMFXResourceListPtr CMaterialEffects::GetResources(TMFXEffectId effectId)
{
	SMFXResourceListPtr pResourceList = SMFXResourceList::Create();
	IMFXEffectPtr mfx = InternalGetEffect(effectId);
	if (mfx)
		mfx->GetResources(*pResourceList);
  return pResourceList;
}

void CMaterialEffects::TimedEffect(IMFXEffectPtr effect, SMFXRunTimeEffectParams &params)
{
	if (!m_bUpdateMode)
		return;
	m_delayedEffects.push_back(SDelayedEffect(effect, params));
}

void CMaterialEffects::SetUpdateMode(bool bUpdate)
{
	if (!bUpdate)
	{
		m_delayedEffects.clear();
		m_pMaterialFGManager->Reset(false);
	}
//	if (bUpdate && !m_bUpdateMode)
//		PreLoadAssets();
	m_bUpdateMode = bUpdate;
}

void CMaterialEffects::FullReload()
{
	Reset( true );

	LoadFXLibraries();
	Load("MaterialEffects.xml");
	LoadFlowGraphLibs();
}

void CMaterialEffects::Update(float frameTime)
{
	SetUpdateMode(true);
	std::vector< SDelayedEffect >::iterator it = m_delayedEffects.begin();
	std::vector< SDelayedEffect >::iterator next = it;
	while (it != m_delayedEffects.end())
	{
		++next;
		SDelayedEffect& cur = *it;
		cur.m_delay -= frameTime;
		if (cur.m_delay <= 0.0f)
		{
			cur.m_pEffect->Execute(cur.m_effectRuntimeParams);
			next = m_delayedEffects.erase(it);
		}
		it = next;
	}

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	if (CMaterialEffectsCVars::Get().mfx_DebugVisual)
	{
		m_debugVisual.Update(frameTime);
	}
#endif
}

void CMaterialEffects::NotifyFGHudEffectEnd(IFlowGraphPtr pFG)
{
	if (m_pMaterialFGManager)
		m_pMaterialFGManager->EndFGEffect(pFG);
}

void CMaterialEffects::Reset( bool bCleanup )
{
	// make sure all pre load data has been propperly released to not have any
	// dangling pointers are for example the materials itself have been flushed
	for (TMFXEffectId id = 0; id < m_effectVec.size(); ++id)
	{
		if (m_effectVec[id])
		{
			m_effectVec[id]->ReleasePreLoadAssets();
		}
	}

	if (m_pMaterialFGManager)
		m_pMaterialFGManager->Reset(bCleanup);

	if (bCleanup)
	{
		stl::free_container(m_mfxLibraries);
		stl::free_container(m_delayedEffects);
		stl::free_container(m_effectVec);
		stl::free_container(m_customConvert);
		stl::free_container(m_surfaceIdToMatrixEntry);
		stl::free_container(m_ptrToMatrixEntryMap);
		stl::free_container(m_filename);
		m_matmatArray.Free();
		m_bDataInitialized = false;

		SMFXResourceList::FreePool();
		SMFXFlowGraphListNode::FreePool();
		SMFXDecalListNode::FreePool();
		SMFXParticleListNode::FreePool();
		SMFXSoundListNode::FreePool();
		SMFXForceFeedbackListNode::FreePool();
		CMFXRandomEffect::StaticReset();
	}
}

void CMaterialEffects::ClearDelayedEffects()
{
	m_delayedEffects.resize(0);
}

void CMaterialEffects::Serialize(TSerialize ser)
{
	if (m_pMaterialFGManager && CMaterialEffectsCVars::Get().mfx_SerializeFGEffects != 0)
		m_pMaterialFGManager->Serialize(ser);
}

void CMaterialEffects::GetMemoryUsage(ICrySizer * s) const
{
	SIZER_SUBCOMPONENT_NAME(s,"MaterialEffects");
	s->AddObject(this, sizeof(*this));
	s->AddObject(m_filename);
	s->AddObject(m_pMaterialFGManager);	
		
	{
		SIZER_SUBCOMPONENT_NAME(s, "libs");
		s->AddObject(m_mfxLibraries);	
	}
	{
		SIZER_SUBCOMPONENT_NAME(s, "convert");		
		s->AddObject(m_customConvert);
		s->AddObject(m_surfaceIdToMatrixEntry);
		s->AddObject(m_ptrToMatrixEntryMap);
	}
	{
		SIZER_SUBCOMPONENT_NAME(s, "lookup");
		s->AddObject(m_effectVec); // the effects themselves already accounted in "libs"		
		s->AddObject(m_matmatArray);
	}
	{
		SIZER_SUBCOMPONENT_NAME(s, "playing"); 
		s->AddObject(m_delayedEffects); // the effects themselves already accounted in "libs"		
		//s->AddObject((const void*)&CMFXRandomEffect::GetMemoryUsage, CMFXRandomEffect::GetMemoryUsage());
	}
}

static float NormalizeMass(float fMass)
{
	float massMin = 0.0f;
	float massMax = 500.0f;
	float paramMin = 0.0f;
	float paramMax = 1.0f/3.0f;

	// tiny - bullets
	if (fMass <= 0.1f)
	{
		// small
		massMin = 0.0f;
		massMax = 0.1f;
		paramMin = 0.0f;
		paramMax = 1.0f;
	}
	else if (fMass < 20.0f)
	{
		// small
		massMin = 0.0f;
		massMax = 20.0f;
		paramMin = 0.0f;
		paramMax = 1.0f/3.0f;
	}
	else if (fMass < 200.0f)
	{
		// medium
		massMin = 20.0f;
		massMax = 200.0f;
		paramMin = 1.0f/3.0f;
		paramMax = 2.0f/3.0f;
	}
	else
	{
		// ultra large
		massMin = 200.0f;
		massMax = 2000.0f;
		paramMin = 2.0f/3.0f;
		paramMax = 1.0f;
	}

	const float p = min(1.0f, (fMass - massMin)/(massMax - massMin));
	const float finalParam = paramMin + (p * (paramMax - paramMin));
	return finalParam;
}

bool CMaterialEffects::PlayBreakageEffect(ISurfaceType* pSurfaceType, const char* breakageType, const SMFXBreakageParams& breakageParams)
{
	if (pSurfaceType == 0)
		return false;

	CryFixedStringT<128> fxName ("Breakage:");
	fxName+=breakageType;
	TMFXEffectId effectId = this->GetEffectId(fxName.c_str(), pSurfaceType->GetId());
	if (effectId == InvalidEffectId)
		return false;

	// only play sound at the moment
	SMFXRunTimeEffectParams params;
	params.playflags = MFX_PLAY_SOUND;

	// if hitpos is set, use it
	// otherwise use matrix (hopefully been set or 0,0,0)
	if (breakageParams.CheckFlag(SMFXBreakageParams::eBRF_HitPos) && breakageParams.GetHitPos().IsZero() == false)
		params.pos = breakageParams.GetHitPos();
	else
		params.pos = breakageParams.GetMatrix().GetTranslation();

	//params.soundSemantic = eSoundSemantic_Physics_General;

	const Vec3& hitImpulse = breakageParams.GetHitImpulse();
	const float strength = hitImpulse.GetLengthFast();
	params.AddSoundParam("strength", strength);
	const float mass = NormalizeMass(breakageParams.GetMass());
	params.AddSoundParam("mass", mass);

	if (CMaterialEffectsCVars::Get().mfx_Debug & 2)
	{
		IMFXEffectPtr pEffect = InternalGetEffect(effectId);
		if (pEffect != 0)
		{
			CryLogAlways("[MFX]: %s:%s FX=%s:%s Pos=%f,%f,%f NormMass=%f  F=%f Imp=%f,%f,%f  RealMass=%f Vel=%f,%f,%f",
				breakageType, pSurfaceType->GetName(), pEffect->m_effectParams.libName.c_str(), pEffect->m_effectParams.name.c_str(),
				params.pos[0],params.pos[1],params.pos[2], 
				mass,
				strength, 
				breakageParams.GetHitImpulse()[0],
				breakageParams.GetHitImpulse()[1],
				breakageParams.GetHitImpulse()[2],
				breakageParams.GetMass(),
				breakageParams.GetVelocity()[0],
				breakageParams.GetVelocity()[1],
				breakageParams.GetVelocity()[2]
				);
		}
	}

	/*
	if (breakageParams.GetMass() == 0.0f)
	{
		int a = 0;
	}
	*/


	const bool bSuccess = ExecuteEffect(effectId, params);

	return bSuccess;
}

void CMaterialEffects::CompleteInit()
{
	if (m_bDataInitialized)
		return;

	LoadFXLibraries();
	Load("MaterialEffects.xml");
	LoadFlowGraphLibs();
}

int	CMaterialEffects::GetDefaultCanopyIndex()
{
#ifdef DEBUG_VISUAL_MFX_SYSTEM
	if(m_defaultSurfaceId == m_canopySurfaceId)
	{
		GameWarning("[MFX] CMaterialEffects::GetDefaultCanopyIndex returning default - called before MFX loaded");
	}	
#endif

	return m_canopySurfaceId;
}

void CMaterialEffects::ReloadMatFXFlowGraphs()
{
	m_pMaterialFGManager->ReloadFlowGraphs();
}

int CMaterialEffects::GetMatFXFlowGraphCount() const
{
	return m_pMaterialFGManager->GetFlowGraphCount();
}

IFlowGraphPtr CMaterialEffects::GetMatFXFlowGraph(int index, string* pFileName /*= NULL*/) const
{
	return m_pMaterialFGManager->GetFlowGraph(index, pFileName);
}

IFlowGraphPtr CMaterialEffects::LoadNewMatFXFlowGraph(const string& filename)
{
	IFlowGraphPtr res;
	m_pMaterialFGManager->LoadFG(filename, &res);
	return res;
}


#include UNIQUE_VIRTUAL_WRAPPER(IMaterialEffects)
