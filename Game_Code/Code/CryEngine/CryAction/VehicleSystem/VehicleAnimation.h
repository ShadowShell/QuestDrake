/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 22:03:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLEANIMATION_H__
#define __VEHICLEANIMATION_H__

#include <IVehicleSystem.h>

class CVehiclePartAnimated;

class CVehicleAnimation : public IVehicleAnimation
{
public:

	CVehicleAnimation();
	virtual ~CVehicleAnimation() {}

	VIRTUAL bool Init(IVehicle* pVehicle, const CVehicleParams& table);
	VIRTUAL void Reset();
	virtual void Release() { delete this; }

	VIRTUAL bool StartAnimation();
	VIRTUAL void StopAnimation();

	VIRTUAL bool ChangeState(TVehicleAnimStateId stateId);
	VIRTUAL TVehicleAnimStateId GetState();

	VIRTUAL string GetStateName(TVehicleAnimStateId stateId);
	VIRTUAL TVehicleAnimStateId GetStateId(const string& name);
	
	VIRTUAL void SetSpeed(float speed);

	VIRTUAL void ToggleManualUpdate(bool isEnabled);
	VIRTUAL void SetTime(float time, bool force=false);
	
	VIRTUAL float GetAnimTime(bool raw=false);
  VIRTUAL bool IsUsingManualUpdates();
	
	VIRTUAL void GetMemoryUsage(ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_animationStates);
	}
protected:

	struct SAnimationStateMaterial
	{
		string	material, setting;
		float		_min, _max;
		bool		invertValue;

		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(material);
			pSizer->AddObject(setting);
		}
	};

	typedef std::vector <SAnimationStateMaterial> TAnimationStateMaterialVector;

	struct SAnimationState
	{
		string name;
		string animation;

		string sound;
		//tSoundID soundId;
		IVehicleHelper* pSoundHelper;

		float speedDefault;
		float speedMin;
		float speedMax;
		bool isLooped;
		bool isLoopedEx;

		TAnimationStateMaterialVector materials;

		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(name);
			pSizer->AddObject(animation);
			pSizer->AddObject(sound);
			pSizer->AddObject(pSoundHelper);
			pSizer->AddObject(materials);
		}
	};

	typedef std::vector <SAnimationState> TAnimationStateVector;

protected:

	bool ParseState(const CVehicleParams& table, IVehicle* pVehicle);
	IMaterial* FindMaterial(const SAnimationStateMaterial& animStateMaterial, IMaterial* pMaterial);

	CVehiclePartAnimated* m_pPartAnimated;
	int m_layerId;

	TAnimationStateVector m_animationStates;
	TVehicleAnimStateId m_currentStateId;

	bool m_currentAnimIsWaiting;
};


#endif
