/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action for sounds (ie: honk on trucks)

-------------------------------------------------------------------------
History:
- 16:11:2005: Created by Mathieu Pinard

*************************************************************************/

#ifndef __VEHICLESEATACTIONSOUND_H__
#define __VEHICLESEATACTIONSOUND_H__

class CVehicleSeatActionSound
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehicleSeatActionSound();

	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset() {}
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId) {}
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
  virtual void PostSerialize(){}
	virtual void Update(const float deltaTime) {};

	virtual void ExecuteTrigger(const TAudioControlID& controlID);
	virtual void StopTrigger();

  virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

	virtual void GetMemoryUsage(ICrySizer * s) const;

protected:

	IVehicle* m_pVehicle;
	IVehicleHelper* m_pHelper;
	CVehicleSeat* m_pSeat;

	bool m_enabled;

	TAudioControlID m_nAudioControlIDStart;
	TAudioControlID m_nAudioControlIDStop;
};

#endif
