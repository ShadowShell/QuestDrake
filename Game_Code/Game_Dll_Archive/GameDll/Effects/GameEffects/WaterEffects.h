/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2012.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Water related effects

-------------------------------------------------------------------------
History:
- 16:05:2012   Created by Benito Gangoso Rodriguez

*************************************************************************/
#pragma once

#ifndef _WATER_GAME_EFFECTS_
#define _WATER_GAME_EFFECTS_

#include "GameEffect.h"
#include "Effects/GameEffectsSystem.h"

class CWaterGameEffects : public CGameEffect
{
	typedef CGameEffect BaseEffectClass;

public:
	CWaterGameEffects( );
	virtual ~CWaterGameEffects( );

	virtual void	Update(float frameTime);
	virtual const char* GetName() const;
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const;

	void OnCameraComingOutOfWater( );

private:

	float m_waterDropletsAmount;
};

#endif // _WATER_GAME_EFFECTS_