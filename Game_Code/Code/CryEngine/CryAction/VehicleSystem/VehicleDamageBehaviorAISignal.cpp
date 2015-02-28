/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a damage behavior which send an AI signal

-------------------------------------------------------------------------
History:
- 23:11:2006: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "IAgent.h"
#include "IVehicleSystem.h"
#include "VehicleDamageBehaviorAISignal.h"

//------------------------------------------------------------------------
bool CVehicleDamageBehaviorAISignal::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
	m_pVehicle = pVehicle;

	CVehicleParams signalTable = table.findChild("AISignal");
	if (!signalTable)
		return false;

	if (!signalTable.getAttr("signalId", m_signalId))
		return false;
	
	m_signalText = signalTable.getAttr("signalText");

	m_freeSignalText = signalTable.getAttr("freeSignalText");
	m_freeSignalRadius = 15.0f;
	signalTable.getAttr("freeSignalRadius", m_freeSignalRadius);
	m_freeSignalRepeat = false;
	signalTable.getAttr("freeSignalRepeat",m_freeSignalRepeat);

	return true;
}

void CVehicleDamageBehaviorAISignal::Reset()
{
	if (m_isActive)
		ActivateUpdate(false);
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorAISignal::ActivateUpdate(bool activate)
{
	if (activate && !m_isActive)
	{
		m_timeCounter = 1.0f;
		m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);
	}
	else if (!activate && m_isActive)
		m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_NoUpdate);

	m_isActive = activate;
}

//------------------------------------------------------------------------
void CVehicleDamageBehaviorAISignal::OnDamageEvent(EVehicleDamageBehaviorEvent event, 
																									 const SVehicleDamageBehaviorEventParams& behaviorParams)
{
	if(event == eVDBE_VehicleDestroyed || event == eVDBE_MaxRatioExceeded)
	{
		ActivateUpdate(false);
		return;
	}
	
	if(event != eVDBE_Hit)
		return;
	
	IEntity* pEntity = m_pVehicle->GetEntity();
	CRY_ASSERT(pEntity);

	IAISystem* pAISystem = gEnv->pAISystem;

	if(!pAISystem)
		return;

	if (!m_freeSignalText.empty() && pEntity)
	{
		Vec3 entityPosition = pEntity->GetPos();

		IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
		pData->point = entityPosition;

		pAISystem->SendAnonymousSignal(m_signalId, m_freeSignalText.c_str(), entityPosition, m_freeSignalRadius, NULL, pData);

		if(m_freeSignalRepeat)
			ActivateUpdate(true);
	}

	if (!pEntity || !pEntity->HasAI())
		return;

	IAISignalExtraData* pExtraData = pAISystem->CreateSignalExtraData();
	CRY_ASSERT(pExtraData);

	if (pExtraData)
		pExtraData->iValue = behaviorParams.shooterId;

	pAISystem->SendSignal(SIGNALFILTER_SENDER, m_signalId, m_signalText.c_str(), pEntity->GetAI(), pExtraData);
}

void CVehicleDamageBehaviorAISignal::Update( const float deltaTime )
{
	if(m_pVehicle->IsDestroyed())
	{
		ActivateUpdate(false);
		return;
	}
	
	m_timeCounter -= deltaTime;
	if(m_timeCounter < 0.0f)
	{
		IEntity* pEntity = m_pVehicle->GetEntity();
		CRY_ASSERT(pEntity);
		IAISystem* pAISystem = gEnv->pAISystem;
		CRY_ASSERT(pAISystem);

		Vec3 entityPosition = pEntity->GetPos();

		IAISignalExtraData* pData = gEnv->pAISystem->CreateSignalExtraData();
		pData->point = entityPosition;

		pAISystem->SendAnonymousSignal(m_signalId, m_freeSignalText.c_str(), entityPosition, m_freeSignalRadius, NULL, pData);

		m_timeCounter = 1.0f;
	}
}

void CVehicleDamageBehaviorAISignal::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this,sizeof(*this));
	s->AddObject(m_signalText);
}

DEFINE_VEHICLEOBJECT(CVehicleDamageBehaviorAISignal);
