/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action for the steering wheel

-------------------------------------------------------------------------
History:
- 28:11:2005: Created by Mathieu Pinard
- 14:06:2012: Refactored to work with CryMannequin by Benito Gangoso Rodriguez

*************************************************************************/
#ifndef __VEHICLESEATACTIONSTEERINGWHEEL_H__
#define __VEHICLESEATACTIONSTEERINGWHEEL_H__

#include <SharedParams/ISharedParams.h>
#include "VehicleNoiseGenerator.h"

class CVehiclePartAnimatedJoint;
class CVehiclePartSubPartWheel;

class CVehicleSeatActionSteeringWheel
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT

private:
	enum ESteeringClass
	{
		eSC_Generic = 1,
		eSC_Car,
	};

public:

	CVehicleSeatActionSteeringWheel();
	virtual ~CVehicleSeatActionSteeringWheel() { Reset(); }

	// IVehicleSeatAction
	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset();
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId);
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
	
	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
  virtual void PostSerialize();
	virtual void Update(const float deltaTime);

	virtual void GetMemoryUsage(ICrySizer * s) const {s->Add(*this);}
	// ~IVehicleSeatAction

  virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

protected:
	
	BEGIN_SHARED_PARAMS(SSharedParams)
	
		ESteeringClass steeringClass;
		float steeringRelaxMult;
		float steeringForce;

		float jitterAmpLow;         // amplitude of jitter at low speed
		float jitterAmpHi;          // amplitude of jitter at high speed
		float jitterFreqLow;        // frequency at low speed
		float jitterFreqHi;         // frequency at high speed/high accel
		float jitterSuspAmp;        // max amplitude due to suspension changes
		float jitterSuspResponse;   // suspension response

	END_SHARED_PARAMS

	IVehicle* m_pVehicle;
	
	SSharedParamsConstPtr m_pSharedParams;

	// car based steering
	float m_wheelInvRotationMax;
	float m_currentSteering;

	// generic (non car) steering
	Vec3 m_steeringActions;
	Vec3 m_steeringValues;

	EntityId m_userId;
	bool m_isBeingUsed;
	bool m_isUsedSerialization;

	CVehicleNoiseValue m_jitter;
	float m_jitterOffset;
	float m_jitterSuspensionResponse;
	
};

#endif
