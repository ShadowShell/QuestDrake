////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   MaterialEffects.h
//  Version:     v1.00
//  Created:     28/11/2006 by JohnN/AlexL
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __MATERIALEFFECTS_H__
#define __MATERIALEFFECTS_H__

#pragma once

#include <map>
#include <ISystem.h>
#include <IMaterialEffects.h>
#include "IMFXEffect.h"
#include <SerializeFwd.h>


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//Visual debug info for MFX system

#if !defined(_RELEASE)
	#define DEBUG_VISUAL_MFX_SYSTEM
#endif

#ifdef DEBUG_VISUAL_MFX_SYSTEM

#define MAX_DEBUG_VISUAL_MFX_ENTRIES 48
#define DEFAULT_DEBUG_VISUAL_MFX_LIFETIME 12.0f

class CMaterialEffects;

struct SDebugVisualMFXSystem
{
private:

	struct SDebugVisualEntry
	{
		SDebugVisualEntry()
			: lifeTime(0.0f)
			, fxId(InvalidEffectId)
			, materialName1("")
			, materialName2("")
		{

		}

		CryFixedStringT<32> materialName1;
		CryFixedStringT<32> materialName2;

		Vec3 fxPosition;
		Vec3 fxDirection;

		float lifeTime;

		TMFXEffectId fxId;
	};

	struct SLastSearchHint
	{
		SLastSearchHint()
		{
			Reset();
		}

		void Reset()
		{
			materialName1 = "";
			materialName2 = "";
			fxId = InvalidEffectId;
		}

		CryFixedStringT<32> materialName1;
		CryFixedStringT<32> materialName2;
		
		TMFXEffectId fxId; 
	};

public:

	SDebugVisualMFXSystem()
		: m_nextHit(0)
		, m_pMaterialFX(NULL)
	{

	}

	ILINE void InitWith(CMaterialEffects* pMaterialFX)
	{
		assert(pMaterialFX);
		m_pMaterialFX = pMaterialFX;
	}

	void AddLastSearchHint(const TMFXEffectId effectId, int surfaceIndex1, int surfaceIndex2);
	void AddLastSearchHint(const TMFXEffectId effectId, const char* customName, int surfaceIndex2);
	void AddLastSearchHint(const TMFXEffectId effectId, IEntityClass* pEntityClass, int surfaceIndex2);

	void AddEffectDebugVisual(const TMFXEffectId effectId, const SMFXRunTimeEffectParams& runtimeParams);
	void Update(float frameTime);

private:

	
	CMaterialEffects* m_pMaterialFX;

	SLastSearchHint m_lastSearchHint;

	SDebugVisualEntry m_effectList[MAX_DEBUG_VISUAL_MFX_ENTRIES];
	uint32 m_nextHit;
};

#endif //DEBUG_VISUAL_MFX_SYSTEM
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CMaterialFGManager;
class CScriptBind_MaterialEffects;

class CMaterialEffects : public IMaterialEffects
{
	friend class CScriptBind_MaterialEffects;

public:
	CMaterialEffects();
	virtual ~CMaterialEffects();

	// load spreadsheet and effect libraries
	bool Load(const char* filenameSpreadSheet);

	// load flowgraph based effects 
	bool LoadFlowGraphLibs();

	// serialize
	void Serialize(TSerialize ser);

	// load assets referenced by material effects.
	void PreLoadAssets();

	// IMaterialEffects
	VIRTUAL void LoadFXLibraries();
	VIRTUAL void Reset( bool bCleanup );
	VIRTUAL void ClearDelayedEffects();
	VIRTUAL TMFXEffectId GetEffectIdByName(const char* libName, const char* effectName);
	VIRTUAL TMFXEffectId GetEffectId(int surfaceIndex1, int surfaceIndex2);
	VIRTUAL TMFXEffectId GetEffectId(const char* customName, int surfaceIndex2);
	VIRTUAL TMFXEffectId GetEffectId(IEntityClass* pEntityClass, int surfaceIndex2);
	VIRTUAL SMFXResourceListPtr GetResources(TMFXEffectId effectId);
	VIRTUAL bool ExecuteEffect(TMFXEffectId effectId, SMFXRunTimeEffectParams& runtimeParams);
	VIRTUAL void StopEffect(TMFXEffectId effectId);
	VIRTUAL int	GetDefaultSurfaceIndex() { return m_defaultSurfaceId; }
	VIRTUAL int	GetDefaultCanopyIndex();
	VIRTUAL bool PlayBreakageEffect(ISurfaceType* pSurfaceType, const char* breakageType, const SMFXBreakageParams& breakageParams);
	VIRTUAL void SetCustomParameter(TMFXEffectId effectId, const char* customParameter, const SMFXCustomParamValue& customParameterValue);
	VIRTUAL void CompleteInit();
	VIRTUAL void ReloadMatFXFlowGraphs();
	VIRTUAL int GetMatFXFlowGraphCount() const;
	VIRTUAL IFlowGraphPtr GetMatFXFlowGraph(int index, string* pFileName = NULL) const;
	VIRTUAL IFlowGraphPtr LoadNewMatFXFlowGraph(const string& filename);
	// ~IMaterialEffects

	void GetMemoryUsage(ICrySizer * s) const;
	void NotifyFGHudEffectEnd(IFlowGraphPtr pFG);
	void Update(float frameTime);
	void SetUpdateMode(bool bMode);
	CMaterialFGManager* GetFGManager() const { return m_pMaterialFGManager; }

