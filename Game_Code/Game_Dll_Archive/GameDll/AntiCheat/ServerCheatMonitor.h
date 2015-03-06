/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Central storage and processing of all gamecode based server  
					   side monitoring. By centralising, the system can load balance,
						 share data, and limit the intrusiveness into general game code. 
						 General code is only required to supply samples of data which 
						 this monitor processes and if required flags any suspicious
						 activity with the IAntiCheatManager interface.

-------------------------------------------------------------------------
History:
- 09:11:2011: Created by Stewart Needham

*************************************************************************/
#ifndef ___SERVER_CHEAT_MONITOR_H___
#define ___SERVER_CHEAT_MONITOR_H___

#include "AntiCheat/AnticheatDefines.h"


struct HitInfo;
struct INetChannel;
class CServerPlayerTracker;

#include "Actor.h"

#if SERVER_CHEAT_MONITOR_ENABLED
#include <IGameFramework.h>
#include "RecordingSystemDefines.h"

//struct SPlayerLocomotionSample;

class CServerCheatMonitor : public IGameFrameworkListener
{
public:

								CServerCheatMonitor();
							 ~CServerCheatMonitor();

					void	Init(IGameFramework *pFramework);
					 
	// IGameFrameworkListener
	virtual void	OnPostUpdate(float fDeltaTime);
	virtual void	OnSaveGame(ISaveGame* pSaveGame);
	virtual void	OnLoadGame(ILoadGame* pLoadGame);
	virtual void	OnLevelEnd(const char* nextLevel);
	virtual void	OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	void KillOccurred(const HitInfo &hitInfo, uint16 victimChannelId, bool bExpectKillCam);
	void StoreKillcamDataPart(IActor * pActor, const CActor::KillCamFPData& packet);
	ILINE SPlaybackInstanceData& GetPlaybackInstanceData() { return m_KillcamPlaybackInstanceData; }

	void NewCloseCombatSnapTarget(const IActor& rPlayer, const IActor * pTargetActor);
	bool EvaluateHitValidity(INetChannel * pNetChannel, const HitInfo& rHitInfo) const;

	void OnClientConnect(uint16 channelId);
	void OnClientDisconnect(uint16 channelId, IActor * pActor);
	void OnPlayerRevived(EntityId player);

#ifndef _RELEASE
	void DebugBulletWorldIntersection();
#endif

	static const float kMaxHorizontalPlayerSpeed;

	static bool ValidateTargetActorPositionAgainstHit(IActor& rTargetActor, const HitInfo& rHitInfo, float fNetLagSeconds);
	static void GetHitValidationInfo(const IActor& pShooterActor, const HitInfo& hitInfo, float& fDistance2D, float& fDistanceMax, float& fNetLagSeconds);

					//void	UpdateLocomotion( const SPlayerLocomotionSample& sample );
private:
	bool EvaluateHitLineOfSight(IActor* pActor, const HitInfo& rHitInfo, float &stage) const;
	bool BulletWorldIntersection(const Vec3 &org, const Vec3 &dir, int pierceability, ray_hit &hit) const;
	bool LineSegIntersectsStatObj(IRenderNode *pRenderNode, const Vec3 &orig, const Vec3 &dest, Vec3 &hitPos, int &surfaceTypeId) const;
#ifndef _RELEASE
	static void CmdLoadRayCasts(IConsoleCmdArgs* pArgs);
#endif

	enum ECustomValidationResult
	{
		eCVR_NoCustomValidation						= BIT(0),
		eCVR_Failed												= BIT(1),
		eCVR_Passed_SkipNormalValidation	= BIT(2),
		eCVR_Passed_DoNormalValidation		= BIT(3)
	};

	IGameFramework* m_pFramework;

	SPlaybackInstanceData  m_KillcamPlaybackInstanceData;

	CServerPlayerTracker * GetServerPlayerTracker(EntityId player) const;

	ECustomValidationResult CustomValidation(uint16 channelId, IActor& rActor, const HitInfo& rHitInfo) const;
	ECustomValidationResult CustomValidation_EnvironmentalMelee(  uint16 channelId, IActor& rActor, const HitInfo& rHitInfo ) const;
};

#else

class CServerCheatMonitor {
public:
	ILINE void StoreKillcamDataPart(IActor * pActor, const CActor::KillCamFPData& packet)	{}
	ILINE void KillOccurred(const HitInfo &hitInfo, uint16 victimChannelId, bool bExpectKillCam)					{}
	ILINE bool EvaluateHitValidity(INetChannel * pNetChannel, const HitInfo& rHitInfo) const { return true; }
	ILINE void ReceiveFirstPersonDataPart(EntityId victimId, EntityId fromId, uint8 *data, int part, int numparts, size_t datasize) {}
	ILINE void OnClientConnect(uint16 channelId) {}
	ILINE void OnClientDisconnect(uint16 channelId, IActor * pActor) {}
	ILINE void NewCloseCombatSnapTarget(const IActor& rPlayer, const IActor * pTargetActor) {}
	ILINE void OnPlayerRevived(EntityId player) { }

	ILINE static bool ValidateTargetActorPositionAgainstHit(IActor& rTargetActor, const HitInfo& rHitInfo, float fNetLagSeconds) { return true; }
};

#endif //SERVER_CHEAT_MONITOR_ENABLED
#endif // ___SERVER_CHEAT_MONITOR_H___
