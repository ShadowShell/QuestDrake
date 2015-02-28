/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a flow node to handle vehicle seats

-------------------------------------------------------------------------
History:
- 12:12:2005: Created by Mathieu Pinard

*************************************************************************/
#ifndef __FLOWVEHICLESEAT_H__
#define __FLOWVEHICLESEAT_H__

#include "FlowVehicleBase.h"

class CFlowVehicleSeat
	: public CFlowVehicleBase
{
public:

	CFlowVehicleSeat(SActivationInfo* pActivationInfo);
	~CFlowVehicleSeat() { Delete(); }

	// CFlowBaseNode
	virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
	// ~CFlowBaseNode

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	// ~IVehicleEventListener

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

protected:

	void ActivateOutputPorts(SActivationInfo* pActivationInfo);

	enum EInputs
	{
    IN_SEATID,
		IN_SEATNAME,
		IN_DRIVERSEAT,
    IN_LOCK,
		IN_UNLOCK,
		IN_LOCKTYPE,
	};

	enum EOutputs
	{
		OUT_SEATID,
		OUT_PASSENGERID,
	};

	TVehicleSeatId m_seatId;
	bool m_isDriverSeatRequested;
};

#endif
