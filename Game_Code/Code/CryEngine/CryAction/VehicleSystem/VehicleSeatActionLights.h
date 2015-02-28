/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action to head/spot lights

-------------------------------------------------------------------------
History:
- 01:03:2006: Created by Mathieu Pinard
- 20:07:2010: Refactored by Paul Slinger

*************************************************************************/
#ifndef __VEHICLESEATACTIONLIGHTS_H__
#define __VEHICLESEATACTIONLIGHTS_H__

#include "SharedParams/ISharedParams.h"

class CVehiclePartLight;

class CVehicleSeatActionLights : public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT

public:
	~CVehicleSeatActionLights();

	// IVehicleSeatAction

	virtual bool Init(IVehicle *pVehicle, IVehicleSeat* pSeat, const CVehicleParams &table);
	virtual void Reset();
	virtual void Release() { delete this; }
	virtual void StartUsing(EntityId passengerId) {}
	virtual void ForceUsage() {};
	virtual void StopUsing() {}
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;

	// ~IVehicleSeatAction

	// IVehicleObject

	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
  virtual void PostSerialize() {}
	virtual void Update(const float deltaTime) {}

	// IVehicleObject

	// IVehicleEventListener
  
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams &params);

	// ~IVehicleEventListener

protected:

  enum ELightActivation
  {
    eLA_Toggle = 0,
    eLA_Brake,
		eLA_Reversing
  };

	BEGIN_SHARED_PARAMS(SSharedParams)

		TVehicleSeatId		seatId;

		ELightActivation	activation;

		string						onSound, offSound;

	END_SHARED_PARAMS

  struct SLightPart
  {
		inline SLightPart() : pPart(NULL)
		{
		}

		inline SLightPart(CVehiclePartLight *pPart) : pPart(pPart)
		{
		}

		CVehiclePartLight	*pPart;
    
		void GetMemoryUsage(ICrySizer *pSizer) const {}
  };

	typedef std::vector<SLightPart> TVehiclePartLightVector;

	void ToggleLights(bool enable);
	void PlaySound(const string &name);

	IVehicle								*m_pVehicle;
	
	SSharedParamsConstPtr		m_pSharedParams;

	TVehiclePartLightVector	m_lightParts;

	bool										m_enabled;
};

#endif //__VEHICLESEATACTIONLIGHTS_H__
