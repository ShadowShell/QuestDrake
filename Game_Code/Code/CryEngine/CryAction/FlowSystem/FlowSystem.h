// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __FLOWSYSTEM_H__
#define __FLOWSYSTEM_H__

#pragma once

#include "IFlowSystem.h"
#include "FlowSystemCVars.h"
#include "CryListenerSet.h"

class CFlowGraphBase;
class CFlowGraphModuleManager;

struct CFlowSystemContainer : public IFlowSystemContainer
{
	VIRTUAL void AddItem(TFlowInputData item);
	VIRTUAL void AddItemUnique(TFlowInputData item);

	VIRTUAL void RemoveItem(TFlowInputData item);

	VIRTUAL TFlowInputData GetItem(int i);

	VIRTUAL void RemoveItemAt(int i);
	VIRTUAL int GetItemCount() const;

	VIRTUAL void Clear();

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

	VIRTUAL void Serialize(TSerialize ser);
private:
	DynArray<TFlowInputData> m_container;
};

class CFlowSystem : public IFlowSystem, public IEntitySystemSink
{
public:
	CFlowSystem();
	~CFlowSystem();
	
	// IFlowSystem
	virtual void Release();
	VIRTUAL void Update();
	VIRTUAL void Reset(bool unload);
	VIRTUAL void ReloadAllNodeTypes();
	VIRTUAL IFlowGraphPtr CreateFlowGraph();
	VIRTUAL TFlowNodeTypeId RegisterType( const char *type, IFlowNodeFactoryPtr factory );
	VIRTUAL bool UnregisterType(const char* typeName);
	VIRTUAL const char* GetTypeName( TFlowNodeTypeId typeId );
	VIRTUAL TFlowNodeTypeId GetTypeId( const char* typeName );
	VIRTUAL IFlowNodeTypeIteratorPtr CreateNodeTypeIterator();
	VIRTUAL void RegisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
	VIRTUAL void UnregisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
	VIRTUAL void EnableInspecting(bool bEnable) { m_bInspectingEnabled = bEnable; }
	VIRTUAL bool IsInspectingEnabled() const { return m_bInspectingEnabled; }
	VIRTUAL IFlowGraphInspectorPtr GetDefaultInspector() const { return m_pDefaultInspector; }
	VIRTUAL IFlowGraph* GetGraphById(TFlowGraphId graphId);
	VIRTUAL void OnEntityIdChanged( EntityId oldId, EntityId newId );
	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;


	VIRTUAL bool CreateContainer(TFlowSystemContainerId id);
	VIRTUAL void DeleteContainer(TFlowSystemContainerId id);	
	VIRTUAL IFlowSystemContainerPtr GetContainer(TFlowSystemContainerId id);
	
	VIRTUAL void Serialize(TSerialize ser);
	// ~IFlowSystem

	// TODO Make a single point of entry for this and the AIProxyManager to share?
	// IEntitySystemSink
	VIRTUAL bool OnBeforeSpawn( SEntitySpawnParams &params ) { return true; }
	VIRTUAL void OnSpawn( IEntity *pEntity,SEntitySpawnParams &params );
	VIRTUAL bool OnRemove( IEntity *pEntity ) { return true; }
	VIRTUAL void OnReused( IEntity *pEntity, SEntitySpawnParams &params );
	VIRTUAL void OnEvent( IEntity *pEntity, SEntityEvent &event ) { }
	//~IEntitySystemSink

	IFlowNodePtr CreateNodeOfType( IFlowNode::SActivationInfo *, TFlowNodeTypeId typeId );
	void NotifyCriticalLoadingError() { m_criticalLoadingErrorHappened = true; }

	void PreInit();
	void Init();
	void Shutdown();
	void Enable(bool enable){m_cVars.m_enableUpdates = enable;}

	TFlowGraphId RegisterGraph( CFlowGraphBase *pGraph, const char* debugName );
	void UnregisterGraph( CFlowGraphBase *pGraph );

	CFlowGraphModuleManager* GetModuleManager();
	const CFlowGraphModuleManager* GetModuleManager() const;

	IFlowGraphModuleManager* GetIModuleManager();

	// resembles IFlowGraphInspector currently
	void NotifyFlow(CFlowGraphBase *pGraph, const SFlowAddress from, const SFlowAddress to);
	void NotifyProcessEvent(CFlowGraphBase *pGraph, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo, IFlowNode *pImpl);

	struct STypeInfo
	{
		STypeInfo() : name(""), factory(NULL) {}
		STypeInfo(const string& typeName, IFlowNodeFactoryPtr pFactory) : name(typeName), factory(pFactory) {}
		string name;
		IFlowNodeFactoryPtr factory;

		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(name);
		}
	};
	
#ifndef _RELEASE
	struct TSFGProfile
	{
		int graphsUpdated;
		int nodeActivations;
		int nodeUpdates;

		void Reset()
		{
			memset( this, 0, sizeof(*this) );
		}
	};
	static TSFGProfile FGProfile;
#endif //_RELEASE

	const STypeInfo& GetTypeInfo(TFlowNodeTypeId typeId) const;

private:
	typedef std::queue<XmlNodeRef> TPendingComposites;

	void LoadExtensions( string path );
	void LoadExtensionFromXml( XmlNodeRef xml, TPendingComposites * pComposites );
	void LoadComposites( TPendingComposites * pComposites );

	void RegisterAllNodeTypes();
	void RegisterAutoTypes();
	void RegisterEntityTypes();
	TFlowNodeTypeId GenerateNodeTypeID();

	bool BlacklistedFlownode(const char **editorName);

	void UpdateGraphs();

private:
	// Inspecting enabled/disabled
	bool m_bInspectingEnabled;
	bool m_needToUpdateForwardings;
	bool m_criticalLoadingErrorHappened;

	class CNodeTypeIterator;
	typedef std::map<string, TFlowNodeTypeId> TTypeNameToIdMap;
	TTypeNameToIdMap m_typeNameToIdMap;
	std::vector<STypeInfo> m_typeRegistryVec; // 0 is invalid
	typedef CListenerSet<CFlowGraphBase*> TGraphs;
	TGraphs m_graphs;
	std::vector<IFlowGraphInspectorPtr> m_systemInspectors; // only inspectors which watch all graphs

	std::vector<TFlowNodeTypeId> m_freeNodeTypeIDs;
	TFlowNodeTypeId m_nextNodeTypeID;
	IFlowGraphInspectorPtr m_pDefaultInspector;

	CFlowSystemCVars m_cVars;

	TFlowGraphId m_nextFlowGraphId;

	CFlowGraphModuleManager* m_pModuleManager;

	typedef std::map<TFlowSystemContainerId, IFlowSystemContainerPtr> TFlowSystemContainerMap;
	TFlowSystemContainerMap m_FlowSystemContainers;
};

inline CFlowGraphModuleManager* CFlowSystem::GetModuleManager()
{
	return m_pModuleManager;
}

////////////////////////////////////////////////////
inline const CFlowGraphModuleManager* CFlowSystem::GetModuleManager() const
{
	return m_pModuleManager;
}


#endif
