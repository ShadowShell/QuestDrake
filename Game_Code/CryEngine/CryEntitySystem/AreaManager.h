////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   AreaManager.h
//  Version:     v1.00
//  Created:     24/02/2002 by Kirill Bulatsev.
//  Compilers:   Visual Studio.NET
//  Description: 2D area class. Area is in XY plane. Area has entities attached to it.
//	Area has fade width (m_Proximity) - distance from border to inside at wich fade coefficient
//	changes linearly from 0(on the border) to 1(inside distance to border more than m_Proximity). 
//	
//	Description: 2D areas manager. Checks player for entering/leaving areas. Updates fade 
//	coefficient for areas player is in

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AreaManager_h__
#define __AreaManager_h__
#pragma once

#include <VectorMap.h>
#include "AreaGrid.h"

class CEntitySystem;
class CArea;
struct IVisArea;
struct IAreaManagerEventListener;

typedef std::vector<IAreaManagerEventListener*>	TAreaManagerEventListenerVector;

//Areas manager
class CAreaManager : public IAreaManager, public ISystemEventListener
{
	struct SAreaCacheEntry
	{
		SAreaCacheEntry()
		: pArea(NULL),
			bNear(false),
			bInside(false),
			bInGrid(true)
		{};

		SAreaCacheEntry(CArea* const pPassedArea, bool const bIsNear, bool const bIsInside)
		: pArea(pPassedArea),
			bNear(bIsNear),
			bInside(bIsInside),
			bInGrid(true)
		{};

		void operator = (SAreaCacheEntry const& rOther)
		{
			pArea		= rOther.pArea;
			bInGrid	= rOther.bInGrid;
			bInside	= rOther.bInside;
			bNear		= rOther.bNear;
		}

		CArea*	pArea;
		bool		bInGrid;
		bool		bInside;
		bool		bNear;
	};

	typedef std::vector<SAreaCacheEntry> TAreaCacheVector;

	struct SAreasCache
	{
		SAreasCache()
		: vLastUpdatePos(ZERO)
		{}

		bool GetCacheEntry(CArea const* const pAreaToFind, SAreaCacheEntry** const ppAreaCacheEntry)
		{
			bool bSuccess = false;
			*ppAreaCacheEntry = NULL;
			TAreaCacheVector::iterator Iter(aoAreas.begin());
			TAreaCacheVector::const_iterator const IterEnd(aoAreas.end());

			for(; Iter != IterEnd; ++Iter)
			{
				if ((*Iter).pArea == pAreaToFind)
				{
					*ppAreaCacheEntry = &(*Iter);
					bSuccess = true;

					break;
				}
			}

			return bSuccess;
		}

		TAreaCacheVector aoAreas;
		Vec3 vLastUpdatePos;
	};

	typedef VectorMap<EntityId, SAreasCache>	TAreaCacheMap;
	typedef VectorMap<EntityId, size_t>				TEntitiesToUpdateMap;

public:

	CAreaManager(CEntitySystem* pEntitySystem);
	~CAreaManager(void);

	//IAreaManager
	VIRTUAL size_t							GetAreaAmount() const {return m_apAreas.size();}
	VIRTUAL IArea const* const	GetArea(size_t const nAreaIndex) const;
  VIRTUAL size_t              GetOverlappingAreas(const AABB& bb, PodArray<IArea*>& list) const;
	VIRTUAL void								SetAreasDirty();
	VIRTUAL void								SetAreaDirty(IArea* pArea);

	VIRTUAL void								ExitAllAreas(IEntity const* const pEntity);
	//~IAreaManager
	
	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	// Makes a new area.
	CArea* CreateArea();

	CEntitySystem* GetEntitySystem() const { return m_pEntitySystem; };

	// Puts the passed entity ID into the update list for the next update.
	VIRTUAL void MarkEntityForUpdate(EntityId const nEntityID);

	// Called every frame.
	void Update();

	bool ProceedExclusiveUpdateByHigherArea(SAreasCache* const pAreaCache, IEntity const* const pEntity, Vec3 const& rEntityPos, CArea* const pArea, Vec3 const& vOnLowerHull);
	void NotifyAreas(CArea* const __restrict pArea, SAreasCache const* const pAreaCache, IEntity const* const pEntity);

	VIRTUAL void DrawLinkedAreas(EntityId linkedId) const;
	size_t GetLinkedAreas(EntityId linkedId, int areaId, std::vector<CArea *> &areas) const;

	VIRTUAL bool GetLinkedAreas(EntityId linkedId, EntityId* pOutArray, int &outAndMaxResults) const;

