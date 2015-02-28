////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   EntitySystem.h
//  Version:     v1.00
//  Created:     24/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntitySystem_h__
#define __EntitySystem_h__
#pragma once

#include "IEntitySystem.h"
#include <ISystem.h>
#include "ITimer.h"
#include "SaltBufferArray.h"					// SaltBufferArray<>
#include "EntityTimeoutList.h"
#include <StlUtils.h>
#include "STLPoolAllocator.h"
#include "STLGlobalAllocator.h"

//////////////////////////////////////////////////////////////////////////
// forward declarations.
//////////////////////////////////////////////////////////////////////////
class  CEntity;
struct ICVar;
struct IPhysicalEntity;
class  IComponent;
class  CComponentEventDistributer;
class  CEntityClassRegistry;
class  CScriptBind_Entity;
class  CPhysicsEventListener;
class  CAreaManager;
class  CBreakableManager;
class  CEntityArchetypeManager;
class  CPartitionGrid;
class  CProximityTriggerSystem;
class  CEntityLayer;
class  CEntityLoadManager;
class  CEntityPoolManager;
struct SEntityLayerGarbage;
class CGeomCacheAttachmentManager;
class CCharacterBoneAttachmentManager;
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
typedef stl::hash_map<EntityId,CEntity*> EntityMap;



//////////////////////////////////////////////////////////////////////////
struct SEntityTimerEvent
{
	EntityId entityId;
	int nTimerId;
	int nMilliSeconds;
};



//////////////////////////////////////////////////////////////////////////
struct SEntityAttachment
{
	EntityId child;
	EntityId parent;
	EntityGUID parentGuid;
	Vec3 pos;
	Quat rot;
	Vec3 scale;
	bool guid;
	int flags;
	string target;

	SEntityAttachment() : child(0), parent(0), parentGuid(0), pos(ZERO), rot(ZERO), scale(ZERO), guid(false), flags(0) {}
};



//////////////////////////////////////////////////////////////////////////
// Structure for extended information about loading an entity
//	Supports reusing an entity container if specified
struct SEntityLoadParams
{
	SEntitySpawnParams spawnParams;
	XmlNodeRef entityNode;
	CEntity *pReuseEntity;
	bool bCallInit;

	SEntityLoadParams();
	SEntityLoadParams(CEntity *pReuseEntity, SEntitySpawnParams &resetParams);
	~SEntityLoadParams();

	SEntityLoadParams& operator =(const SEntityLoadParams& other);
	SEntityLoadParams(const SEntityLoadParams& other) { *this = other; }
	void UseClonedEntityNode(const XmlNodeRef sourceEntityNode, XmlNodeRef parentNode);

private:
	void AddRef();
	void RemoveRef();

	static bool CloneXmlNode(const XmlNodeRef sourceNode, XmlNodeRef destNode);
};
typedef std::vector<SEntityLoadParams> TEntityLoadParamsContainer;


typedef CSaltHandle<unsigned short,unsigned short> CEntityHandle;

//////////////////////////////////////////////////////////////////////
class CEntitySystem : public IEntitySystem
{
public:
	CEntitySystem(ISystem *pSystem);
	~CEntitySystem();

	bool Init(ISystem *pSystem);

