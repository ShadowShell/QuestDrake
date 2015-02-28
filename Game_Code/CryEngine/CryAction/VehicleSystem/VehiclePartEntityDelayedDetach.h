/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2012.
-------------------------------------------------------------------------

Description: Subclass of VehiclePartEntity that can be asked to detach at a random point in the future

-------------------------------------------------------------------------
History:
- 14:02:2012:	Created by Andrew Blackwell

*************************************************************************/

#ifndef __VEHICLEPARTENTITYDELAYEDDETACH_H__
#define __VEHICLEPARTENTITYDELAYEDDETACH_H__

//Base Class include
#include "VehiclePartEntity.h"

class CVehiclePartEntityDelayedDetach : public CVehiclePartEntity
{
public:
	IMPLEMENT_VEHICLEOBJECT

	CVehiclePartEntityDelayedDetach();
	virtual ~CVehiclePartEntityDelayedDetach();
	
	virtual void Update( const float frameTime );
	virtual void OnVehicleEvent( EVehicleEvent event, const SVehicleEventParams &params );
	
private:
	float						m_detachTimer;
};

#endif //__VEHICLEPARTENTITYDELAYEDDETACH_H__
