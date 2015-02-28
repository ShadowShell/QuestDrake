/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a wheel vehicle part

-------------------------------------------------------------------------
History:
- 25:08:2005: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEPARTSUBPARTWHEEL_H__
#define __VEHICLEPARTSUBPARTWHEEL_H__

#include "VehiclePartSubPart.h"

class CVehicle;

class CVehiclePartSubPartWheel
  : public CVehiclePartSubPart   
  , public IVehicleWheel 
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehiclePartSubPartWheel();	
	virtual ~CVehiclePartSubPartWheel() {}


	// IVehiclePart
	virtual void Reset();
	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
  
  virtual bool ChangeState(EVehiclePartState state, int flags=0);
	virtual void Physicalize();  
	virtual void OnEvent(const SVehiclePartEvent& event);	
	
  virtual void Serialize(TSerialize ser, EEntityAspects aspects);  
	virtual void PostSerialize();

  virtual IVehicleWheel* GetIWheel() { return this; }
	virtual void GetMemoryUsage(ICrySizer * s) const 
	{ 
		s->Add(*this); 
		CVehiclePartSubPart::GetMemoryUsageInternal(s); 
	}
	// ~IVehiclePart  
  
  // IVehicleWheel
  virtual int GetSlot() const { return m_slot; }
  virtual int GetWheelIndex() const { return m_pSharedParameters->m_wheelIndex; }
  virtual float GetTorqueScale() const { return m_pSharedParameters->m_torqueScale; }
  virtual float GetSlipFrictionMod(float slip) const;  
  virtual const pe_cargeomparams* GetCarGeomParams() const { return &m_physGeomParams; }
  // ~IVehicleWheel

	float GetRimRadius() const { return m_pSharedParameters->m_rimRadius; }

  virtual void GetGeometryName(EVehiclePartState state, string& name);

protected:
  
  virtual void InitGeometry();
  bool IsFront() const { return m_physGeomParams.pivot.y > 0.f; }

	pe_cargeomparams m_physGeomParams;	  
  
	friend class CVehiclePartTread;  
  friend class CVehicle;
};

#endif
