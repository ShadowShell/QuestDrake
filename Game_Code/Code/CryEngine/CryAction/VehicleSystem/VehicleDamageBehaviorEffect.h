/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements an effect damage behavior

-------------------------------------------------------------------------
History:
- 13:10:2005: Created by Mathieu Pinard
- 20:07:2010: Refactored by Paul Slinger

*************************************************************************/
#ifndef __VEHICLEDAMAGEBEHAVIOREFFECT_H__
#define __VEHICLEDAMAGEBEHAVIOREFFECT_H__

#include "SharedParams/ISharedParams.h"

class CVehicleDamageBehaviorEffect : public IVehicleDamageBehavior
{
	IMPLEMENT_VEHICLEOBJECT

public:

	// IVehicleDamageBehavior

	virtual bool Init(IVehicle *pVehicle, const CVehicleParams &table);
	virtual void Reset();
	virtual void Release() { delete this; }
	virtual void OnDamageEvent(EVehicleDamageBehaviorEvent event, const SVehicleDamageBehaviorEventParams &behaviorParams);
	virtual void GetMemoryUsage(ICrySizer *pSizer) const;

	// ~IVehicleDamageBehavior

	// IVehicleObject

	virtual void Serialize(TSerialize ser, EEntityAspects aspects);
	virtual void Update(const float deltaTime);

	// ~IVehicleObject

	// IVehicleEventListener

  virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams &params) {}

	// ~IVehicleEventListener
	
protected:

	BEGIN_SHARED_PARAMS(SSharedParams)

		string	effectName;

		float		damageRatioMin;

		bool		disableAfterExplosion;
		bool		updateFromHelper;

	END_SHARED_PARAMS

	void LoadEffect(IVehicleComponent *pComponent);
	void UpdateEffect(float randomness, float damageRatio);

	IVehicle								*m_pVehicle;

	SSharedParamsConstPtr		m_pSharedParams;
  
	const SDamageEffect			*m_pDamageEffect;
  	
	int											m_slot;
};

#endif //__VEHICLEDAMAGEBEHAVIOREFFECT_H__
