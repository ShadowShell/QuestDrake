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
#ifndef __VEHICLEVIEWBASE_H__
#define __VEHICLEVIEWBASE_H__

class CVehicleViewGameBase
	: public IVehicleView
{
public:

	CVehicleViewGameBase();

	// IVehicleView
	virtual bool Init(IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset();
	virtual void ResetPosition() {};
	virtual void Release() { delete this; }

	virtual const char* GetName() { return NULL; }  
	virtual bool IsThirdPerson() = 0;
	virtual bool IsPassengerHidden() { return m_hidePlayer; }

	virtual void OnStartUsing(EntityId passengerId);
	virtual void OnStopUsing();

	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);
	virtual void UpdateView(SViewParams &viewParams, EntityId playerId) {}

	virtual void Update(const float frameTime);
	virtual void Serialize(TSerialize serialize, EEntityAspects);

	virtual void SetDebugView(bool debug) { m_isDebugView = debug; }
	virtual bool IsDebugView() { return m_isDebugView; }

	virtual bool ShootToCrosshair() { return true; }
	virtual bool IsAvailableRemotely() const { return m_isAvailableRemotely; }
	// ~IVehicleView

	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

	bool Init(IVehicleSeat* pSeat);

protected:

	IVehicle* m_pVehicle;
	IVehiclePart* m_pAimPart;
	IVehicleSeat* m_pSeat;

	bool m_hidePlayer;
	bool m_isDebugView;
	bool m_isAvailableRemotely;
	bool m_playerViewThirdOnExit;

	typedef std::vector <string> TVehiclePartNameVector;
	TVehiclePartNameVector m_hideParts;
};

#endif
