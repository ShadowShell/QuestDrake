/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Burst Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26:10:2005   12:15 : Created by Márcio Martins

*************************************************************************/
#ifndef __BURST_H__
#define __BURST_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CBurst : public CSingle
{
private:

	typedef CSingle BaseClass;

	class CBurstFiringLocator : public IWeaponFiringLocator
	{
	public:
		CBurstFiringLocator();
		void SetBurst(CBurst* pBurst);

	private:
		int GetCurrentWeaponSlot() const;
		bool HasValidFiringLocator() const;
		Matrix34 GetFiringLocatorTM();

		virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit);
		virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos);
		virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
		virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
		virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir);
		virtual void WeaponReleased();

		CBurst* m_pBurst;
	};

public:
	CRY_DECLARE_GTI(CBurst);

	CBurst();
	virtual ~CBurst();

	// CSingle
	virtual void Update(float frameTime, uint32 frameId);
	virtual void GetMemoryUsage(ICrySizer * s) const
	{ 
		s->AddObject(this, sizeof(*this));	
		CSingle::GetInternalMemoryUsage(s);		// collect memory of parent class
	}

	virtual void Activate(bool activate);
	virtual bool CanFire(bool considerAmmo /* = true */) const;
	virtual bool IsFiring() const;

	virtual void StartFire();
	virtual void StopFire();
	virtual void NetStartFire();
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	// ~CSingle

protected:
	virtual void EndBurst();

	int		m_burst_shot;
	bool	m_bursting;
	bool	m_fireTriggerDown;

	float	m_next_burst_dt;
	float	m_next_burst;

	bool  m_canShoot;

private:
	friend class CBurstFiringLocator;

	CBurstFiringLocator m_burstFiringLocator;
};


class CBurstFireAction : public TFiremodeAction<CBurst>
{
private:
	typedef TFiremodeAction<CBurst> BaseClass;

public:
	DEFINE_ACTION("BurstFireAction");

	CBurstFireAction(int priority, FragmentID fragmentID, CBurst* pBurst, TagState tagState)
		: TFiremodeAction<CBurst>(priority, fragmentID, pBurst, tagState)
	{
	}

	virtual EStatus Update(float timePassed);

	virtual EPriorityComparison ComparePriority(const IAction &actionCurrent) const
	{
		return Higher;
	}

private:
};


#endif