/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	A module for servers to use to track information about
							players for the purposes of cheat detection

-------------------------------------------------------------------------
History:
- 16:02:2012: Created by Richard Semmens

*************************************************************************/
#ifndef ___SERVER_PLAYER_TRACKER_H___
#define ___SERVER_PLAYER_TRACKER_H___

#include "ServerCheatMonitor.h"

#if SERVER_CHEAT_MONITOR_ENABLED

#include "RecordingSystemDefines.h"

class CServerPlayerTracker : public IGameObjectExtension, public stl::intrusive_linked_list_node<CServerPlayerTracker> 
{
public:
	CServerPlayerTracker();
	~CServerPlayerTracker();

	virtual bool Init( IGameObject * pGameObject );
	virtual void PostInit( IGameObject * pGameObject )	{}
	virtual void InitClient(int channelId)							{}
	virtual void PostInitClient(int channelId)					{}

	static void GetGameObjectExtensionRMIData( void ** ppRMI, size_t * nCount )
	{
		*ppRMI = NULL;
		*nCount = 0;
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual bool ReloadExtension( IGameObject * pGameObject, const SEntitySpawnParams &params )			{ return true; }
	virtual void PostReloadExtension( IGameObject * pGameObject, const SEntitySpawnParams &params ) {}
	virtual bool GetEntityPoolSignature( TSerialize signature )																			{ return false; }

	virtual void Release()																																				{ delete this; }
	virtual void FullSerialize( TSerialize ser )																									{}
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags ) { return true; }
	virtual void PostSerialize()																																	{}

	virtual void SerializeSpawnInfo( TSerialize ser ) {}
	virtual ISerializableInfoPtr GetSpawnInfo()				{ return NULL; }

	virtual void Update( SEntityUpdateContext& ctx, int updateSlot ) {};

	static void UpdateAllPlayerTrackers(float fFrameTime);
	void UpdateTracker(float fCurrentTime);

	static void OnClientDisconnect( uint16 channelId );
	void ClientDisconnected( uint16 channelId );

	void OnPlayerRevived();

	virtual void HandleEvent( const SGameObjectEvent& event ) {}
	virtual void ProcessEvent( SEntityEvent& event )					{}

	virtual void SetChannelId(uint16 id)		{}
	virtual void SetAuthority( bool auth )	{}

	virtual void PostUpdate( float frameTime )	{}
	virtual void PostRemoteSpawn()							{}	
	const void * GetRMIBase() const	{	return NULL; }

	void KillCamDataReceived(uint16 victimChannelId);
	void KillOccurred(const HitInfo &hitInfo, uint16 victimChannelId, bool bExpectKillCam);
	
	void NewCloseCombatSnapTarget(const IActor * pTargetActor);
	bool DidTrackTargetDuringKillcam(EntityId victim, uint16 victimChannelId);
	const Vec3& GetLastKillDirection(uint16 victimChannelId);

	CKillCamDataStreamer& GetKillCamStreamer() { return m_killCamStreamer; }

private:
	struct SKillCamValidation{
		SKillCamValidation() : vLastKillDir(Vec3Constants<float>::fVec3_OneZ), uKills(0), uKillCams(0), fLastKillTime(0.0f), fLastKillCamTime(0.0f) {}
		Vec3		vLastKillDir;
		uint16	uKills;
		uint16	uKillCams;
		float		fLastKillTime;
		float		fLastKillCamTime; 
	};
	
	typedef std::map<uint16, SKillCamValidation> TKillCamValidationMap;

	struct SMeleeLockRecord {
		SMeleeLockRecord(uint16 _targetChannelId, float _fStartLock) : targetChannelId(_targetChannelId), fStartLock(_fStartLock), fEndLock(kInvalidEndLockTime) {}
		uint16		targetChannelId;
		float			fStartLock;
		float			fEndLock;
	};

	typedef std::deque<SMeleeLockRecord> TMeleeLockQueue;


	void CheckKillCamValidities(float fCurrentTime);
	void CheckKillCamSettings();
	SKillCamValidation& GetOrCreateVictimData(uint16 channelId);

	TKillCamValidationMap m_KillCamValidationMap;
	TMeleeLockQueue				m_MeleeLockQueue;

	int8  m_nOriginalKillCamSetting;
	int		m_nTotalHostileKills;
	static const float kInvalidEndLockTime;

	static float s_fLastTickTime;
	static const float kKillCamValidationCheckFrequency;

	CKillCamDataStreamer m_killCamStreamer;
};

#endif // SERVER_CHEAT_MONITOR_ENABLED
#endif //___SERVER_PLAYER_TRACKER_H___