	// interface IEntitySystem ------------------------------------------------------
	VIRTUAL void RegisterCharactersForRendering();
	virtual void Release();
	VIRTUAL void Update();
	VIRTUAL void DeletePendingEntities();
	VIRTUAL void PrePhysicsUpdate();
	VIRTUAL void Reset();
	VIRTUAL void Unload();
	VIRTUAL void PurgeHeaps();
	VIRTUAL IEntityClassRegistry* GetClassRegistry();	
	VIRTUAL IEntity* SpawnEntity( SEntitySpawnParams &params,bool bAutoInit=true );
	VIRTUAL bool InitEntity( IEntity* pEntity,SEntitySpawnParams &params );
	VIRTUAL IEntity* GetEntity( EntityId id ) const;
	VIRTUAL IEntity* FindEntityByName( const char *sEntityName ) const;
	VIRTUAL void ReserveEntityId( const EntityId id );
	VIRTUAL EntityId ReserveUnknownEntityId();
	VIRTUAL void RemoveEntity( EntityId entity,bool bForceRemoveNow=false );
	VIRTUAL uint32 GetNumEntities() const;
	VIRTUAL IEntityIt* GetEntityIterator();
	VIRTUAL void SendEventViaEntityEvent( IEntity* piEntity, SEntityEvent &event );
	VIRTUAL void SendEventToAll( SEntityEvent &event );
	VIRTUAL int QueryProximity( SEntityProximityQuery &query );
	VIRTUAL void ResizeProximityGrid( int nWidth,int nHeight );
	VIRTUAL int GetPhysicalEntitiesInBox( const Vec3 &origin, float radius,IPhysicalEntity **&pList, int physFlags ) const;
	VIRTUAL IEntity* GetEntityFromPhysics( IPhysicalEntity *pPhysEntity ) const;
	VIRTUAL void AddSink( IEntitySystemSink *pSink, uint32 subscriptions, uint64 onEventSubscriptions );
	VIRTUAL void RemoveSink( IEntitySystemSink *pSink );
	VIRTUAL void PauseTimers(bool bPause,bool bResume=false);
	VIRTUAL bool IsIDUsed( EntityId nID ) const;
	VIRTUAL void GetMemoryStatistics(ICrySizer *pSizer) const;
	VIRTUAL ISystem* GetSystem() const { return m_pISystem; };
	VIRTUAL void SetNextSpawnId(EntityId id);
	VIRTUAL void ResetAreas();
	VIRTUAL void UnloadAreas();

	VIRTUAL void AddEntityEventListener( EntityId nEntity,EEntityEvent event,IEntityEventListener *pListener );
	VIRTUAL void RemoveEntityEventListener( EntityId nEntity,EEntityEvent event,IEntityEventListener *pListener );

	VIRTUAL EntityId FindEntityByGuid( const EntityGUID &guid ) const;
	VIRTUAL EntityId GenerateEntityIdFromGuid( const EntityGUID &guid );

	VIRTUAL IEntityArchetype* LoadEntityArchetype( XmlNodeRef oArchetype);
	VIRTUAL IEntityArchetype* LoadEntityArchetype( const char *sArchetype );
	VIRTUAL IEntityArchetype* CreateEntityArchetype( IEntityClass *pClass,const char *sArchetype );

	VIRTUAL void Serialize(TSerialize ser);

	VIRTUAL void DumpEntities();

	VIRTUAL void ResumePhysicsForSuppressedEntities(bool bWakeUp);
	VIRTUAL void SaveInternalState(struct IDataWriteStream& writer) const;
	VIRTUAL void LoadInternalState(struct IDataReadStream& reader);

	VIRTUAL int GetLayerId(const char* szLayerName) const;
	VIRTUAL int GetVisibleLayerIDs(uint8* pLayerMask, const uint32 maxCount) const;
	VIRTUAL void ToggleLayerVisibility(const char* layer, bool isEnabled);

	VIRTUAL void LockSpawning(bool lock) {m_bLocked = lock;}
	
	VIRTUAL bool OnLoadLevel(const char* szLevelPath);
	void OnLevelLoadStart();

