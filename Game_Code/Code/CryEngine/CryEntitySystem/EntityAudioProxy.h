// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef ENTITY_AUDIO_PROXY_H_INCLUDED
#define ENTITY_AUDIO_PROXY_H_INCLUDED

#include <IAudioSystem.h>
#include <STLPoolAllocator.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles audio on the entity.
//////////////////////////////////////////////////////////////////////////
struct CEntityAudioProxy : public IEntityAudioProxy
{
public:

	CEntityAudioProxy();
	virtual ~CEntityAudioProxy();

	// IComponent
	VIRTUAL	void											ProcessEvent(SEntityEvent& event);
	VIRTUAL void											Initialize(SComponentInitializer const& init);
	// ~IComponent

	// IEntityProxy
	VIRTUAL EEntityProxy							GetType() {return ENTITY_PROXY_AUDIO;}
	VIRTUAL void											Release();
	VIRTUAL void											Done();
	VIRTUAL	void											Update(SEntityUpdateContext& ctx) {}
	VIRTUAL bool											Init(IEntity* pEntity, SEntitySpawnParams& params ) {return true;}
	VIRTUAL void											Reload(IEntity* pEntity, SEntitySpawnParams& params);
	VIRTUAL void											SerializeXML(XmlNodeRef &entityNode,bool bLoading) {}
	VIRTUAL void											Serialize(TSerialize ser);
	VIRTUAL bool											NeedSerialize() {return false;}
	VIRTUAL bool											GetSignature(TSerialize signature);
	VIRTUAL void											GetMemoryUsage(ICrySizer* pSizer) const {pSizer->AddObject(this, sizeof(*this));}
	// ~IEntityProxy

	// IEntityAudioProxy
	VIRTUAL void											SetFadeDistance(float const fFadeDistance) {m_fFadeDistance = fFadeDistance;}
	VIRTUAL float											GetFadeDistance() const {return m_fFadeDistance;}
	VIRTUAL void											SetEnvironmentFadeDistance(float const fEnvironmentFadeDistance) {m_fEnvironmentFadeDistance = fEnvironmentFadeDistance;}
	VIRTUAL float											GetEnvironmentFadeDistance() const {return m_fEnvironmentFadeDistance;}
	VIRTUAL void											SetEnvironmentID(TAudioEnvironmentID const nEnvironmentID) {m_nAudioEnvironmentID = nEnvironmentID;}
	VIRTUAL TAudioEnvironmentID				GetEnvironmentID() const {return m_nAudioEnvironmentID;}
	VIRTUAL TAudioProxyID							CreateAuxAudioProxy();
	VIRTUAL bool											RemoveAuxAudioProxy(TAudioProxyID const nAudioProxyLocalID);
	VIRTUAL void											SetAuxAudioProxyOffset(SATLWorldPosition const& rOffset, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL SATLWorldPosition const&	GetAuxAudioProxyOffset(TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											ExecuteTrigger(TAudioControlID const nID, ELipSyncMethod const eLipSyncMethod, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											ExecuteTrigger(TAudioControlID const nID, ELipSyncMethod const eLipSyncMethod, TTriggerFinishedCallback const pCallback, void* const pCallbackCookie, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											StopTrigger(TAudioControlID const nID, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											SetSwitchState(TAudioControlID const nSwitchID, TAudioSwitchStateID const nStateID, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											SetRtpcValue(TAudioControlID const nRtpcID, float const fValue, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											SetObstructionCalcType(EAudioObjectObstructionCalcType const eObstructionType, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											SetEnvironmentAmount(TAudioEnvironmentID const nEnvironmentID, float const fAmount, TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	VIRTUAL void											SetCurrentEnvironments(TAudioProxyID const nAudioProxyLocalID = DEFAULT_AUDIO_PROXY_ID);
	// ~IEntityAudioProxy

private:

	struct SAudioProxyWrapper
	{
		SAudioProxyWrapper(IAudioProxy* const pPassedIAudioProxy)
			:	pIAudioProxy(pPassedIAudioProxy)
		{}

		~SAudioProxyWrapper() {}

		IAudioProxy* const	pIAudioProxy;
		SATLWorldPosition		oOffset;

	private:

		SAudioProxyWrapper()
		:	pIAudioProxy(NULL)
		{}
	};

	typedef std::pair<TAudioProxyID const, SAudioProxyWrapper> TAudioProxyPair;
	typedef std::map<TAudioProxyID const, SAudioProxyWrapper, std::less<TAudioProxyID>, stl::STLPoolAllocator<TAudioProxyPair > >	TAuxAudioProxies;
	static TAudioProxyPair s_oNULLAudioProxyPair;
	static SATLWorldPosition const s_oNULLOffset;

	void							OnListenerEnter(IEntity const* const pEntity);
	void							OnListenerMoveNear(IEntity const* const __restrict pEntity, IEntity const* const __restrict pArea);
	void							OnListenerMoveInside(IEntity const* const pEntity);
	void							OnListenerExclusiveMoveInside(IEntity const* const __restrict pEntity, IEntity const* const __restrict pAreaHigh, IEntity const* const __restrict pAreaLow, float const fFade);
	void							OnMove();
	void							OnEnter(IEntity const* const pEntity);
	void							OnLeaveNear(IEntity const* const pEntity);
	void							OnMoveNear(IEntity const* const __restrict pEntity, IEntity const* const __restrict pArea);
	void							OnMoveInside(IEntity const* const pEntity);
	void							OnExclusiveMoveInside(IEntity const* const __restrict pEntity, IEntity const* const __restrict pEntityAreaHigh, IEntity const* const __restrict pEntityAreaLow, float const fFade);
	void							OnAreaCrossing();
	void							OnHide(bool const bHide);
	void							PrecacheHeadBone();
	TAudioProxyPair&	GetAuxAudioProxyPair(TAudioProxyID const nAudioProxyLocalID);
	void							ExecuteTriggerInternal(TAudioControlID const nID, ELipSyncMethod const eLipSyncMethod, TTriggerFinishedCallback const pCallback, void* const pCallbackCookie, TAudioProxyID const nAudioProxyLocalID);
	void							SetEnvironmentAmountInternal(IEntity const* const pEntity, float const fAmount) const;

	// Function objects
	struct SReleaseAudioProxy
	{
		SReleaseAudioProxy() {}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->Release();
		}
	};

	struct SResetAudioProxy
	{
		SResetAudioProxy() {}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->Reset();
		}
	};

	struct SInitializeAudioProxy
	{
		SInitializeAudioProxy(char const* const sPassedObjectName)
			:	sObjectName(sPassedObjectName)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->Initialize(sObjectName);
		}

	private:

		char const* const sObjectName;
	};

	struct SRepositionAudioProxy
	{
		SRepositionAudioProxy(SATLWorldPosition const& rPassedPosition)
			:	rPosition(rPassedPosition)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetPosition(SATLWorldPosition(rPosition.mPosition * rPair.second.oOffset.mPosition, ZERO));
		}

	private:

		SATLWorldPosition const& rPosition;
	};

	struct SExecuteTrigger
	{
		SExecuteTrigger(TAudioControlID const nPassedID, ELipSyncMethod const ePassedLipSyncMethod, TTriggerFinishedCallback const pPassedCallback, void* const pPassedCallbackCookie)
			:	nTriggerID(nPassedID)
			,	eLipSyncMethod(ePassedLipSyncMethod)
			, pCallback(pPassedCallback)
			, pCallbackCookie(pPassedCallbackCookie)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->ExecuteTrigger(nTriggerID, eLipSyncMethod, pCallback, pCallbackCookie);
		}

	private:

		TAudioControlID const nTriggerID;
		ELipSyncMethod const eLipSyncMethod;
		TTriggerFinishedCallback const pCallback;
		void* const pCallbackCookie;
	};

	struct SStopTrigger
	{
		SStopTrigger(TAudioControlID const nPassedID)
			:	nTriggerID(nPassedID)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->StopTrigger(nTriggerID);
		}

	private:
		
		TAudioControlID const nTriggerID;
	};

	struct SSetSwitchState
	{
		SSetSwitchState(TAudioControlID const nPassedSwitchID, TAudioSwitchStateID const nPassedStateID)
			:	nSwitchID(nPassedSwitchID)
			,	nStateID(nPassedStateID)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetSwitchState(nSwitchID, nStateID);
		}

	private:

		TAudioControlID const nSwitchID;
		TAudioSwitchStateID const nStateID;
	};

	struct SSetRtpcValue
	{
		SSetRtpcValue(TAudioControlID const nPassedRtpcID, float const fPassedValue)
			:	nRtpcID(nPassedRtpcID)
			,	fValue(fPassedValue)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetRtpcValue(nRtpcID, fValue);
		}

	private:

		TAudioControlID const nRtpcID;
		float const fValue;
	};

	struct SSetObstructionCalcType
	{
		SSetObstructionCalcType(EAudioObjectObstructionCalcType const ePassedObstructionType)
			:	eObstructionType(ePassedObstructionType)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetObstructionCalcType(eObstructionType);
		}

	private:

		EAudioObjectObstructionCalcType const eObstructionType;
	};

