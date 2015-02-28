/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a part for vehicles which uses static objects

-------------------------------------------------------------------------
History:
- 23:08:2005: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEPARTSTATIC_H__
#define __VEHICLEPARTSTATIC_H__

class CVehicle;

class CVehiclePartStatic
	: public CVehiclePartBase
{
	IMPLEMENT_VEHICLEOBJECT
public:

	CVehiclePartStatic() {}
	~CVehiclePartStatic() {}

	// IVehiclePart
	virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
  virtual void InitGeometry();
	virtual void Release();
	virtual void Reset();
	
	virtual void Physicalize();

	virtual void Update(const float frameTime);

  virtual void SetLocalTM(const Matrix34& localTM);

	virtual void GetMemoryUsage(ICrySizer * s) const 
	{ 
		s->Add(*this); 
		s->AddObject(m_filename);		
		s->AddObject(m_filenameDestroyed);
		s->AddObject(m_geometry);
		CVehiclePartBase::GetMemoryUsageInternal(s); 
	}
	// ~IVehiclePart

protected:

	string m_filename;
	string m_filenameDestroyed;
	string m_geometry;
};

#endif
