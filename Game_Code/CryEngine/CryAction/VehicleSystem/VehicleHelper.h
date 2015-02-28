/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 03:04:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEHELPER_H__
#define __VEHICLEHELPER_H__

class CVehicle;

class CVehicleHelper
	: public IVehicleHelper
{
public:
	CVehicleHelper() : m_pParentPart(NULL)
	{
	}

	// IVehicleHelper
	virtual void Release() { delete this; }

	VIRTUAL const Matrix34& GetLocalTM() const { return m_localTM; }
	VIRTUAL void GetVehicleTM(Matrix34& vehicleTM, bool forced = false) const;
	VIRTUAL void GetWorldTM(Matrix34& worldTM) const;
	VIRTUAL void GetReflectedWorldTM(Matrix34 &reflectedWorldTM) const;

	VIRTUAL Vec3 GetLocalSpaceTranslation() const;
	VIRTUAL Vec3 GetVehicleSpaceTranslation() const;
	VIRTUAL Vec3 GetWorldSpaceTranslation() const;

	VIRTUAL IVehiclePart* GetParentPart() const;
	// ~IVehicleHelper

	void GetMemoryUsage(ICrySizer *pSizer)const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

protected:
	IVehiclePart* m_pParentPart;

	Matrix34 m_localTM;

	friend class CVehicle;
};

#endif
