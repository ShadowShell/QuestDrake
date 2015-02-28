/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements the first person pit view for vehicles

-------------------------------------------------------------------------
History:
- 29:01:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEVIEWFIRSTPERSON_H__
#define __VEHICLEVIEWFIRSTPERSON_H__

#include "VehicleViewBase.h"

class CVehicleViewFirstPerson
	: public CVehicleViewBase
{
	IMPLEMENT_VEHICLEOBJECT;
public:

  CVehicleViewFirstPerson();
  virtual ~CVehicleViewFirstPerson() {}

	// IVehicleView
	virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
  virtual void Reset();

	virtual const char* GetName() { return m_name; }
	virtual bool IsThirdPerson() { return false; }

  virtual void OnStartUsing(EntityId playerId);
  virtual void OnStopUsing();  
  
	virtual void Update(const float frameTime);
	virtual void UpdateView(SViewParams &viewParams, EntityId playerId = 0);

	void GetMemoryUsage(ICrySizer * s) const;

	void OffsetPosition(const Vec3 &delta) {
#ifdef SEG_WORLD
		m_viewPosition += delta;
#endif
	}
	// ~IVehicleView

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	// ~IVehicleEventListener

protected:

	virtual Vec3 GetWorldPosGoal();
	virtual Quat GetWorldRotGoal();
	Quat GetVehicleRotGoal();

  void HideEntitySlots(IEntity* pEnt, bool hide);

	static const char* m_name;
	
  IVehicleHelper* m_pHelper;
	string m_sCharacterBoneName;
	Vec3 m_offset;  
	float m_fov;
  float m_relToHorizon;
  bool m_hideVehicle; 
  int m_frameSlot;
  Matrix34 m_invFrame;
  Vec3 m_frameObjectOffset;

  typedef std::multimap<EntityId, int> TSlots;
  TSlots m_slotFlags;
  
	Vec3 m_viewPosition;
	Quat m_viewRotation;

	float m_speedRot;

	EntityId m_passengerId;  
};

#endif