	VIRTUAL CEntityLayer* AddLayer(const char* name, const char* parent, uint16 id, bool bHasPhysics, int specs, bool bDefaultLoaded);
	VIRTUAL void LoadLayers(const char* dataFile);
	VIRTUAL void LinkLayerChildren();
	VIRTUAL void AddEntityToLayer(const char* layer, EntityId id);
	VIRTUAL void RemoveEntityFromLayers(EntityId id);
	VIRTUAL void ClearLayers();
	VIRTUAL void EnableDefaultLayers(bool isSerialized = true);
	VIRTUAL void EnableLayer(const char* layer, bool isEnable, bool isSerialized = true);
	VIRTUAL bool IsLayerEnabled(const char* layer, bool bMustBeLoaded) const;
	VIRTUAL bool ShouldSerializedEntity(IEntity* pEntity);
	VIRTUAL void RegisterPhysicCallbacks();
	VIRTUAL void UnregisterPhysicCallbacks();
	CEntityLayer* FindLayer(const char* layer);

	// ------------------------------------------------------------------------

	CEntityLayer* GetLayerForEntity(EntityId id);

	bool OnBeforeSpawn(SEntitySpawnParams &params);
	void OnEntityReused(IEntity *pEntity, SEntitySpawnParams &params);

	// Sets new entity timer event.
	void AddTimerEvent( SEntityTimerEvent &event, CTimeValue startTime = gEnv->pTimer->GetFrameStartTime());

	//////////////////////////////////////////////////////////////////////////
	// Load entities from XML.
	void LoadEntities( XmlNodeRef &objectsNode, bool bIsLoadingLevelFile );
	void LoadEntities( XmlNodeRef &objectsNode, bool bIsLoadingLevelFile, const Vec3 &segmentOffset, std::vector<IEntity *> *outGlobalEntityIds, std::vector<IEntity *> *outLocalEntityIds );

	ILINE CComponentEventDistributer* GetEventDistributer() { return m_pEventDistributer; }

	virtual bool ExtractEntityLoadParams(XmlNodeRef &entityNode, SEntitySpawnParams &spawnParams) const;
	virtual void BeginCreateEntities(int nAmtToCreate);
	virtual bool CreateEntity(XmlNodeRef &entityNode,SEntitySpawnParams &pParams,EntityId &outUsingId);
	virtual void EndCreateEntities();

	//////////////////////////////////////////////////////////////////////////
	// Called from CEntity implementation.
	//////////////////////////////////////////////////////////////////////////
	void RemoveTimerEvent( EntityId id,int nTimerId );
	void ChangeEntityNameRemoveTimerEvent( EntityId id );

	// Puts entity into active list.
	void ActivateEntity( CEntity *pEntity,bool bActivate );
	void ActivatePrePhysicsUpdateForEntity( CEntity *pEntity,bool bActivate );
	bool IsPrePhysicsActive( CEntity * pEntity );
	void OnEntityEvent( CEntity *pEntity,SEntityEvent &event );

	// Access to class that binds script to entity functions.
	// Used by Script proxy.
	CScriptBind_Entity* GetScriptBindEntity() { return m_pEntityScriptBinding; };

	// Access to area manager.
	IAreaManager* GetAreaManager() const { return (IAreaManager*)(m_pAreaManager); }

	// Access to breakable manager.
	VIRTUAL IBreakableManager* GetBreakableManager() const { return m_pBreakableManager; };

	// Access to entity pool manager.
	VIRTUAL IEntityPoolManager* GetIEntityPoolManager() const { return (IEntityPoolManager*)m_pEntityPoolManager; }
	CEntityPoolManager* GetEntityPoolManager() const { return m_pEntityPoolManager; }
	
	CEntityLoadManager* GetEntityLoadManager() const { return m_pEntityLoadManager; }

	CGeomCacheAttachmentManager *GetGeomCacheAttachmentManager() const { return m_pGeomCacheAttachmentManager; }
	CCharacterBoneAttachmentManager *GetCharacterBoneAttachmentManager() const { return m_pCharacterBoneAttachmentManager; }

	static ILINE uint16 IdToIndex(const EntityId id) { return id & 0xffff; }
	static ILINE CSaltHandle<> IdToHandle( const EntityId id ) { return CSaltHandle<>(id>>16,id&0xffff); }
	static ILINE EntityId HandleToId( const CSaltHandle<> id ) { return (((uint32)id.GetSalt())<<16) | ((uint32)id.GetIndex()); }

