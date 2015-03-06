/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a base class for the vehicle views

-------------------------------------------------------------------------
History:
- 06:07:2006: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"

#include "Game.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include "ICryAnimation.h"
#include "IViewSystem.h"
#include "IVehicleSystem.h"
#include "IWeapon.h"
#include "VehicleViewGameBase.h"
#include "GameCVars.h"

//------------------------------------------------------------------------
CVehicleViewGameBase::CVehicleViewGameBase()
{
	m_isDebugView = false;    
	m_pVehicle = NULL;
	m_pAimPart = NULL;
	m_pSeat = NULL;
	m_isAvailableRemotely = false;
}

//------------------------------------------------------------------------
bool CVehicleViewGameBase::Init(IVehicleSeat* pSeat, const CVehicleParams& table)
{
	m_pSeat = pSeat;
	m_pVehicle = pSeat->GetVehicle();
	m_pAimPart = pSeat->GetAimPart();

	m_hidePlayer = false;

	table.getAttr("hidePlayer", m_hidePlayer);
	table.getAttr("isAvailableRemotely", m_isAvailableRemotely);

	if (CVehicleParams hideTable = table.findChild("HideParts"))
	{
		int i = 0;
		int c = hideTable.getChildCount();

		for (; i < c; i++)
		{
			CVehicleParams partRef = hideTable.getChild(i);
			m_hideParts.push_back(partRef.getAttr("value"));
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CVehicleViewGameBase::Init(IVehicleSeat* pSeat)
{
	m_pSeat = pSeat;
	m_pVehicle = pSeat->GetVehicle();
	m_hidePlayer = false;
	return true;
}

//------------------------------------------------------------------------
void CVehicleViewGameBase::Reset()
{
}

//------------------------------------------------------------------------
void CVehicleViewGameBase::OnStartUsing(EntityId playerId)
{
	//if (VehicleCVars().v_debugdraw == eVDB_View)
	//CryLog("StartUsing: %s %s", m_pSeat->GetName().c_str(), IsThirdPerson()?"(tp)":"");

	Reset();

	m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);

	// Remember the player's view settings
	EntityId entity = m_pSeat->GetPassenger(true);
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity);
	if (pActor)
	{
		m_playerViewThirdOnExit = pActor->IsThirdPerson();
		if (pActor->IsThirdPerson() != this->IsThirdPerson())
			pActor->ToggleThirdPerson();
	}  


	// hide vehicle parts we don't want to see. NB there is a hide-count in vehicle part base, so we can safely
	// hide and unhide from here without upsetting anything

	IVehiclePart::SVehiclePartEvent partEvent;
	partEvent.type = IVehiclePart::eVPE_Hide;
	partEvent.bparam = true;

	for(size_t i=0; i<m_hideParts.size(); ++i)
	{
		if(IVehiclePart* pPart = m_pVehicle->GetPart(m_hideParts[i].c_str()))
		{
			pPart->OnEvent(partEvent);
		}
	}
}

//------------------------------------------------------------------------
void CVehicleViewGameBase::OnStopUsing()
{
	//if (VehicleCVars().v_debugdraw == eVDB_View)
	//CryLog("StopUsing: %s %s", m_pSeat->GetName().c_str(), IsThirdPerson()?"(tp)":"");

	m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);

	// unhide vehicle parts we hid while using
	
	IVehiclePart::SVehiclePartEvent partEvent;
	partEvent.type = IVehiclePart::eVPE_Hide;
	partEvent.bparam = false;

	for(size_t i=0; i<m_hideParts.size(); ++i)
	{
		if(IVehiclePart* pPart = m_pVehicle->GetPart(m_hideParts[i].c_str()))
		{
			pPart->OnEvent(partEvent);
		}
	}

	// Restore the player's original view settings
	EntityId entity = m_pSeat->GetPassenger(true);
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity);
	if (pActor)
	{
		if(pActor->IsClient())
		{
			if(gEnv->bMultiplayer)
			{
				m_playerViewThirdOnExit = pActor->IsDead() ?  (g_pGameCVars->pl_switchTPOnKill != 0) : false;
			}
			else
			{
				m_playerViewThirdOnExit = false;
			}
		}

		if(m_playerViewThirdOnExit != pActor->IsThirdPerson())
		{
			pActor->ToggleThirdPerson();
		}
	}

}

//------------------------------------------------------------------------
void CVehicleViewGameBase::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
}

//------------------------------------------------------------------------
void CVehicleViewGameBase::Update(const float frameTime)
{
}

//------------------------------------------------------------------------
void CVehicleViewGameBase::Serialize(TSerialize serialize, EEntityAspects aspects)
{
	if (serialize.GetSerializationTarget() != eST_Network)
	{
		//serialize.Value("rotation", m_rotation);
	}
}
