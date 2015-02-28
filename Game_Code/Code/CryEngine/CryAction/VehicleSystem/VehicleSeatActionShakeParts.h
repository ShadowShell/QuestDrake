/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
	Implements a seat action for shaking a list of parts based on
	vehicle movement, speed and acceleration

-------------------------------------------------------------------------
History:
- Created by Stan Fichele

*************************************************************************/

#ifndef __VEHICLESEATACTION_SHAKE_PARTS__H__
#define __VEHICLESEATACTION_SHAKE_PARTS__H__

#include <SharedParams/ISharedParams.h>
#include "VehicleNoiseGenerator.h"

class CVehicleSeatActionShakeParts
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT

public:

	CVehicleSeatActionShakeParts();

	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset() {}
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId); 
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
	virtual void PostSerialize(){}
	virtual void Update(const float deltaTime);

	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

	virtual void GetMemoryUsage(ICrySizer * s) const;

protected:

	BEGIN_SHARED_PARAMS(SSharedParams)

		struct SPartInfo
		{
			unsigned int   partIndex;
			float          amplitudeUpDown;
			float          amplitudeRot;
			float          freq;
			float          suspensionAmp;
			float          suspensionResponse;
			float          suspensionSharpness;
		};
		typedef std::vector<SPartInfo>  TPartInfos;
		typedef const TPartInfos        TPartInfosConst;
		TPartInfos                      partInfos;

	END_SHARED_PARAMS

	struct SPartInstance
	{
		// Updated at runtime
		CVehicleNoiseValue  noiseUpDown;
		CVehicleNoiseValue  noiseRot;
		float               zpos;
	};

	typedef std::vector<SPartInstance>  TParts;
	IVehicle*                           m_pVehicle;
	TParts                              m_controlledParts;
	SSharedParamsConstPtr               m_pSharedParams;
};

#endif //__VEHICLESEATACTION_SHAKE_PARTS__H__
