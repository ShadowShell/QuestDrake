#ifndef ___SHOUTCOUNTER_H___
#define ___SHOUTCOUNTER_H___

#ifdef SERVER_CHECKS

#include "IGameRulesSystem.h"

class CShotCounter : public stl::intrusive_linked_list_node<CShotCounter> {

	typedef f32 THitCount;

public:

	CShotCounter(const EntityId ownerPlayer);
	~CShotCounter();

	// Record input
	void RecordHit( const HitInfo &hitInfo, const bool isHeadshot );
	ILINE void RecordShot() { m_shotCount++; }
	//~Record input

	//Evaluate
	void EvaluateCheatState();
	void EvaluateTooManyHeadShots();
	//~Evaluate

	static void Update(float fDeltaTime);

private :
	struct HitInfoWithDir
	{
		HitInfoWithDir() { bInitialized = false; }
		HitInfoWithDir(const HitInfo& _hitInfo, const Vec3& _dir) : hitInfo(_hitInfo), dir(_dir), bInitialized(true) {}
		HitInfo hitInfo;
		Vec3 dir;
		bool bInitialized;
	};

	HitInfoWithDir m_LastHit;
	EntityId m_ownerPlayer;

	THitCount m_headshotHits;
	THitCount m_hits;
	THitCount m_shotCount;

	int m_longestHeadshotStreak;  // these are global values, across all weapons for this player
	int m_currentHeadshotStreak;

	bool m_changeSinceLastCheck;
};

#endif // SERVER_CHECKS

#endif // ___SHOUTCOUNTER_H___