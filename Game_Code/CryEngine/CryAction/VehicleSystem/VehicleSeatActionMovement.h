/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action which handle the vehicle movement

-------------------------------------------------------------------------
History:
- 20:10:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLESEATACTIONMOVEMENT_H__
#define __VEHICLESEATACTIONMOVEMENT_H__

class CVehicleSeatActionMovement
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehicleSeatActionMovement();
	~CVehicleSeatActionMovement();

	// IVehicleSeatAction
	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat);
	virtual void Reset();
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId);
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params) {}

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {}
	virtual void PostSerialize() {}
	virtual void Update(const float deltaTime);

	virtual void GetMemoryUsage(ICrySizer * s) const  { s->Add(*this); }
	// ~IVehicleSeatAction

protected:

	IVehicle* m_pVehicle;
	IVehicleSeat* m_pSeat;

	float m_actionForward;
	float m_delayedStop;
};

#endif
