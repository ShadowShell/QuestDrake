/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a part for vehicles which is the an attachment 
	of a parent Animated part

-------------------------------------------------------------------------
History:
- 25:08:2005: Created by Mathieu Pinard

*************************************************************************/

#ifndef __VEHICLEPARTSUBPART_H__
#define __VEHICLEPARTSUBPART_H__

#include "VehiclePartBase.h"

class CVehicle;

class CVehiclePartSubPart : public CVehiclePartBase
{
	IMPLEMENT_VEHICLEOBJECT

	public:

		CVehiclePartSubPart();

		virtual ~CVehiclePartSubPart();

		// IVehiclePart
		virtual bool Init(IVehicle *pVehicle, const CVehicleParams &table, IVehiclePart *pParent, CVehicle::SPartInitInfo &initInfo, int partType);
		virtual void Reset();
		virtual void Release();
		virtual void OnEvent(const SVehiclePartEvent &event);
		virtual bool ChangeState(EVehiclePartState state, int flags = 0);
		virtual void Physicalize();
		virtual void Update(const float frameTime);  
		virtual void GetMemoryUsage(ICrySizer *pSizer) const;
		virtual void GetMemoryUsageInternal(ICrySizer *pSizer) const;
		virtual const Matrix34 &GetLocalInitialTM();
		// ~IVehiclePart

	protected:

		virtual void InitGeometry();

		void RegisterStateGeom(EVehiclePartState state, IStatObj *pStatObj);

		Matrix34	m_savedTM;

	private:

		struct StateGeom
		{
			inline StateGeom() : state(eVGS_Default), pStatObj(NULL)
			{
			}

			inline StateGeom(EVehiclePartState state, IStatObj *pStatObj) : state(state), pStatObj(pStatObj)
			{
				if(pStatObj)
				{
					pStatObj->AddRef();
				}
			}

			inline StateGeom(const StateGeom &other) : state(other.state), pStatObj(other.pStatObj)
			{
				if(pStatObj)
				{
					pStatObj->AddRef();
				}
			}

			~StateGeom()
			{
				if(pStatObj)
				{
					pStatObj->Release();
				}
			}

			inline StateGeom &operator = (const StateGeom &other)
			{
				state			= other.state;
				pStatObj	= other.pStatObj;

				if(pStatObj)
				{
					pStatObj->AddRef();
				}

				return *this;
			}

			EVehiclePartState	state;
			IStatObj					*pStatObj;
		};

		typedef std::vector<StateGeom> TStateGeomVector;

		TStateGeomVector	m_stateGeoms;
};

#endif //__VEHICLEPARTSUBPART_H__