	struct SSetEnvironmentAmount
	{
		SSetEnvironmentAmount(TAudioEnvironmentID const nPassedEnvironmentID, float const fPassedAmount)
			:	nEnvironmentID(nPassedEnvironmentID)
			,	fAmount(fPassedAmount)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetEnvironmentAmount(nEnvironmentID, fAmount);
		}

	private:

		TAudioEnvironmentID const nEnvironmentID;
		float const fAmount;
	};

	struct SSetCurrentEnvironments
	{
		SSetCurrentEnvironments(EntityId const nEntityID)
		:	m_nEntityID(nEntityID)
		{}

		inline void operator()(TAudioProxyPair const& rPair)
		{
			rPair.second.pIAudioProxy->SetCurrentEnvironments(m_nEntityID);
		}

	private:

		EntityId const m_nEntityID;
	};

	struct SSetAuxAudioProxyOffset
	{
		SSetAuxAudioProxyOffset(SATLWorldPosition const& rPassedOffset, Matrix34 const& rPassedEntityPosition)
		:	rOffset(rPassedOffset)
		,	rEntityPosition(rPassedEntityPosition)
		{}

		inline void operator()(TAudioProxyPair& rPair)
		{
			Matrix34 const oOffset(IDENTITY, rOffset.GetPositionVec());
			SATLWorldPosition const oPosition(rEntityPosition * oOffset, ZERO);
			rPair.second.oOffset = rOffset;
			rPair.second.pIAudioProxy->SetPosition(oPosition);
		}

	private:

		SATLWorldPosition const& rOffset;
		Matrix34 const& rEntityPosition;
	};
	// ~Function objects

	TAuxAudioProxies		m_mapAuxAudioProxies;
	TAudioProxyID				m_nAudioProxyIDCounter;

	TAudioEnvironmentID	m_nAudioEnvironmentID;
	CEntity*						m_pEntity;
	bool								m_bHide;

	float								m_fFadeDistance;
	float								m_fEnvironmentFadeDistance;

	// Head members for voice files
	int									m_nBoneHead;
	int									m_nAttachmentIndex;
};

#endif // ENTITY_AUDIO_PROXY_H_INCLUDED
