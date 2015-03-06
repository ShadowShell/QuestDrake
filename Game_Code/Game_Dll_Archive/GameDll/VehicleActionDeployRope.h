/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a vehicle action to deploy a rope

-------------------------------------------------------------------------
History:
- 30:11:2006: Created by Mathieu Pinard
- 22:04:2010: Re-factored and re-worked by Paul Slinger

*************************************************************************/

#ifndef __VEHICLEACTIONDEPLOYROPE_H__
#define __VEHICLEACTIONDEPLOYROPE_H__

struct IRopeRenderNode;

class CVehicleActionDeployRope : public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT;

	public:

		CVehicleActionDeployRope();

		virtual ~CVehicleActionDeployRope();

		// IVehicleSeatAction

		virtual bool Init(IVehicle *pVehicle, IVehicleSeat* pSeat, const CVehicleParams &table);

		virtual void Reset();

		virtual void Release();

		virtual void StartUsing(EntityId passengerId);
		virtual void ForceUsage() {};

		virtual void StopUsing();

		virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

		virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams &params);

		virtual void Serialize(TSerialize serialize, EEntityAspects aspects);

		virtual void PostSerialize();

		virtual void Update(const float deltaTime);

		virtual void GetMemoryUsage(ICrySizer* pSizer) const;

		// ~IVehicleSeatAction

	protected:

		bool DeployRope();

		EntityId CreateRope(IPhysicalEntity *pLinkedEntity, const Vec3 &highPos, const Vec3 &lowPos);

		IRopeRenderNode *GetRopeRenderNode(EntityId ropeId);

		bool AttachActorToRope(EntityId actorId, EntityId ropeId);

		static const float	ms_extraRopeLength;

		IVehicle						*m_pVehicle;

		IVehicleSeat				*m_pSeat;

		IVehicleHelper			*m_pRopeHelper;

		IVehicleAnimation		*m_pDeployAnim;
		
		TVehicleAnimStateId	m_deployAnimOpenedId, m_deployAnimClosedId;

		EntityId						m_actorId, m_upperRopeId, m_lowerRopeId;

		float								m_altitude;
};

#endif //__VEHICLEACTIONDEPLOYROPE_H__