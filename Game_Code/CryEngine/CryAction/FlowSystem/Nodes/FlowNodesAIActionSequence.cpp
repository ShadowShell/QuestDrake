////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   FlowNodesAIActionSequence.cpp
//  Version:     v1.00
//  Created:     2014-04-29 by Christian Werle.
//  Description: A bunch of FlowNodes that are AI-Sequence-compatible and can only be instantiated from within such one.
//               Having them AI-Sequence-compatible is an "intended restriction" for a higher-level decision system to manage
//               what is currently executing on an AI in a more controllable way which ultimately boils down to being able to 
//               distinguish between autonomous behavior vs. scripting via FlowNodes.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <IAIActionSequence.h>
#include <IMovementSystem.h>
#include <MovementRequest.h>
#include "FlowBaseNode.h"
#include "VehicleSystem/VehicleSeat.h"
#include "VehicleSystem/VehicleCVars.h"

namespace
{

//////////////////////////////////////////////////////////////////////////
//
// AIActionSequenceFlowNodeBase
//
//////////////////////////////////////////////////////////////////////////
class AIActionSequenceFlowNodeBase
	: public CFlowBaseNode<eNCT_Instanced>
	, public AIActionSequence::SequenceActionBase
{
protected:
	void FinishSequenceActionAndActivateOutputPort(int port);
	void CancelSequenceAndActivateOutputPort(int port);

	SActivationInfo m_actInfo;
};

void AIActionSequenceFlowNodeBase::FinishSequenceActionAndActivateOutputPort(int port)
{
	gEnv->pAISystem->GetSequenceManager()->ActionCompleted(GetAssignedSequenceId());
	ActivateOutput(&m_actInfo, port, true);
}

void AIActionSequenceFlowNodeBase::CancelSequenceAndActivateOutputPort(int port)
{
	gEnv->pAISystem->GetSequenceManager()->CancelSequence(GetAssignedSequenceId());
	ActivateOutput(&m_actInfo, port, true);
}

} // namespace


//////////////////////////////////////////////////////////////////////////
//
// CFlowNode_AISequenceAction_ApproachAndEnterVehicle
//
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISequenceAction_ApproachAndEnterVehicle
	: public AIActionSequenceFlowNodeBase
	, public IVehicleEventListener
{
public:
	CFlowNode_AISequenceAction_ApproachAndEnterVehicle(SActivationInfo* pActInfo);
	~CFlowNode_AISequenceAction_ApproachAndEnterVehicle();

	// IFlowNode
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
	virtual void GetConfiguration(SFlowNodeConfig& config);
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);
	virtual void GetMemoryUsage(ICrySizer* sizer) const;
	// ~IFlowNode

	// AIActionSequence::SequenceActionBase
	virtual void HandleSequenceEvent(AIActionSequence::SequenceEvent sequenceEvent);
	// ~AIActionSequence::SequenceActionBase

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	// ~IVehicleEventListener

private:
	enum
	{
		InputPort_Start,
		InputPort_VehicleId,
		InputPort_SeatNumber,
		InputPort_Speed,
		InputPort_Stance,
		InputPort_Fast,
	};

	enum
	{
		OutputPort_Done
	};

	IEntity* GetEntity();
	IVehicle* GetVehicle(bool alsoCheckForCrewHostility);
	IVehicleSeat* GetVehicleSeat(IVehicle* pVehicleHint);
	bool GetAnimationTransitionEnabled() const;
	void MovementRequestCallback(const MovementRequestResult& result);
	void TeleportToVehicleSeat();
	void EnterVehicleSeat(bool animationTransitionEnabled, IVehicleSeat* pVehicleSeatHint);
	void UnregisterFromVehicleEvent(IVehicleSeat* pVehicleSeatHint);

	EntityId m_entityId;
	EntityId m_vehicleId;
	int m_seatNumber;
	Vec3 m_vehicleSeatEnterPosition;
	MovementRequestID m_movementRequestID;  // used for moving to the vehicle seat that we wanna enter
	bool m_fast;                            // if true: skip approaching and enter-animation
};

CFlowNode_AISequenceAction_ApproachAndEnterVehicle::CFlowNode_AISequenceAction_ApproachAndEnterVehicle(SActivationInfo* pActInfo)
	: m_entityId(0)
	, m_vehicleId(0)
	, m_seatNumber(0)
	, m_vehicleSeatEnterPosition(ZERO)
	, m_fast(false)
{
}

CFlowNode_AISequenceAction_ApproachAndEnterVehicle::~CFlowNode_AISequenceAction_ApproachAndEnterVehicle()
{
}

