/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2010.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Handles management for loading of entities
  
 -------------------------------------------------------------------------
  History:
  - 15:03:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ENTITYLOADMANAGER_H__
#define __ENTITYLOADMANAGER_H__

#include "EntitySystem.h"

class CEntityLoadManager
{
public:
	CEntityLoadManager(CEntitySystem *pEntitySystem);
	~CEntityLoadManager();

	void Reset();

	bool LoadEntities(XmlNodeRef &entitesNode, bool bIsLoadingLevelFile, const Vec3 &segmentOffset = Vec3(0, 0, 0), std::vector<IEntity *> *outGlobalEntityIds = 0, std::vector<IEntity *> *outLocalEntityIds = 0);

	void PrepareBatchCreation(int nSize);
	bool CreateEntity(SEntityLoadParams &loadParams, EntityId &outUsingId, bool bIsLoadingLevellFile);
	void OnBatchCreationCompleted();

	void GetMemoryStatistics(ICrySizer* pSizer) const
	{
		pSizer->Add(*this);
		pSizer->AddContainer(m_queuedAttachments);
		pSizer->AddContainer(m_queuedFlowgraphs);
	}

	bool	ExtractEntityLoadParams(XmlNodeRef &entityNode, SEntitySpawnParams &outLoadParams) const;
	bool	CreateEntity(XmlNodeRef &entityNode,SEntitySpawnParams &pParams,EntityId &outUsingId);

private:
	// Loading helpers
	bool ReserveEntityIds(XmlNodeRef &entityNode);
	bool CanParseEntity(XmlNodeRef &entityNode, std::vector<IEntity *> *outGlobalEntityIds);
	bool ParseEntities(XmlNodeRef &entityNode, bool bIsLoadingLevelFile, const Vec3 &segmentOffset, std::vector<IEntity *> *outGlobalEntityIds, std::vector<IEntity *> *outLocalEntityIds);

	bool ExtractEntityLoadParams(XmlNodeRef &entityNode, SEntityLoadParams &outLoadParams, const Vec3 &segmentOffset,bool bWarningMsg) const;

	// Batch creation helpers
	void AddQueuedAttachment(EntityId nParent, EntityGUID nParentGuid, EntityId nChild, const Vec3& pos, const Quat& rot, const Vec3& scale, bool guid, const int flags, const char *target);
	void AddQueuedFlowgraph(IEntity *pEntity, XmlNodeRef &pNode);
	void AddQueuedEntityLink(IEntity *pEntity, XmlNodeRef &pNode);

	void ResolveLinks();

	// Attachment queue for post Entity batch creation
	typedef std::vector<SEntityAttachment> TQueuedAttachments;
	TQueuedAttachments m_queuedAttachments;

	// Flowgraph queue for Flowgraph initiation at post Entity batch creation
	struct SQueuedFlowGraph
	{
		IEntity *pEntity;
		XmlNodeRef pNode;

		SQueuedFlowGraph() : pEntity(NULL), pNode() {}
	};
	typedef std::vector<SQueuedFlowGraph> TQueuedFlowgraphs;
	TQueuedFlowgraphs m_queuedFlowgraphs;

	TQueuedFlowgraphs m_queuedEntityLinks;
	typedef std::map<uint32, EntityId>	TGuidToId;
	TGuidToId m_guidToId;

	CEntitySystem *m_pEntitySystem;
	bool m_bSWLoading;
};

#endif //__ENTITYLOADMANAGER_H__
