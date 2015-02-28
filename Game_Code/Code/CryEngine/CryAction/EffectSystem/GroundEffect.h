// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __GROUNDEFFECT_H__
#define __GROUNDEFFECT_H__

#include "../IEffectSystem.h"
#include "CryActionPhysicQueues.h"

struct IParticleEffect;

class CGroundEffect : public IGroundEffect
{
public:

	CGroundEffect(IEntity *pEntity);
	virtual ~CGroundEffect();

	// IGroundEffect
	VIRTUAL void SetHeight(float height);
  VIRTUAL void SetHeightScale(float sizeScale, float countScale); 
  VIRTUAL void SetBaseScale(float sizeScale, float countScale, float speedScale = 1.0f);
  VIRTUAL void SetInterpolation(float speed);
	VIRTUAL void SetFlags(int flags);
	VIRTUAL int GetFlags() const;
	VIRTUAL bool SetParticleEffect(const char *pName);
  VIRTUAL void SetInteraction(const char *pName);  
	VIRTUAL void Update();
	VIRTUAL void Stop(bool stop);
	// ~IGroundEffect

	void OnRayCastDataReceived(const QueuedRayID &rayID, const RayCastResult &result);

protected:

  void SetSpawnParams(const SpawnParams &params);
  void Reset();

	inline bool DebugOutput() const
	{
		static ICVar	*pVar = gEnv->pConsole->GetCVar("g_groundeffectsdebug");

		CRY_ASSERT(pVar);

		return pVar->GetIVal() > 0;
	}

	inline bool DeferredRayCasts() const
	{
		static ICVar	*pVar = gEnv->pConsole->GetCVar("g_groundeffectsdebug");

		CRY_ASSERT(pVar);

		return pVar->GetIVal() != 2;
	}

	IEntity					*m_pEntity;

	IParticleEffect	*m_pParticleEffect;  

	int							m_flags, m_slot, m_surfaceIdx, m_rayWorldIntersectSurfaceIdx;
	QueuedRayID			m_raycastID;

	string					m_interaction;

	bool						m_active									: 1;

	bool						m_stopped									: 1;

	bool						m_validRayWorldIntersect	: 1;

	float						m_height, m_rayWorldIntersectHeight, m_ratio;
	
	float 					m_sizeScale, m_sizeGoal;
  
	float 					m_countScale;
  
	float 					m_speedScale, m_speedGoal;
  
	float 					m_interpSpeed;
  
	float 					m_maxHeightCountScale, m_maxHeightSizeScale;
};
#endif //__GROUNDEFFECT_H__
