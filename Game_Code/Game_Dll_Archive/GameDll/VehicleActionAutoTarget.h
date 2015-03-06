/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Makes the vehicle a target for auto asist

*************************************************************************/
#ifndef __VEHICLEACTIONAUTOTARGET_H__
#define __VEHICLEACTIONAUTOTARGET_H__

#include "IVehicleSystem.h"
#include "AutoAimManager.h"

class CVehicleActionAutoTarget
	: public IVehicleAction
{
	IMPLEMENT_VEHICLEOBJECT;

public:
	CVehicleActionAutoTarget();
	virtual ~CVehicleActionAutoTarget();

	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table);
	virtual void Reset();
	virtual void Release();

	virtual int OnEvent(int eventType, SVehicleEventParams& eventParams);
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

	// IVehicleObject
	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
	virtual void Update(const float deltaTime) {};
	// ~IVehicleObject

protected:
	SAutoaimTargetRegisterParams m_autoAimParams;

	IVehicle* m_pVehicle;
	bool m_RegisteredWithAutoAimManager;

private:
	static const char * m_name;
};

#endif //__VEHICLEACTIONAUTOTARGET_H__
