/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: Automatic shotgun firemode. It works like the shotgun one, spawning
             several pellets on a single shot, but doesn't require 'pump' action
						 and it has a 'single' magazine reload

-------------------------------------------------------------------------
History:
- 14:09:09   Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _AUTOMATIC_SHOTGUN_H_
#define _AUTOMATIC_SHOTGUN_H_

#include "Shotgun.h"

class CAutomaticShotgun : public CShotgun
{
public:
	CRY_DECLARE_GTI(CAutomaticShotgun);

	CAutomaticShotgun();
	virtual ~CAutomaticShotgun();

	virtual void Update(float frameTime, uint32 frameId);
	virtual void StartFire();
	virtual void StopFire();
	virtual void Activate(bool activate);
	virtual void StartReload(int zoomed);
	virtual void EndReload(int zoomed);

	virtual void CancelReload();
	virtual bool CanCancelReload();

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));	
		CShotgun::GetInternalMemoryUsage(s);		// collect memory of parent class
	}

private:
	float m_rapidFireCountdown;
	bool m_firing;
};

#endif