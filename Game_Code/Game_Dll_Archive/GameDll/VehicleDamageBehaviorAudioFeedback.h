/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a damage behavior that plays sounds when the
vehicle is hit (different for first and third person)

-------------------------------------------------------------------------
History:
- 1:12:2011: Created by Ben Parbury

*************************************************************************/
#ifndef __VEHICLE_DAMAGE_BEHAVIOR_AUDIO_FEEDBACK_H__
#define __VEHICLE_DAMAGE_BEHAVIOR_AUDIO_FEEDBACK_H__

#include "Audio/AudioTypes.h"
#include "IVehicleSystem.h"

class CVehicleDamageBehaviorAudioFeedback
	: public IVehicleDamageBehavior
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehicleDamageBehaviorAudioFeedback();
	virtual ~CVehicleDamageBehaviorAudioFeedback();

	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
	virtual void Reset() {}
	virtual void Release() { delete this; }

	virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams) {};

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
	virtual void Update(const float deltaTime) {}
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

	virtual void GetMemoryUsage(ICrySizer * s) const { s->Add(*this); }

protected:
	TAudioSignalID GetSignal() const;

	TAudioSignalID m_firstPersonSignal;
	TAudioSignalID m_thirdPersonSignal;
	EntityId m_vehicleId;
};

#endif //__VEHICLE_DAMAGE_BEHAVIOR_AUDIO_FEEDBACK_H__
