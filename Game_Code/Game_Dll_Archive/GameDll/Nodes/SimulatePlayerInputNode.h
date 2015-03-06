/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
*************************************************************************/

#pragma once

#ifndef SIMULATE_INPUT_NODE_H
#define SIMULATE_INPUT_NODE_H

#include "G2FlowBaseNode.h"

//Simulate player input/actions
class CFlowNode_SimulatePlayerInput : public CFlowBaseNode<eNCT_Singleton>
{
	enum EInputPorts
	{
		eInputPorts_Action,
		eInputPorts_Press,
		eInputPorts_Hold,
		eInputPorts_Release,
		eInputPorts_Value
	};

	enum EOutputPorts
	{
		eOutputPort_Pressed,
		eOutputPort_Held,
		eOutputPort_Released,
	};

public:
	CFlowNode_SimulatePlayerInput(SActivationInfo* pActInfo);

	virtual void GetConfiguration(SFlowNodeConfig& config);
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
	virtual void GetMemoryUsage(ICrySizer* sizer) const;

};

#endif