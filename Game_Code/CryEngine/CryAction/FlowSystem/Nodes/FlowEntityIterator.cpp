// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "FlowBaseNode.h"
#include <IEntitySystem.h>
#include <IActorSystem.h>
#include <IVehicleSystem.h>
#include <IItemSystem.h>


#define ADD_BASE_INPUTS() \
	InputPortConfig_Void("Start", _HELP("Call to start iterating through all entities inside the defined sphere")), \
	InputPortConfig_Void("Next", _HELP("Call to get next entity found")), \
	InputPortConfig<int>("Limit", 0, _HELP("Limit how many entities are returned. Use 0 to get all entities")), \
	InputPortConfig<bool>("Immediate", false, _HELP("TRUE to iterate immediately through results, without having to call Next"))

#define ADD_BASE_OUTPUTS() \
	OutputPortConfig<EntityId>("Entity", _HELP("Called each time an entity is found, with the Id of the entity returned")), \
	OutputPortConfig<int>("Count", _HELP("Called each time an entity is found, with the current count returned")), \
	OutputPortConfig<int>("Done", _HELP("Called when all entities have been found, with the total count returned"))


enum EEntityType
{
	eET_Unknown = 0x00,
	eET_Valid = 0x01,
	eET_AI = 0x02,
	eET_Actor = 0x04,
	eET_Vehicle = 0x08,
	eET_Item = 0x10,
};

#define ENTITY_TYPE_ENUM ("enum_int:All=0,AI=1,Actor=2,Vehicle=3,Item=4,Other=5")
bool IsValidType(int requested, const EEntityType &type)
{
	bool bValid = false;

	if (requested == 0)
	{
		bValid = (type&eET_Valid)==eET_Valid;
	}
	else if (requested == 1)
	{
		bValid = (type&eET_AI)==eET_AI;
	}
	else if (requested == 2)
	{
		bValid = (type&eET_Actor)==eET_Actor;
	}
	else if (requested == 3)
	{
		bValid = (type&eET_Vehicle)==eET_Vehicle;
	}
	else if (requested == 4)
	{
		bValid = (type&eET_Item)==eET_Item;
	}
	else if (requested == 5)
	{
		bValid = (type==eET_Valid);
	}

	return bValid;
}

EEntityType GetEntityType(EntityId id)
{
	int type = eET_Unknown;

	IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
	if (pEntitySystem)
	{
		IEntity *pEntity = pEntitySystem->GetEntity(id);
		if (pEntity)
		{
			type = eET_Valid;

			IEntityClass *pClass = pEntity->GetClass();
			if (pClass)
			{
				const char* className = pClass->GetName();

				// Check AI
				if (pEntity->GetAI())
				{
					type |= eET_AI;
				}
				
				// Check actor
				IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
				if (pActorSystem)
				{
					IActor *pActor = pActorSystem->GetActor(id);
					if (pActor)
					{
						type |= eET_Actor;
					}
				}

				// Check vehicle
				IVehicleSystem *pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
				if (pVehicleSystem)
				{
					if (pVehicleSystem->IsVehicleClass(className))
					{
						type |= eET_Vehicle;
					}
				}

				// Check item
				IItemSystem *pItemSystem = gEnv->pGame->GetIGameFramework()->GetIItemSystem();
				if (pItemSystem)
				{
					if (pItemSystem->IsItemClass(className))
					{
						type |= eET_Item;
					}
				}
			}
		}
	}

	return (EEntityType)type;
}


class CFlowBaseIteratorNode : public CFlowBaseNode<eNCT_Instanced>
{
protected:
	enum EInputPorts
	{
		EIP_Start,
		EIP_Next,
		EIP_Limit,
		EIP_Immediate,

		EIP_CustomStart,
	};

	enum EOutputPorts
	{
		EOP_EntityId,
		EOP_Count,
		EOP_Done,

		EOP_CustomStart,
	};

	typedef std::list<EntityId> IterList;
	IterList m_List;
	IterList::iterator m_ListIter;
	int m_Count;
	int m_Iter;

public:
	virtual bool GetCustomConfiguration(SFlowNodeConfig& config) { return false; }
	virtual void OnIterStart(SActivationInfo *pActInfo) {}
	virtual void OnIterNext(SActivationInfo *pActInfo) {}

public:

