/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a flow node to handle vehicle turrets

-------------------------------------------------------------------------
History:
- 12:12:2005: Created by Mathieu Pinard

*************************************************************************/
#ifndef __FLOWVEHICLETURRET_H__
#define __FLOWVEHICLETURRET_H__

#include "FlowVehicleBase.h"

class CFlowVehicleTurret
	: public CFlowVehicleBase
{
public:

	CFlowVehicleTurret(SActivationInfo* pActivationInfo) { Init(pActivationInfo); }
	~CFlowVehicleTurret() { Delete(); }

	// CFlowBaseNode
	virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
	virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
	// ~CFlowBaseNode

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	// ~IVehicleEventListener

	void SetAimPos();

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

protected:

	enum EInputs
	{
		IN_TRIGGER,
		IN_SEATID,
		IN_AIMANGLES,
		IN_AIMPOS,
	};

	enum EOutputs
	{
		OUT_HASREACHEDAIMGOAL,
	};

	TVehicleSeatId m_seatId;
	CVehicleSeatActionRotateTurret* m_pActionRotateTurret;

	Vec3 m_aimPos;
};

#endif
