/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a seat action for orientate one or multiple vehicle parts
			towards the view direction of the seat passanger

-------------------------------------------------------------------------
History:
- 16:11:2005: Created by Benito G.R.

*************************************************************************/

#ifndef __VEHICLESEATACTIONORIENTATEPARTTOVIEW_H__
#define __VEHICLESEATACTIONORIENTATEPARTTOVIEW_H__

class CVehicleSeatActionOrientatePartToView
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT

private:

	struct SOrientatePartInfo
	{
		SOrientatePartInfo()
			: partIndex(-1)
			, orientationAxis(1.0f, 1.0f, 1.0f)
		{

		}

		int		partIndex;
		Vec3	orientationAxis;
	};

public:

	CVehicleSeatActionOrientatePartToView();

	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset() {}
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId); 
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
	virtual void PostSerialize(){}
	virtual void Update(const float deltaTime);

	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

	virtual void GetMemoryUsage(ICrySizer * s) const;

protected:

	Vec3 GetViewDirection() const;

	typedef std::vector<SOrientatePartInfo>	TPartIndices;

	IVehicle*		m_pVehicle;
	IVehicleSeat*	m_pSeat;

	TPartIndices	m_controlledParts;
};

#endif //__VEHICLESEATACTIONORIENTATEPARTTOVIEW_H__