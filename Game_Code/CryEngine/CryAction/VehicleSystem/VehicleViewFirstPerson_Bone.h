/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements the first person pit view for vehicles

-------------------------------------------------------------------------
History:
- 01:09:2011: Created by Richard Semmens

*************************************************************************/
#ifndef __VEHICLEVIEWFIRSTPERSON_BONE_H__
#define __VEHICLEVIEWFIRSTPERSON_BONE_H__

#include "VehicleViewFirstPerson.h"

class CVehicleViewFirstPerson_Bone
	: public CVehicleViewFirstPerson
{
	IMPLEMENT_VEHICLEOBJECT;

public:
  CVehicleViewFirstPerson_Bone();
	virtual ~CVehicleViewFirstPerson_Bone() {}

	virtual const char* GetName() { return m_name; }

	virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);

protected:
	virtual Quat GetWorldRotGoal();
	virtual Vec3 GetWorldPosGoal();

	int		m_MoveBoneId;
	Quat	m_additionalRotation;

	static const char* m_name;
};

#endif // __VEHICLEVIEWFIRSTPERSON_BONE_H__

