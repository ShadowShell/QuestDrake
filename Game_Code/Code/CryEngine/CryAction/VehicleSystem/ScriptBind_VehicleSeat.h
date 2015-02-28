/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Script Binding for the Vehicle Seat

-------------------------------------------------------------------------
History:
- 28:04:2004   17:02 : Created by Mathieu Pinard

*************************************************************************/
#ifndef __SCRIPTBIND_VEHICLESEAT_H__
#define __SCRIPTBIND_VEHICLESEAT_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IVehicleSystem;
struct IGameFramework;
class CVehicleSeat;

class CScriptBind_VehicleSeat :
	public CScriptableBase
{
public:

	CScriptBind_VehicleSeat( ISystem *pSystem, IGameFramework *pGameFW );
	virtual ~CScriptBind_VehicleSeat();

	void AttachTo(IVehicle *pVehicle, TVehicleSeatId seatId);
	void Release() { delete this; };

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	//! <code>VehicleSeat.GetVehicleSeat()</code>
	//! <description>Gets the vehicle seat identifier.
	CVehicleSeat* GetVehicleSeat(IFunctionHandler *pH);

	//! <code>VehicleSeat.Reset()</code>
	//! <description>Resets the vehicle seat.
	int Reset(IFunctionHandler *pH);

	//! <code>VehicleSeat.IsFree(actor)</code>
	//!		<param name="actorHandle">Passenger identifier.</param>
	//! <description>Checks if the seat is free.
	int IsFree(IFunctionHandler *pH, ScriptHandle actorHandle);

	//! <code>VehicleSeat.IsDriver()</code>
	//! <description>Checks if the seat is the driver seat.
	int IsDriver(IFunctionHandler* pH);

	//! <code>VehicleSeat.IsGunner()</code>
	//! <description>Checks if the seat is the gunner seat.
	int IsGunner(IFunctionHandler* pH);

	//! <code>VehicleSeat.GetWeaponId(weaponIndex)</code>
	//!		<param name="weaponIndex">Weapon identifier.</param>
	//! <description>Gets the weapon identifier.
	int GetWeaponId(IFunctionHandler* pH, int weaponIndex);

	//! <code>VehicleSeat.GetWeaponCount()</code>
	//! <description>Gets the number of weapons available on this seat.	
	int GetWeaponCount(IFunctionHandler* pH);

	//! <code>VehicleSeat.SetAIWeapon(weaponHandle)</code>
	//!		<param name="weaponHandle">Weapon identifier.</param>
	//! <description>Sets the weapon artificial intelligence.
	int SetAIWeapon(IFunctionHandler* pH, ScriptHandle weaponHandle);

	//! <code>VehicleSeat.GetPassengerId()</code>
	//! <description>Gets the passenger identifier.
	int GetPassengerId(IFunctionHandler* pH);

private:

	void RegisterGlobals();
	void RegisterMethods();
	
	IVehicleSystem *m_pVehicleSystem;
};

#endif //__SCRIPTBIND_VEHICLESEAT_H__