	CFlowBaseIteratorNode(SActivationInfo *pActInfo)
	{
		m_Count = 0;
		m_Iter = 0;
	}

	virtual ~CFlowBaseIteratorNode()
	{
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		// Define input ports here, in same order as EInputPorts
		static const SInputPortConfig inputs[] =
		{
			ADD_BASE_INPUTS(),
			{0}
		};

		// Define output ports here, in same oreder as EOutputPorts
		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			{0}
		};

		// Fill in configuration
		if (!GetCustomConfiguration(config))
		{
			config.pInputPorts = inputs;
			config.pOutputPorts = outputs;
			config.sDescription = _HELP("Base iterator");
			config.SetCategory(EFLN_APPROVED);
		}
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		switch (event)
		{
			case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Start))
				{
					m_List.clear();
					m_ListIter = m_List.end();
					m_Iter = 0;
					m_Count = 0;

					OnIterStart(pActInfo);
					m_ListIter = m_List.begin();

					if (m_Count <= 0)
					{
						// None found
						ActivateOutput(pActInfo, EOP_Done, 0);
					}
					else
					{
						const bool bImmediate = GetPortBool(pActInfo, EIP_Immediate);
						const bool bFirstResult = SendNext(pActInfo);
						if (bImmediate && bFirstResult)
						{
							pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
						}
					}
				}
				else if (IsPortActive(pActInfo, EIP_Next))
				{
					const bool bImmediate = GetPortBool(pActInfo, EIP_Immediate);
					if (!bImmediate)
					{
						SendNext(pActInfo);
					}
				}
			}
			break;

			case eFE_Update:
			{
				if (!SendNext(pActInfo))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				}

			}
			break;
		}
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowBaseIteratorNode(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer *s) const
	{
		s->Add(*this);
	}

	void AddEntity(EntityId id)
	{
		m_List.push_back(id);
		++m_Count;
	}

	bool SendNext(SActivationInfo *pActInfo)
	{
		bool bResult = false;

		if (m_Count > 0)
		{
			if (m_Iter < m_Count)
			{
				const int limit = GetPortInt(pActInfo, EIP_Limit);
				if (limit == 0 || m_Iter <= limit)
				{
					OnIterNext(pActInfo);

					EntityId id = *m_ListIter;
					ActivateOutput(pActInfo, EOP_EntityId, id);
					ActivateOutput(pActInfo, EOP_Count, m_Iter);

					++m_Iter;
					++m_ListIter;

					bResult = true;
				}
			}
		}

		if (!bResult)
		{
			ActivateOutput(pActInfo, EOP_Done, m_Iter);
		}

		return bResult;
	}
};


class CFlowNode_GetEntitiesInSphere : public CFlowBaseIteratorNode
{
	enum ECustomInputPorts
	{
		EIP_Type = EIP_CustomStart,
		EIP_Pos,
		EIP_Range,
	};

public:
	CFlowNode_GetEntitiesInSphere(SActivationInfo *pActInfo) : CFlowBaseIteratorNode(pActInfo)
	{
	}

	virtual ~CFlowNode_GetEntitiesInSphere()
	{
	}

	virtual bool GetCustomConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			ADD_BASE_INPUTS(),
			InputPortConfig<int>("Type", 0, _HELP("Type of entity to iterate"), 0, _UICONFIG(ENTITY_TYPE_ENUM)),
			InputPortConfig<Vec3>("Center", _HELP("Center point of sphere")),
			InputPortConfig<float>("Range", 0.f, _HELP("Range i.e., radius of sphere - Distance from center to check for entities")),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Finds and returns all entities that are inside the defined sphere");
		config.SetCategory(EFLN_APPROVED);

