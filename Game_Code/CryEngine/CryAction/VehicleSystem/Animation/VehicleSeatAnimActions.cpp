/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2012.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements vehicle seat specific Mannequin actions

-------------------------------------------------------------------------
History:
- 06:02:2012: Created by Tom Berry

*************************************************************************/
#include "StdAfx.h"

#include "../Vehicle.h"
#include "../VehicleSeat.h"

#include "VehicleSeatAnimActions.h"

void CVehicleSeatAnimActionEnter::Enter()
{
	BaseAction::Enter();

	bool isThirdPerson = !m_pSeat->ShouldEnterInFirstPerson();
	TVehicleViewId viewId = InvalidVehicleViewId;
	TVehicleViewId firstViewId = InvalidVehicleViewId;

	while (viewId = m_pSeat->GetNextView(viewId))
	{
		if (viewId == firstViewId)
			break;

		if (firstViewId == InvalidVehicleViewId)
			firstViewId = viewId;

		if (IVehicleView* pView = m_pSeat->GetView(viewId))
		{
			if (pView->IsThirdPerson() == isThirdPerson)
				break;
		}
	}

	if (viewId != InvalidVehicleViewId)
		m_pSeat->SetView(viewId);

	IActor* pActor = m_pSeat->GetPassengerActor();
	CRY_ASSERT(pActor);

	IAnimatedCharacter *pAnimChar = pActor ? pActor->GetAnimatedCharacter() : NULL;
	if (pAnimChar)
	{
		pAnimChar->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
		pAnimChar->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "CVehicleSeatAnimActionEnter::Enter");
	}
}

void CVehicleSeatAnimActionExit::Enter()
{
	BaseAction::Enter();

	bool isThirdPerson = !m_pSeat->ShouldExitInFirstPerson();
	TVehicleViewId viewId = InvalidVehicleViewId;
	TVehicleViewId firstViewId = InvalidVehicleViewId;

	while (viewId = m_pSeat->GetNextView(viewId))
	{
		if (viewId == firstViewId)
			break;

		if (firstViewId == InvalidVehicleViewId)
			firstViewId = viewId;

		if (IVehicleView* pView = m_pSeat->GetView(viewId))
		{
			if (pView->IsThirdPerson() == isThirdPerson)
				break;
		}
	}

	if (viewId != InvalidVehicleViewId)
		m_pSeat->SetView(viewId);

	IActor* pActor = m_pSeat->GetPassengerActor();
	CRY_ASSERT(pActor);

	IAnimatedCharacter *pAnimChar = pActor ? pActor->GetAnimatedCharacter() : NULL;
	if (pAnimChar)
	{
		pAnimChar->SetMovementControlMethods(eMCM_Animation, eMCM_Animation);
		pAnimChar->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "CVehicleSeatAnimActionExit::Enter");
	}
}

void CVehicleSeatAnimActionExit::Exit()
{
	BaseAction::Exit();

	IActor* pActor = m_pSeat->GetPassengerActor();
	CRY_ASSERT(pActor);

	m_pSeat->StandUp();
}
