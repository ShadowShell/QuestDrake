/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Vehicle part class that spawns an entity and attaches it to the vehicle.

-------------------------------------------------------------------------
History:
- 01:09:2010: Created by Paul Slinger

*************************************************************************/

#ifndef __VEHICLEPARTENTITY_H__
#define __VEHICLEPARTENTITY_H__

#include "IGameObject.h"
#include "VehiclePartBase.h"

class CVehiclePartEntity : public CVehiclePartBase
{
public:

	IMPLEMENT_VEHICLEOBJECT

	CVehiclePartEntity();

	~CVehiclePartEntity();

	virtual bool Init(IVehicle *pVehicle, const CVehicleParams &table, IVehiclePart *parent, CVehicle::SPartInitInfo &initInfo, int partType);
	virtual void PostInit();
	virtual void Reset();
	virtual void Update(const float frameTime);
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams &params);

	void TryDetachPart( const SVehicleEventParams & params );

	virtual void Serialize(TSerialize serializer, EEntityAspects aspects);
	virtual void PostSerialize();
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;

	IEntity *GetPartEntity() const;

protected:

	bool EntityAttached() const { return m_entityAttached; }
	EntityId GetPartEntityId() const { return m_entityId; }

private:

	struct Flags { enum
	{
		Hidden					= 1,
		EntityAttached			= 2,
		Destroyed				= 4,

	}; };

	void UpdateEntity();
	void DestroyEntity();

	void SafeRemoveBuddyConstraint();

	int							m_index;
	uint						m_constraintId;

	string					m_entityName, m_entityArchetype, m_helperName;

	EntityId				m_entityId;
	IVehicleHelper*	m_pHelper;
	IEntityLink*		m_pLink;
	uint16					m_entityNetId;

	bool						m_hidden : 1;
	bool						m_entityAttached : 1;
	bool                        m_destroyed : 1;
	bool						m_CollideWithParent : 1;
	bool						m_createBuddyConstraint : 1;
	bool						m_shouldDetachInsteadOfDestroy : 1;	
};

#endif //__VEHICLEPARTENTITY_H__