		return true;
	}

	virtual void OnIterStart(SActivationInfo *pActInfo)
	{
		const int type = GetPortInt(pActInfo, EIP_Type);
		const Vec3& center(GetPortVec3(pActInfo, EIP_Pos));
		const float range = GetPortFloat(pActInfo, EIP_Range);
		const float rangeSq = range * range;

		const Vec3 min(center.x-range, center.y-range, center.z-range);
		const Vec3 max(center.x+range, center.y+range, center.z+range);

		IPhysicalWorld *pWorld = gEnv->pPhysicalWorld;
		IPhysicalEntity **ppList = NULL;
		int	numEnts = pWorld->GetEntitiesInBox(min,max,ppList,ent_all);
		for (int i = 0; i < numEnts; ++i)
		{
			const EntityId id = pWorld->GetPhysicalEntityId(ppList[i]);
			const EEntityType entityType = GetEntityType(id);
			if (IsValidType(type, entityType))
			{
				AddEntity(id);
			}
		}
	}

	virtual void OnIterNext(SActivationInfo *pActInfo)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowNode_GetEntitiesInSphere(pActInfo);
	}
};

class CFlowNode_GetEntitiesInBox : public CFlowBaseIteratorNode
{
	enum ECustomInputPorts
	{
		EIP_Type = EIP_CustomStart,
		EIP_Min,
		EIP_Max,
	};

public:
	CFlowNode_GetEntitiesInBox(SActivationInfo *pActInfo) : CFlowBaseIteratorNode(pActInfo)
	{
	}

	virtual ~CFlowNode_GetEntitiesInBox()
	{
	}

	virtual bool GetCustomConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			ADD_BASE_INPUTS(),
			InputPortConfig<int>("Type", 0, _HELP("Type of entity to iterate"), 0, _UICONFIG(ENTITY_TYPE_ENUM)),
			InputPortConfig<Vec3>("Min", _HELP("AABB min")),
			InputPortConfig<Vec3>("Max", _HELP("AABB max")),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Finds and returns all entities that are inside the defined AABB");
		config.SetCategory(EFLN_APPROVED);

		return true;
	}

	virtual void OnIterStart(SActivationInfo *pActInfo)
	{
		const int type = GetPortInt(pActInfo, EIP_Type);
		const Vec3& min(GetPortVec3(pActInfo, EIP_Min));
		const Vec3& max(GetPortVec3(pActInfo, EIP_Max));

		IPhysicalWorld *pWorld = gEnv->pPhysicalWorld;
		IPhysicalEntity **ppList = NULL;
		int	numEnts = pWorld->GetEntitiesInBox(min,max,ppList,ent_all);
		for (int i = 0; i < numEnts; ++i)
		{
			const EntityId id = pWorld->GetPhysicalEntityId(ppList[i]);
			const EEntityType entityType = GetEntityType(id);
			if (IsValidType(type, entityType))
			{
				AddEntity(id);
			}
		}
	}

	virtual void OnIterNext(SActivationInfo *pActInfo)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowNode_GetEntitiesInBox(pActInfo);
	}
};


class CFlowNode_GetEntitiesInArea : public CFlowBaseIteratorNode
{
	enum ECustomInputPorts
	{
		EIP_Type = EIP_CustomStart,
		EIP_Area,
	};

public:
	CFlowNode_GetEntitiesInArea(SActivationInfo *pActInfo) : CFlowBaseIteratorNode(pActInfo)
	{
	}

	virtual ~CFlowNode_GetEntitiesInArea()
	{
	}

	virtual bool GetCustomConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			ADD_BASE_INPUTS(),
			InputPortConfig<int>("Type", 0, _HELP("Type of entity to iterate"), 0, _UICONFIG(ENTITY_TYPE_ENUM)),
			InputPortConfig<string>("Area", _HELP("Name of area shape"), 0, 0),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Finds and returns all entities that are inside the area shape with the given name");
		config.SetCategory(EFLN_APPROVED);

