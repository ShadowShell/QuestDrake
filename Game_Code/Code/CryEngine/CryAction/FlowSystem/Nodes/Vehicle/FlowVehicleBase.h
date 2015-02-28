/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a flow node to vehicle seats

-------------------------------------------------------------------------
History:
- 12:12:2005: Created by Mathieu Pinard

*************************************************************************/
#ifndef __FLOWVEHICLEBASE_H__
#define __FLOWVEHICLEBASE_H__

#include "../FlowBaseNode.h"

class CFlowVehicleBase 
	: public CFlowBaseNode<eNCT_Instanced>,
	public IVehicleEventListener
{
public:

	virtual void Init(SActivationInfo* pActivationInfo);
	virtual void Delete();

	// CFlowBaseNode
	virtual void GetConfiguration(SFlowNodeConfig& nodeConfig);
	virtual void ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
	virtual void Serialize(SActivationInfo* pActivationInfo, TSerialize ser);
	// ~CFlowBaseNode

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	// ~IVehicleEventListener

protected:

	IVehicle* GetVehicle();

	IFlowGraph *m_pGraph;
	TFlowNodeId m_nodeID;

	EntityId m_vehicleId;
};

#endif
