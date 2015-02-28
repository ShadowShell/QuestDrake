////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   LipSync_TransitionQueue.h
//  Version:     v1.00
//  Created:     2014-08-29 by Christian Werle.
//  Description: Automatic start of facial animation when a sound is being played back.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once


/// The CLipSyncProvider_TransitionQueue communicates with sound proxy to play synchronized facial animation directly on the transition queue
class CLipSyncProvider_TransitionQueue : public ILipSyncProvider
{
public:
	explicit CLipSyncProvider_TransitionQueue(EntityId entityId);

	// ILipSyncProvider
	void RequestLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	void StartLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	void PauseLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	void UnpauseLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	void StopLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	void UpdateLipSync(IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod) OVERRIDE;
	// ~ILipSyncProvider

	void FullSerialize(TSerialize ser);
	void GetEntityPoolSignature(TSerialize signature);

private:
	IEntity* GetEntity();
	ICharacterInstance* GetCharacterInstance();
	void FindMatchingAnim(const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod, ICharacterInstance& character, int* pAnimIdOut, CryCharAnimationParams* pAnimParamsOut) const;
	void FillCharAnimationParams(const bool isDefaultAnim, CryCharAnimationParams* pParams) const;
	void SynchronizeAnimationToSound(const TAudioControlID nAudioTriggerId);

private:
	enum EState
	{
		eS_Init,
		eS_Requested,
		eS_Started,
		eS_Paused,
		eS_Unpaused,
		eS_Stopped,
	};

	static uint32 s_lastAnimationToken;

	EntityId m_entityId;
	int m_nCharacterSlot;           // the lip-sync animation will be played back on the character residing in this slot of the entity
	int m_nAnimLayer;               // the lip-sync animation will be played back on this layer of the character
	string m_sDefaultAnimName;      // fallback animation to play if there is no animation matching the currently playing sound in the character's database

	EState m_state;
	bool m_isSynchronized;

	// Filled when request comes in:
	int m_requestedAnimId;
	CryCharAnimationParams m_requestedAnimParams;
	CAutoResourceCache_CAF m_cachedAnim;

	// Filled when animation is started:
	uint32 m_nCurrentAnimationToken;
	TAudioControlID m_soundId;
};
DECLARE_BOOST_POINTERS(CLipSyncProvider_TransitionQueue);


class CLipSync_TransitionQueue : public CGameObjectExtensionHelper<CLipSync_TransitionQueue, IGameObjectExtension>
{
public:
	// IGameObjectExtension
	virtual void GetMemoryUsage(ICrySizer *pSizer) const OVERRIDE;
	virtual bool Init(IGameObject* pGameObject) OVERRIDE;
	virtual void PostInit(IGameObject* pGameObject) OVERRIDE;
	virtual void InitClient(int channelId) OVERRIDE;
	virtual void PostInitClient(int channelId) OVERRIDE;
	virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) OVERRIDE;
	virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) OVERRIDE;
	virtual bool GetEntityPoolSignature(TSerialize signature) OVERRIDE;
	virtual void Release() OVERRIDE;
	virtual void FullSerialize(TSerialize ser) OVERRIDE;
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) OVERRIDE;
	virtual void PostSerialize() OVERRIDE;
	virtual void SerializeSpawnInfo(TSerialize ser) OVERRIDE;
	virtual ISerializableInfoPtr GetSpawnInfo() OVERRIDE;
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) OVERRIDE;
	virtual void HandleEvent(const SGameObjectEvent& event) OVERRIDE;
	virtual void ProcessEvent(SEntityEvent& event) OVERRIDE;
	virtual void SetChannelId(uint16 id) OVERRIDE;
	virtual void SetAuthority(bool auth) OVERRIDE;
	virtual void PostUpdate(float frameTime) OVERRIDE;
	virtual void PostRemoteSpawn() OVERRIDE;
	// ~IGameObjectExtension

private:
	void InjectLipSyncProvider();

private:
	CLipSyncProvider_TransitionQueuePtr m_pLipSyncProvider;
};
