// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __VEHICLESEATACTIONORIENTATEBONETOVIEW_H__
#define __VEHICLESEATACTIONORIENTATEBONETOVIEW_H__

#include "ICryAnimation.h"
#include <IComponent.h>

struct ISkeletonPose;
struct IAnimatedCharacter;

class CVehicleSeatActionOrientateBoneToView
	: public IVehicleSeatAction
{
	IMPLEMENT_VEHICLEOBJECT

private:

public:
	CVehicleSeatActionOrientateBoneToView();

	virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
	virtual void Reset();
	virtual void Release() { delete this; }

	virtual void StartUsing(EntityId passengerId); 
	virtual void ForceUsage() {};
	virtual void StopUsing();
	virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value) {};

	virtual void Serialize(TSerialize ser, EEntityAspects aspects) {};
	virtual void PostSerialize(){}
	virtual void Update(const float deltaTime) { }

	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

	virtual void PrePhysUpdate(const float dt);

	virtual void GetMemoryUsage(ICrySizer * s) const;

protected:
	Ang3 GetDesiredViewAngles(const Vec3& lookPos, const Vec3& aimPos) const;
	Vec3 GetDesiredAimPosition() const;
	Vec3 GetCurrentLookPosition() const;

	IDefaultSkeleton* GetCharacterModelSkeleton() const;
	ISkeletonPose* GetSkeleton() const;

	IVehicle*		m_pVehicle;
	IVehicleSeat*	m_pSeat;

	IAnimationOperatorQueuePtr m_poseModifier;

	int m_MoveBoneId;
	int m_LookBoneId;

	float m_Sluggishness;

	Ang3 m_BoneOrientationAngles;
	Ang3 m_BoneSmoothingRate;
	Quat m_BoneBaseOrientation;

	IAnimatedCharacter* m_pAnimatedCharacter;
};

#endif // __VEHICLESEATACTIONORIENTATEBONETOVIEW_H__