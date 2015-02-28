/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Base class for effects managed by the effect system

-------------------------------------------------------------------------
History:
- 17:01:2006:		Created by Marco Koegler

*************************************************************************/
#ifndef __EFFECT_H__
#define __EFFECT_H__
#pragma once

#include "../IEffectSystem.h"


class CEffect : public IEffect
{
public:
	// IEffect
	VIRTUAL bool Activating(float delta);
	VIRTUAL bool Update(float delta);
	VIRTUAL bool Deactivating(float delta);
	VIRTUAL bool OnActivate();
	VIRTUAL bool OnDeactivate();
	VIRTUAL void SetState(EEffectState state);
	VIRTUAL EEffectState GetState();
	// ~IEffect
private:
	EEffectState		m_state;
};

#endif //__EFFECT_H__
