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
#include "StdAfx.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include "ICryAnimation.h"
#include "IViewSystem.h"
#include "IVehicleSystem.h"
#include "VehicleViewFirstPerson_Bone.h"
#include "VehicleSeat.h"


const char* CVehicleViewFirstPerson_Bone::m_name = "FirstPersonBone";

//------------------------------------------------------------------------
CVehicleViewFirstPerson_Bone::CVehicleViewFirstPerson_Bone()
: CVehicleViewFirstPerson()
,	m_MoveBoneId(-1)
{	

}

//------------------------------------------------------------------------
bool CVehicleViewFirstPerson_Bone::Init(IVehicleSeat* pSeat, const CVehicleParams& table)
{
	if(!CVehicleViewFirstPerson::Init(pSeat, table))
		return false;

	IDefaultSkeleton* pIDefaultSkeleton = NULL;
	if (ICharacterInstance* pCharacterInstance = m_pVehicle->GetEntity()->GetCharacter(0))  
	{
		pIDefaultSkeleton = &pCharacterInstance->GetIDefaultSkeleton();
	}
	
	if(pIDefaultSkeleton)
	{
		if(table.haveAttr("MoveBone") )
		{
			const char* boneName = table.getAttr("MoveBone");
			m_MoveBoneId = pIDefaultSkeleton->GetJointIDByName(boneName);
		}

		if (CVehicleParams paramsTable = table.findChild(m_name))
		{
			Vec3 angles;
			paramsTable.getAttr("rotation", angles);
			angles *= gf_PI/180.0f;

			m_additionalRotation.SetRotationXYZ((Ang3)angles);

			paramsTable.getAttr("offset", m_offset);
			paramsTable.getAttr("speedRot", m_speedRot);
		}
	}
	else
	{
		CryFatalError("Creating first person bone-attached camera on a vehicle with no skeleton. Check your vehicle implementation XMLs");
	}

	return true;
}

//------------------------------------------------------------------------
Quat CVehicleViewFirstPerson_Bone::GetWorldRotGoal()
{
	Quat worldRot = m_pVehicle->GetEntity()->GetWorldRotation();
	
	if (ICharacterInstance* pCharacterInstance = m_pVehicle->GetEntity()->GetCharacter(0))  
	{
		ISkeletonPose * pSkeletonPose = pCharacterInstance->GetISkeletonPose();
		if(pSkeletonPose)
		{
			worldRot = (pSkeletonPose->GetAbsJointByID(m_MoveBoneId) * m_additionalRotation * worldRot).q;
		}
	}
	else
		CryFatalError("No character instance present when using a bone-based camera. Check your vehicle implementations XML");

	return worldRot;
}

//------------------------------------------------------------------------
Vec3 CVehicleViewFirstPerson_Bone::GetWorldPosGoal()
{
	Vec3 worldPos(ZERO);

	if (ICharacterInstance* pCharacterInstance = m_pVehicle->GetEntity()->GetCharacter(0))  
	{
		ISkeletonPose * pSkeletonPose = pCharacterInstance->GetISkeletonPose();
		if(pSkeletonPose)
		{
			QuatT worldRot = QuatT(m_pVehicle->GetEntity()->GetWorldTM());
			worldPos = (worldRot * pSkeletonPose->GetAbsJointByID(m_MoveBoneId) * m_additionalRotation) * m_offset;
		}
	}
	else
		CryFatalError("No character instance present when using a bone-based camera. Check your vehicle implementations XML");

	return worldPos;
}



DEFINE_VEHICLEOBJECT(CVehicleViewFirstPerson_Bone);