	void	DrawAreas(const ISystem * const pSystem);
	void DrawGrid();
	unsigned MemStat();
	void ResetAreas();
	void UnloadAreas();

	VIRTUAL bool QueryAreas(EntityId const nEntityID, Vec3 const& rPos, SAreaManagerResult *pResults, int nMaxResults, int& rNumResults);
	VIRTUAL bool QueryAudioAreas(Vec3 const& rPos, SAudioAreaInfo *pResults, size_t const nMaxResults, size_t& rNumResults);

	// Register EventListener to the AreaManager.
	VIRTUAL void AddEventListener( IAreaManagerEventListener *pListener );
	VIRTUAL void RemoveEventListener( IAreaManagerEventListener *pListener );

	//! Fires event for all listeners to this sound.
	void  OnEvent( EEntityEvent event, EntityId TriggerEntityID, IArea *pArea );

	int GetNumberOfPlayersInArea(CArea const* const pArea);

protected:

	friend class CArea;
	void Unregister(CArea const* const pArea);

	// List of all registered area pointers.
	TAreaPointers m_apAreas;

	CEntitySystem* m_pEntitySystem;
	int m_nCurSoundStep;
	int m_nCurTailStep;
	bool m_bAreasDirty;
	CAreaGrid m_areaGrid;

private:

	IVisArea *m_pPrevArea,*m_pCurrArea;

	TAreaManagerEventListenerVector m_EventListeners;

	//////////////////////////////////////////////////////////////////////////
	SAreasCache* GetAreaCache(EntityId const nEntityId)
	{
		SAreasCache* pAreaCache = NULL;
		TAreaCacheMap::iterator const Iter(m_mapAreaCache.find(nEntityId));

		if (Iter != m_mapAreaCache.end())
		{
			pAreaCache = &(Iter->second);
		}

		return pAreaCache;
	}

	//////////////////////////////////////////////////////////////////////////
	SAreasCache* MakeAreaCache(EntityId const nEntityID)
	{
		SAreasCache* pAreaCache = &m_mapAreaCache[nEntityID];
		return pAreaCache;
	}

	//////////////////////////////////////////////////////////////////////////
	void DeleteAreaCache(EntityId const nEntityId)
	{
		m_mapAreaCache.erase(nEntityId);
	}

	void UpdateEntity(Vec3 const& rPos, IEntity const* const pEntity);
	void UpdateDirtyAreas();
	void ProcessArea(CArea* const __restrict pArea, SAreaCacheEntry& rAreaCacheEntry, SAreasCache* const pAreaCache, Vec3 const& rPos, IEntity const* const pEntity);

	// Unary predicates for conditional removing!
	static inline bool IsDoneUpdating(std::pair<EntityId, size_t> const& rEntry)
	{
		return rEntry.second == 0;
	}

	struct SIsNotInGrid
	{
		SIsNotInGrid(IEntity const* const pPassedEntity, std::vector<CArea*> const& rapPassedAreas, size_t const nPassedCountAreas)
			:	pEntity(pPassedEntity),
			rapAreas(rapPassedAreas),
			nCountAreas(nPassedCountAreas){}

		bool operator()(SAreaCacheEntry const& rCacheEntry) const;

		IEntity const* const pEntity;
		std::vector<CArea*> const& rapAreas;
		size_t const nCountAreas;
	};

	struct SRemoveIfIsNearOrInside
	{
		SRemoveIfIsNearOrInside(IEntity const* const pPassedEntity, std::vector<CArea*> const& rapPassedAreas, size_t const nPassedCountAreas)
			: pEntity(pPassedEntity),
			rapAreas(rapPassedAreas),
			nCountAreas(nPassedCountAreas){}

		bool operator()(SAreaCacheEntry const& rCacheEntry) const;

		IEntity const* const pEntity;
		std::vector<CArea*> const& rapAreas;
		size_t const nCountAreas;
	};

	struct SRemoveIfNoAreasLeft
	{
		SRemoveIfNoAreasLeft(CArea const* const pPassedArea, std::vector<CArea*> const& rapPassedAreas, size_t const nPassedCountAreas)
			:	pArea(pPassedArea),
			rapAreas(rapPassedAreas),
			nCountAreas(nPassedCountAreas){}

		bool operator()(VectorMap<EntityId, SAreasCache>::value_type& rCacheEntry) const;

		CArea const* const pArea;
		std::vector<CArea*> const& rapAreas;
		size_t const nCountAreas;
	};

	TAreaCacheMap         m_mapAreaCache;					// Area cache per entity id.
	TEntitiesToUpdateMap  m_mapEntitiesToUpdate;
};

#endif //__AreaManager_h__