	void FullReload();

protected:
	static const int MAX_SURFACE_COUNT = 255+1; // from SurfaceTypeManager.h, but will also work with any other number
	typedef void*   TPtrIndex;
	typedef int     TIndex;    // normal index 
	typedef std::pair<TIndex, TIndex> TIndexPair;
	typedef std::map<TMFXNameId, IMFXEffectPtr, stl::less_stricmp<string> > TFXLibrary;
	typedef std::map<TMFXNameId, TFXLibrary, stl::less_stricmp<string> > TFXLibrariesMap;

	struct SDelayedEffect 
	{
		SDelayedEffect() 
			: m_delay(0.0f)
		{

		}
		SDelayedEffect(IMFXEffectPtr pEffect, SMFXRunTimeEffectParams& runtimeParams)
			: m_pEffect(pEffect), m_effectRuntimeParams(runtimeParams), m_delay(m_pEffect->m_effectParams.delay)
		{
		}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_pEffect);
		}

		IMFXEffectPtr m_pEffect;
		SMFXRunTimeEffectParams m_effectRuntimeParams;
		float m_delay;
	};

	template <typename T> struct Array2D
	{
		void Create(size_t rows, size_t cols)
		{
			if (m_pData)
				delete[] m_pData;
			m_nRows = rows;
			m_nCols = cols;
			m_pData = new T[m_nRows*m_nCols];
			memset(m_pData, 0, m_nRows*m_nCols*sizeof(T));
		}

		Array2D() : m_pData(0), m_nRows(0), m_nCols(0) 
		{
		}

		~Array2D()
		{
			if (m_pData)
				delete[] m_pData;
		}

		void Free()
		{
			delete[] m_pData;
			m_pData = NULL;
		}

		T* operator[] (const size_t row) const
		{
			assert (m_pData);
			assert (row >= 0 && row < m_nRows);
			return &m_pData[row*m_nCols];
		}

		T& operator() (const size_t row, const size_t col) const
		{
			assert (m_pData);
			assert (row >= 0 && row < m_nRows && col >= 0 && col < m_nCols);
			return m_pData[row*m_nCols+col];
		}

		T GetValue(const size_t row, const size_t col, const T& defaultValue) const
		{
			return (m_pData && row >= 0 && row < m_nRows && col >= 0 && col < m_nCols) ? m_pData[row*m_nCols+col] : defaultValue;
		}
		void GetMemoryUsage(ICrySizer * s) const
		{
			s->AddObject(m_pData, sizeof(T)*m_nRows*m_nCols);
		}

		T* m_pData;
		size_t m_nRows;
		size_t m_nCols;
	};

	// Refactor remains

	void Load();
	void LoadFXLibrary(const char *name);

	// schedule effect
	void TimedEffect(IMFXEffectPtr effect, SMFXRunTimeEffectParams& params);

	// index1 x index2 are used to lookup in m_matmat array
	inline TMFXEffectId InternalGetEffectId(int index1, int index2) const
	{
		const TMFXEffectId effectId = m_matmatArray.GetValue(index1, index2, InvalidEffectId);
		return effectId;
	}

	inline IMFXEffectPtr InternalGetEffect(const char* libName, const char* effectName) const
	{
		TFXLibrariesMap::const_iterator iter = m_mfxLibraries.find(CONST_TEMP_STRING(libName));
		if (iter != m_mfxLibraries.end())
		{
			const TFXLibrary& library = iter->second;
			return stl::find_in_map(library, CONST_TEMP_STRING(effectName), 0);
		}
		return 0;
	}

	inline IMFXEffectPtr InternalGetEffect(TMFXEffectId effectId) const
	{
		assert (effectId < m_effectVec.size());
		if (effectId < m_effectVec.size())
			return m_effectVec[effectId];
		return 0;
	}

	inline TIndex SurfaceIdToMatrixEntry(int surfaceIndex)
	{
		return (surfaceIndex >= 0 && surfaceIndex < m_surfaceIdToMatrixEntry.size()) ? m_surfaceIdToMatrixEntry[surfaceIndex] : 0;
	}

protected:
	ISystem*            m_pSystem;
	string              m_filename;
	int                 m_defaultSurfaceId;
	int                 m_canopySurfaceId;
	bool                m_bUpdateMode;
	bool				m_bDataInitialized;
	CMaterialFGManager* m_pMaterialFGManager;

	// the libraries we have loaded
	TFXLibrariesMap m_mfxLibraries;

	// this maps a surface type to the corresponding column(==row) in the matrix
	std::vector<TIndex> m_surfaceIdToMatrixEntry;

	// this maps custom pointers (entity classes
	std::map<TPtrIndex, TIndex> m_ptrToMatrixEntryMap;

	// converts custom tags to indices
	std::map<string, TIndex, stl::less_stricmp<string> > m_customConvert;

	// main lookup surface x surface -> effectId
	Array2D<TMFXEffectId> m_matmatArray;
	std::vector<IMFXEffectPtr> m_effectVec; // indexed by TEffectId

	// runtime effects which are delayed
	std::vector<SDelayedEffect> m_delayedEffects;

#ifdef DEBUG_VISUAL_MFX_SYSTEM
	friend struct SDebugVisualMFXSystem;

	SDebugVisualMFXSystem m_debugVisual;
#endif
};


#endif