IFlowNodePtr CFlowNode_AISequenceAction_ApproachAndEnterVehicle::Clone(SActivationInfo* pActInfo)
{
	return new CFlowNode_AISequenceAction_ApproachAndEnterVehicle(pActInfo);
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetConfiguration(SFlowNodeConfig& config)
{
	static const SInputPortConfig inputPortConfig[] =
	{
		InputPortConfig_Void("Start"),
		InputPortConfig<EntityId>("VehicleId", _HELP("Vehicle to be entered")),
		InputPortConfig<int>("SeatNumber", _HELP("Seat to be entered"), _HELP("Seat"), _UICONFIG("enum_int:None=0,Driver=1,Gunner=2,Seat03=3,Seat04=4,Seat05=5,Seat06=6,Seath07=7,Seat08=8,Seat09=9,Seat10=10,Seat11=11")),
		InputPortConfig<int>("Speed", _HELP("Speed to approach the vehicle with"), NULL, _UICONFIG("enum_int:Walk=0,Run=1,Sprint=2")),
		InputPortConfig<int>("Stance", _HELP("Stance while approaching the vehicle"), NULL, _UICONFIG("enum_int:Relaxed=0,Alert=1,Combat=2")),
		InputPortConfig<bool>("Fast", _HELP("skip approaching and entering animation")),
		{ 0 }
	};

	static const SOutputPortConfig outputPortConfig[] =
	{
		OutputPortConfig_Void("Done"),
		{ 0 }
	};

	config.pInputPorts = inputPortConfig;
	config.pOutputPorts = outputPortConfig;
	config.sDescription = _HELP("Allows the AI agent to move to the specified vehicle and to then enter the specified seat");
	config.nFlags |= EFLN_TARGET_ENTITY | EFLN_AISEQUENCE_ACTION;
	config.SetCategory(EFLN_APPROVED);
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::ProcessEvent(IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo)
{
	if (!pActInfo->pEntity)
		return;

	switch(event)
	{
	case eFE_Activate:
		{
			m_actInfo = *pActInfo;
			m_movementRequestID = MovementRequestID::Invalid();

			if (IsPortActive(pActInfo, InputPort_Start))
			{
				if (const AIActionSequence::SequenceId assignedSequenceId = GetAssignedSequenceId())
				{
					gEnv->pAISystem->GetSequenceManager()->RequestActionStart(assignedSequenceId, pActInfo->myID);
					m_actInfo.pGraph->SetRegularlyUpdated(m_actInfo.myID, true);
				}
			}
		}
		break;
	}
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetMemoryUsage(ICrySizer* sizer) const
{
	sizer->Add(*this);
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::HandleSequenceEvent(AIActionSequence::SequenceEvent sequenceEvent)
{
	switch(sequenceEvent)
	{
	case AIActionSequence::StartAction:
		{
			if (!m_actInfo.pEntity)
			{
				// the entity has gone for some reason, at least make sure the action gets finished properly and the FG continues
				UnregisterFromVehicleEvent(NULL);
				CancelSequenceAndActivateOutputPort(OutputPort_Done);
				return;
			}

			m_entityId = m_actInfo.pEntity->GetId();
			m_vehicleId = GetPortEntityId(&m_actInfo, InputPort_VehicleId);
			m_seatNumber = GetPortInt(&m_actInfo, InputPort_SeatNumber);
			m_fast = GetPortBool(&m_actInfo, InputPort_Fast);

			const bool alsoCheckForCrewHostility = true;
			IVehicle* pVehicle = GetVehicle(alsoCheckForCrewHostility);
			if (!pVehicle)
			{
				CryLog("Actor %s failed to enter vehicle (specified vehicle not found or its crew is hostile towards the actor returned true)", m_actInfo.pEntity->GetName());
				CancelSequenceAndActivateOutputPort(OutputPort_Done);
				return;
			}

			IVehicleSeat* pSeat = GetVehicleSeat(pVehicle);
			if (!pSeat)
			{
				CryLog("Actor %s failed to enter vehicle (bad seat number provided: %i)", m_actInfo.pEntity->GetName(), m_seatNumber);
				CancelSequenceAndActivateOutputPort(OutputPort_Done);
				return;
			}

			const IVehicleHelper* pEnterHelper = static_cast<CVehicleSeat*>(pSeat)->GetEnterHelper();
			if (!pEnterHelper)
			{
				CryLog("Actor %s failed to enter vehicle (vehicle has no enter-helper)", m_actInfo.pEntity->GetName());
				CancelSequenceAndActivateOutputPort(OutputPort_Done);
				return;
			}

			m_vehicleSeatEnterPosition = pEnterHelper->GetWorldSpaceTranslation();

			assert(gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework() && gEnv->pGame->GetIGameFramework()->GetIActorSystem());
			IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_actInfo.pEntity->GetId());

			// if it's the player, have him enter quickly (we assume that the user moved him close enough to the vehicle)
			if (pActor && pActor->IsPlayer())
			{
				EnterVehicleSeat(true, pSeat);
			}
			else if (m_actInfo.pEntity->GetAI())
			{
				if (m_fast)
				{
					TeleportToVehicleSeat();
					EnterVehicleSeat(false, pSeat);
				}
				else
				{
					MovementRequest request;
					request.callback = functor(*this, &CFlowNode_AISequenceAction_ApproachAndEnterVehicle::MovementRequestCallback);
					request.entityID = m_actInfo.pEntity->GetId();
					request.type = MovementRequest::MoveTo;
					request.destination = m_vehicleSeatEnterPosition;
					request.style.SetSpeed((MovementStyle::Speed)GetPortInt(&m_actInfo, InputPort_Speed));
					request.style.SetStance((MovementStyle::Stance)GetPortInt(&m_actInfo, InputPort_Stance));
					m_movementRequestID = gEnv->pAISystem->GetMovementSystem()->QueueRequest(request);
				}
			}
			else if (pActor)
			{
				pActor->HolsterItem(true);
				pActor->MountedGunControllerEnabled(false);
				pActor->GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);
				TeleportToVehicleSeat();
				EnterVehicleSeat(GetAnimationTransitionEnabled(), pSeat);
			}
			else
			{
				CRY_ASSERT_MESSAGE(0, "no compatible entity was provided");
				CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_WARNING, "Actor %s failed to enter vehicle (no compatible entity was provided)", m_actInfo.pEntity->GetName());
				CancelSequenceAndActivateOutputPort(OutputPort_Done);
			}
		}
		break;

	case AIActionSequence::SequenceStopped:
		{
			if (m_movementRequestID)
			{
				gEnv->pAISystem->GetMovementSystem()->CancelRequest(m_movementRequestID);
				m_movementRequestID = MovementRequestID::Invalid();
				UnregisterFromVehicleEvent(NULL);
			}
		}
		break;
	}
}

IEntity* CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetEntity()
{
	assert(gEnv && gEnv->pEntitySystem);
	return gEnv->pEntitySystem->GetEntity(m_entityId);
}

IVehicle* CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetVehicle(bool alsoCheckForCrewHostility)
{
	if (!m_vehicleId)
		return NULL;

	assert(gEnv && gEnv->pGame && gEnv->pGame->GetIGameFramework() && gEnv->pGame->GetIGameFramework()->GetIVehicleSystem());
	IVehicle* pVehicle = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(m_vehicleId);
	if (!pVehicle)
		return NULL;

	if (alsoCheckForCrewHostility && pVehicle->IsCrewHostile(m_entityId))
		return NULL;

	return pVehicle;
}

IVehicleSeat* CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetVehicleSeat(IVehicle* pVehicleHint)
{
	if (!pVehicleHint)
	{
		const bool alsoCheckForCrewHostility = false;
		pVehicleHint = GetVehicle(alsoCheckForCrewHostility);
	}
	return pVehicleHint ? pVehicleHint->GetSeatById(m_seatNumber) : NULL;
}

bool CFlowNode_AISequenceAction_ApproachAndEnterVehicle::GetAnimationTransitionEnabled() const
{
	bool transitionEnabled = !m_fast;

	// check for suppressed animation transition via cvar v_transitionAnimations
	if (VehicleCVars().v_transitionAnimations == 0)
		transitionEnabled = false;

	return transitionEnabled;
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::MovementRequestCallback(const MovementRequestResult& result)
{
	assert(m_movementRequestID == result.requestID);

	switch(result.result)
	{
	case MovementRequestResult::Failure:
		{
			// could not reach the vehicle seat (probably no path found)
			// => just teleport there
			TeleportToVehicleSeat();
		}
		// fall through
	case MovementRequestResult::ReachedDestination:
		{
			m_movementRequestID = MovementRequestID::Invalid();
			EnterVehicleSeat(GetAnimationTransitionEnabled(), NULL);
		}
		break;
	}
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::TeleportToVehicleSeat()
{
	if (IEntity* pEntity = GetEntity())
	{
		Matrix34 transform = pEntity->GetWorldTM();
		transform.SetTranslation(m_vehicleSeatEnterPosition);
		pEntity->SetWorldTM(transform);
	}
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::EnterVehicleSeat(bool animationTransitionEnabled, IVehicleSeat* pVehicleSeatHint)
{
	if (!pVehicleSeatHint)
		pVehicleSeatHint = GetVehicleSeat(NULL);

	if (pVehicleSeatHint)
	{
		pVehicleSeatHint->GetVehicle()->RegisterVehicleEventListener(this, "CFlowNode_AISequenceAction_ApproachAndEnterVehicle");
		pVehicleSeatHint->Enter(m_entityId, animationTransitionEnabled);
	}
	else
	{
		// prematurely finish the action as the seat or entity has magically disappeared
		CancelSequenceAndActivateOutputPort(OutputPort_Done);
	}
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::UnregisterFromVehicleEvent(IVehicleSeat* pVehicleSeatHint)
{
	IVehicle* pVehicle = NULL;

	if (pVehicleSeatHint)
	{
		pVehicle = pVehicleSeatHint->GetVehicle();
	}
	else
	{
		const bool alsoCheckForCrewHostility = false;
		pVehicle = GetVehicle(alsoCheckForCrewHostility);
	}

	if (pVehicle)
	{
		pVehicle->UnregisterVehicleEventListener(this);
	}
}

void CFlowNode_AISequenceAction_ApproachAndEnterVehicle::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
	if (event == eVE_PassengerEnter)
	{
		UnregisterFromVehicleEvent(NULL);
		FinishSequenceActionAndActivateOutputPort(OutputPort_Done);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("AISequence:ApproachAndEnterVehicle", CFlowNode_AISequenceAction_ApproachAndEnterVehicle)