	EntityId GenerateEntityId(bool bStaticId);
	bool ResetEntityId(CEntity *pEntity, EntityId newEntityId);
	void RegisterEntityGuid( const EntityGUID &guid,EntityId id );
	void UnregisterEntityGuid( const EntityGUID &guid );

	CPartitionGrid* GetPartitionGrid() { return m_pPartitionGrid; }
	CProximityTriggerSystem * GetProximityTriggerSystem() { return m_pProximityTriggerSystem;	}

	void ChangeEntityName( CEntity *pEntity,const char *sNewName );

	void RemoveEntityEventListeners( CEntity *pEntity );

	CEntity* GetEntityFromID( EntityId id ) const;
	ILINE bool HasEntity( EntityId id ) const {	return GetEntityFromID(id) != 0; };

	VIRTUAL void PurgeDeferredCollisionEvents( bool bForce = false );

	void ComponentRegister( EntityId id, IComponentPtr pComponent, const int flags );
	VIRTUAL	void ComponentEnableEvent( const EntityId id, const int eventID, const bool enable );

	VIRTUAL void DebugDraw();
	
	VIRTUAL bool EntitiesUseGUIDs() const { return m_bEntitiesUseGUIDs; }
	VIRTUAL void SetEntitiesUseGUIDs(const bool bEnable) { m_bEntitiesUseGUIDs = bEnable; }
	
private: // -----------------------------------------------------------------
	void DoPrePhysicsUpdate();
	void DoPrePhysicsUpdateFast();
	void DoUpdateLoop(float fFrameTime);

//	void InsertEntity( EntityId id,CEntity *pEntity );
	void DeleteEntity(CEntity* pEntity);
	void UpdateEntity(CEntity *ce,SEntityUpdateContext &ctx);
	void UpdateDeletedEntities();
	void RemoveEntityFromActiveList(CEntity *pEntity);

	void UpdateNotSeenTimeouts();

	void OnBind( EntityId id,EntityId child );
	void OnUnbind( EntityId id,EntityId child );

	void UpdateEngineCVars();
	void UpdateTimers();
	void DebugDraw( CEntity *pEntity,float fUpdateTime );

	void DebugDrawEntityUsage();
	void DebugDrawLayerInfo();
	void DebugDrawProximityTriggers();

	void ClearEntityArray();

  void DumpEntity(IEntity* pEntity);

	void UpdateTempActiveEntities();

	// slow - to find specific problems
	void CheckInternalConsistency() const;
	

	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////
	struct OnEventSink
	{
		OnEventSink(uint64 _subscriptions, IEntitySystemSink* _pSink)
			: subscriptions(_subscriptions)
			, pSink(_pSink)
		{
		}

		uint64 subscriptions;
		IEntitySystemSink* pSink;
	};

	typedef std::vector<OnEventSink, stl::STLGlobalAllocator<OnEventSink> > EntitySystemOnEventSinks;
	typedef std::vector<IEntitySystemSink*, stl::STLGlobalAllocator<IEntitySystemSink*> > EntitySystemSinks;
	typedef std::vector<CEntity*> DeletedEntities;
	typedef std::multimap<CTimeValue,SEntityTimerEvent,std::less<CTimeValue>,stl::STLPoolAllocator<std::pair<CTimeValue,SEntityTimerEvent>,stl::PoolAllocatorSynchronizationSinglethreaded> > EntityTimersMap;
	typedef std::multimap<const char*,EntityId,stl::less_stricmp<const char*> > EntityNamesMap;
	typedef std::map<EntityId,CEntity*> EntitiesMap;
	typedef std::set<EntityId> EntitiesSet;
	typedef std::vector<SEntityTimerEvent> EntityTimersVector;

