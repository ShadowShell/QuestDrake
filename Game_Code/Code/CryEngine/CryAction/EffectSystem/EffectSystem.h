/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Implements a system to manage various effects (bloodsplashes, 
							viewmodes, etc...)

-------------------------------------------------------------------------
History:
- 17:01:2006:		Created by Marco Koegler

*************************************************************************/
#ifndef __EFFECTSYSTEM_H__
#define __EFFECTSYSTEM_H__

#include "../IEffectSystem.h"

class CEffectSystem : public IEffectSystem
{
public:
	CEffectSystem();
	virtual ~CEffectSystem();

	// IEffectSystem
	VIRTUAL bool Init();
	VIRTUAL void Update(float delta);
	VIRTUAL void Shutdown();

	VIRTUAL EffectId GetEffectId(const char* name);

	VIRTUAL void Activate(const EffectId& eid);
	VIRTUAL bool BindEffect(const char* name, IEffect* pEffect);
	VIRTUAL IGroundEffect* CreateGroundEffect(IEntity* pEntity);

	virtual void RegisterFactory(const char *name, IEffect *(*)(), bool isAI);

	VIRTUAL void GetMemoryStatistics(ICrySizer * s);
	// ~IEffectSystem

private:
	typedef std::map<string, EffectId>	TNameToId;
	typedef std::vector<IEffect*>				TEffectVec;
	typedef std::map<string, IEffect *(*)()>	TEffectClassMap;

	TNameToId				m_nameToId;				// map from a string name to an effect id
	TEffectVec			m_effects;				// effect instances (array index == effect id)
	TEffectClassMap	m_effectClasses;	// effect factories
};

#endif //__EFFECTSYSTEM_H__
