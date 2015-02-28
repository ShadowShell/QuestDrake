/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a damage behavior which send an AI signal

-------------------------------------------------------------------------
History:
- 23:11:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEDAMAGEBEHAVIORAISIGNAL_H__
#define __VEHICLEDAMAGEBEHAVIORAISIGNAL_H__

class CVehicle;

class CVehicleDamageBehaviorAISignal
	: public IVehicleDamageBehavior
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehicleDamageBehaviorAISignal() {}
	virtual ~CVehicleDamageBehaviorAISignal() {}

	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
	virtual void Reset();
	virtual void Release() { delete this; }

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
	virtual void Update(const float deltaTime);

	virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams& behaviorParams);
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}
	
	virtual void GetMemoryUsage(ICrySizer * s) const;
protected:

	void ActivateUpdate(bool activate);

	IVehicle* m_pVehicle;
	
	bool m_freeSignalRepeat;
	bool m_isActive;
	int m_signalId;
	float m_freeSignalRadius;
	float m_timeCounter;
	string m_signalText;
	string m_freeSignalText;
};

#endif