	EntitySystemSinks								m_sinks[SinkMaxEventSubscriptionCount];	// registered sinks get callbacks for creation and removal
	EntitySystemOnEventSinks				m_onEventSinks;

	ISystem *												m_pISystem;
	std::vector<CEntity *>					m_EntityArray;					// [id.GetIndex()]=CEntity
	DeletedEntities									m_deletedEntities;
	std::vector<CEntity*>						m_deferredUsedEntities;
	EntitiesMap                     m_mapActiveEntities;		// Map of currently active entities (All entities that need per frame update).
	bool                            m_tempActiveEntitiesValid; // must be set to false whenever m_mapActiveEntities is changed
	EntitiesSet                     m_mapPrePhysicsEntities; // map of entities requiring pre-physics activation

	EntityNamesMap                  m_mapEntityNames;  // Map entity name to entity ID.

	CSaltBufferArray<>							m_EntitySaltBuffer;			// used to create new entity ids (with uniqueid=salt)
	std::vector<EntityId>           m_tempActiveEntities;   // Temporary array of active entities.

	CComponentEventDistributer*			m_pEventDistributer;
	//////////////////////////////////////////////////////////////////////////

	// Entity timers.
	EntityTimersMap									m_timersMap;
	EntityTimersVector							m_currentTimers;
	bool														m_bTimersPause;
	CTimeValue											m_nStartPause;

	// Binding entity.
	CScriptBind_Entity *						m_pEntityScriptBinding;

	// Entity class registry.
	CEntityClassRegistry *					m_pClassRegistry;
	CPhysicsEventListener *					m_pPhysicsEventListener;

	CAreaManager *									m_pAreaManager;

	CEntityLoadManager *					m_pEntityLoadManager;
	CEntityPoolManager *					m_pEntityPoolManager;

	// There`s a map of entity id to event listeners for each event.
	typedef std::multimap<EntityId,IEntityEventListener*> EventListenersMap;
	EventListenersMap m_eventListeners[ENTITY_EVENT_LAST];

	typedef std::map<EntityGUID,EntityId> EntityGuidMap;
	EntityGuidMap m_guidMap;
	EntityGuidMap m_genIdMap;

	IBreakableManager* m_pBreakableManager;
	CEntityArchetypeManager *m_pEntityArchetypeManager;
	CGeomCacheAttachmentManager *m_pGeomCacheAttachmentManager;
	CCharacterBoneAttachmentManager *m_pCharacterBoneAttachmentManager;

	// Partition grid used by the entity system
	CPartitionGrid *m_pPartitionGrid;
	CProximityTriggerSystem* m_pProximityTriggerSystem;

	EntityId m_idForced;

	//don't spawn any entities without being forced to
	bool		m_bLocked;

	CEntityTimeoutList m_entityTimeoutList;

	friend class CEntityItMap;
	class CCompareEntityIdsByClass;

	// helper to satisfy GCC
	static IEntityClass * GetClassForEntity( CEntity * );

	typedef std::map<string, CEntityLayer*> TLayers;
	typedef std::vector<SEntityLayerGarbage> THeaps;

	TLayers m_layers;
	THeaps m_garbageLayerHeaps;
	bool m_bEntitiesUseGUIDs;
	int m_nGeneratedFromGuid;

	//////////////////////////////////////////////////////////////////////////
	// Pool Allocators.
	//////////////////////////////////////////////////////////////////////////
public:
	bool m_bReseting;
	//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_PROFILING_CODE
public:
	struct SLayerProfile
	{
		float fTimeMS;
		float fTimeOn;
		bool isEnable;
		CEntityLayer* pLayer;
	};

	std::vector<SLayerProfile> m_layerProfiles;
#endif //ENABLE_PROFILING_CODE
};

//////////////////////////////////////////////////////////////////////////
// Precache resources mode state.
//////////////////////////////////////////////////////////////////////////
extern bool gPrecacheResourcesMode;

#endif // __EntitySystem_h__