		return true;
	}

	virtual void OnIterStart(SActivationInfo *pActInfo)
	{
		const int type = GetPortInt(pActInfo, EIP_Type);
		const char* area = GetPortString(pActInfo, EIP_Area);

		// Find the entity
		IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
		if (pEntitySystem)
		{
			IEntity *pArea = pEntitySystem->FindEntityByName(area);
			if (pArea)
			{
				IEntityAreaProxy *pAreaProxy = (IEntityAreaProxy*)pArea->GetProxy(ENTITY_PROXY_AREA);
				if (pAreaProxy)
				{
					Vec3 min, max, worldPos(pArea->GetWorldPos());
					min.Set(0.f,0.f,0.f);
					max.Set(0.f,0.f,0.f);
					EEntityAreaType areaType = pAreaProxy->GetAreaType();

					// Construct bounding space around area
					switch (areaType)
					{
						case ENTITY_AREA_TYPE_BOX:
						{
							pAreaProxy->GetBox(min, max);
							min += worldPos;
							max += worldPos;
						}
						break;
						case ENTITY_AREA_TYPE_SPHERE:
						{
							Vec3 center;
							float radius = 0.f;
							pAreaProxy->GetSphere(center, radius);
							
							min.Set(center.x-radius, center.y-radius, center.z-radius);
							max.Set(center.x+radius, center.y+radius, center.z+radius);
						}
						break;
						case ENTITY_AREA_TYPE_SHAPE:
						{
							const Vec3 *points = pAreaProxy->GetPoints();
							const int count = pAreaProxy->GetPointsCount();
							if (count > 0)
							{
								Vec3 p = worldPos + points[0];
								min = p;
								max = p;
								for (int i = 1; i < count; ++i)
								{
									p = worldPos + points[i];
									if (p.x < min.x) min.x = p.x;
									if (p.y < min.y) min.y = p.y;
									if (p.z < min.z) min.z = p.z;
									if (p.x > max.x) max.x = p.x;
									if (p.y > max.y) max.y = p.y;
									if (p.z > max.z) max.z = p.z;
								}
							}
						}
						break;
					}

					IPhysicalWorld *pWorld = gEnv->pPhysicalWorld;
					IPhysicalEntity **ppList = NULL;
					int	numEnts = pWorld->GetEntitiesInBox(min,max,ppList,ent_all);
					for (int i = 0; i < numEnts; ++i)
					{
						const EntityId id = pWorld->GetPhysicalEntityId(ppList[i]);
						const EEntityType entityType = GetEntityType(id);
						if (IsValidType(type, entityType))
						{
							// Sanity check - Test entity's position
							IEntity *pEntity = pEntitySystem->GetEntity(id);
							if (pEntity && pAreaProxy->CalcPointWithin(id, pEntity->GetWorldPos(), pAreaProxy->GetHeight()==0))
							{
								AddEntity(id);
							}
						}
					}
				}
			}
		}
	}

	virtual void OnIterNext(SActivationInfo *pActInfo)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowNode_GetEntitiesInArea(pActInfo);
	}
};


class CFlowNode_GetEntities : public CFlowBaseIteratorNode
{
	enum ECustomInputPorts
	{
		EIP_Type = EIP_CustomStart,
	};

public:
	CFlowNode_GetEntities(SActivationInfo *pActInfo) : CFlowBaseIteratorNode(pActInfo)
	{
	}

	virtual ~CFlowNode_GetEntities()
	{
	}

	virtual bool GetCustomConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			ADD_BASE_INPUTS(),
			InputPortConfig<int>("Type", 0, _HELP("Type of entity to iterate"), 0, _UICONFIG(ENTITY_TYPE_ENUM)),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Finds and returns all entities in the world");
		config.SetCategory(EFLN_APPROVED);

		return true;
	}

	virtual void OnIterStart(SActivationInfo *pActInfo)
	{
		const int type = GetPortInt(pActInfo, EIP_Type);

		IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
		if (pEntitySystem)
		{
			IEntityItPtr iter = pEntitySystem->GetEntityIterator();
			if (iter)
			{
				iter->MoveFirst();
				IEntity *pEntity = NULL;
				while (!iter->IsEnd())
				{
					pEntity = iter->Next();
					if (pEntity)
					{
						const EntityId id = pEntity->GetId();
						const EEntityType entityType = GetEntityType(id);
						if (IsValidType(type, entityType))
						{
							AddEntity(id);
						}
					}
				}
			}
		}
	}

	virtual void OnIterNext(SActivationInfo *pActInfo)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowNode_GetEntities(pActInfo);
	}
};


REGISTER_FLOW_NODE("Iterator:GetEntitiesInSphere", CFlowNode_GetEntitiesInSphere);
REGISTER_FLOW_NODE("Iterator:GetEntitiesInBox", CFlowNode_GetEntitiesInBox);
REGISTER_FLOW_NODE("Iterator:GetEntitiesInArea", CFlowNode_GetEntitiesInArea);
REGISTER_FLOW_NODE("Iterator:GetEntities", CFlowNode_GetEntities);
