// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef IAUDIOSYSTEM_H_INCLUDED
#define IAUDIOSYSTEM_H_INCLUDED

#include "IAudioInterfacesCommonData.h"
#include "ILipSyncProvider.h"

// General macros.
#define AUDIO_STRINGANIZE2(x) #x
#define AUDIO_STRINGANIZE1(x) AUDIO_STRINGANIZE2(x)

#if defined(WIN32) || defined (DURANGO)
#define TODO(y) __pragma(message(__FILE__ "(" AUDIO_STRINGANIZE1(__LINE__) ") : " "[AUDIO] TODO >>> " AUDIO_STRINGANIZE2(y)))
#define REINST_FULL(y) __pragma(message(__FILE__ "(" AUDIO_STRINGANIZE1(__LINE__) ") : " "[AUDIO] REINST " __FUNCSIG__ " >>> " AUDIO_STRINGANIZE2(y)))
#define REINST(y) __pragma(message(__FILE__ "(" AUDIO_STRINGANIZE1(__LINE__) ") : " "[AUDIO] REINST " __FUNCTION__ " >>> " AUDIO_STRINGANIZE2(y)))
#elif defined(ORBIS) || defined(MAC) || defined(LINUX) || defined(CAFE)
#define TODO(y)
#define REINST_FULL(y)
#define REINST(y)
#else
#error "Undefined platform."
#endif

// Note:
//	 Need this explicit here to prevent circular includes to IEntity.
// Summary:
//	 Unique identifier for each entity instance.
typedef unsigned int EntityId;

// Forward declarations.
struct	IVisArea;
struct	SAudioRequest;
struct	IMusicSystem;
struct	ICVar;
struct	IAudioSystemImplementation;
struct	IAudioCallbackManager;

enum EATLDataScope ATL_ENUM_TYPE
{
	eADS_NONE						= 0,
	eADS_GLOBAL					= 1,
	eADS_LEVEL_SPECIFIC	= 2,
	eADS_ALL						= 3,
};

enum EAudioManagerRequestType ATL_ENUM_TYPE
{
	eAMRT_NONE,
	eAMRT_INIT_AUDIO_SYSTEM,
	eAMRT_SHUTDOWN_AUDIO_SYSTEM,
	eAMRT_REFRESH_AUDIO_SYSTEM,
	eAMRT_RESERVE_AUDIO_OBJECT_ID,
	eAMRT_LOSE_FOCUS,
	eAMRT_GET_FOCUS,
	eAMRT_MUTE_ALL,
	eAMRT_UNMUTE_ALL,
	eAMRT_STOP_ALL_SOUNDS,
	eAMRT_PARSE_CONTROLS_DATA,
	eAMRT_PARSE_PRELOADS_DATA,
	eAMRT_CLEAR_CONTROLS_DATA,
	eAMRT_CLEAR_PRELOADS_DATA,
	eAMRT_PRELOAD_SINGLE_REQUEST,
	eAMRT_UNLOAD_SINGLE_REQUEST,
	eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE,
	eAMRT_DRAW_DEBUG_INFO,					// Only used internally!
	eAMRT_ADD_REQUEST_LISTENER,
	eAMRT_REMOVE_REQUEST_LISTENER,
	eAMRT_CHANGE_LANGUAGE,
};

enum EAudioCallbackManagerRequestType ATL_ENUM_TYPE
{
	eACMRT_NONE,
	eACMRT_REPORT_FINISHED_EVENT,							// Only used internally!
	eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY,	// Only used internally!
};

enum EAudioListenerRequestType ATL_ENUM_TYPE
{
	eALRT_NONE = 0,
	eALRT_SET_POSITION,
};

enum EAudioObjectRequestType ATL_ENUM_TYPE
{
	eAORT_NONE = 0,
	eAORT_PREPARE_TRIGGER,
	eAORT_UNPREPARE_TRIGGER,
	eAORT_EXECUTE_TRIGGER,
	eAORT_STOP_TRIGGER,
	eAORT_STOP_ALL_TRIGGERS,

	eAORT_SET_POSITION,
	eAORT_SET_RTPC_VALUE,
	eAORT_SET_SWITCH_STATE,
	eAORT_SET_VOLUME,
	eAORT_SET_ENVIRONMENT_AMOUNT,
	eAORT_RESET_ENVIRONMENTS,

	eAORT_RELEASE_OBJECT,
};

enum EAudioObjectObstructionCalcType ATL_ENUM_TYPE
{
	eAOOCT_NONE					= 0,
	eAOOCT_IGNORE				= 1,
	eAOOCT_SINGLE_RAY		= 2,
	eAOOCT_MULTI_RAY		= 3,

	eAOOCT_COUNT,
};

enum EAudioTriggerFinishedCallbackFlags ATL_ENUM_TYPE
{
	eATFCF_CALL_ON_AUDIO_THREAD = BIT(0),
};

enum EAudioControlType ATL_ENUM_TYPE
{
	eACT_NONE					= 0,
	eACT_AUDIO_OBJECT	= 1,
	eACT_TRIGGER			= 2,
	eACT_RTPC					= 3,
	eACT_SWITCH				= 4,
	eACT_SWITCH_STATE	= 5,
	eACT_PRELOAD			= 6,
	eACT_ENVIRONMENT	= 7,
};

typedef void (*TTriggerFinishedCallback)(TAudioObjectID const, TAudioControlID const, void* const);

struct SAudioTriggerFinishedCallbackData
{
	explicit SAudioTriggerFinishedCallbackData(TTriggerFinishedCallback const pPassedCallback, void* const pPassedCookie = NULL)
		: pCallback(pPassedCallback)
		, pCookie(pPassedCookie)
		, nFlags(0)
	{}

	TTriggerFinishedCallback	pCallback;
	void*											pCookie;
	TATLEnumFlagsType					nFlags;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioManagerRequestDataBase : public SAudioRequestDataBase
{
	explicit SAudioManagerRequestDataBase(EAudioManagerRequestType const ePassedType = eAMRT_NONE)
		: SAudioRequestDataBase(eART_AUDIO_MANAGER_REQUEST)
		, eType(ePassedType)
	{}

	EAudioManagerRequestType const eType;
};

//////////////////////////////////////////////////////////////////////////
template <EAudioManagerRequestType T>
struct SAudioManagerRequestData : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(T)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_RESERVE_AUDIO_OBJECT_ID)
		, pObjectID(NPTR)
		, sObjectName(NPTR)
	{}

	SAudioManagerRequestData(TAudioObjectID* const pPassedObjectID, char const* const sPassedObjectName = NPTR)
		: SAudioManagerRequestDataBase(eAMRT_RESERVE_AUDIO_OBJECT_ID)
		, pObjectID(pPassedObjectID)
		, sObjectName(sPassedObjectName)
	{}

	TAudioObjectID* pObjectID;
	char const*			sObjectName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_ADD_REQUEST_LISTENER> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_ADD_REQUEST_LISTENER)
		, pRequestOwner(NPTR)
		, func(NPTR)
	{}

	SAudioManagerRequestData(void const* const pPassedRequestOwner, void (*passedfunc)(SAudioRequestInfo const* const))
		: SAudioManagerRequestDataBase(eAMRT_ADD_REQUEST_LISTENER)
		, pRequestOwner(pPassedRequestOwner)
		, func(passedfunc)
	{}

	void const*	pRequestOwner;
	void				(*func)(SAudioRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_REMOVE_REQUEST_LISTENER> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_REMOVE_REQUEST_LISTENER)
		, pRequestOwner(NPTR)
		, func(NPTR)
	{}

	SAudioManagerRequestData(void const* const pPassedRequestOwner, void (*passedfunc)(SAudioRequestInfo const* const))
		: SAudioManagerRequestDataBase(eAMRT_REMOVE_REQUEST_LISTENER)
		, pRequestOwner(pPassedRequestOwner)
		, func(passedfunc)
	{}

	void const*	pRequestOwner;
	void				(*func)(SAudioRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_PARSE_CONTROLS_DATA)
		, sFolderPath(NPTR)
	{}

	SAudioManagerRequestData(char const* const sConfigFolderPath, EATLDataScope const ePassedDataScope)
		: SAudioManagerRequestDataBase(eAMRT_PARSE_CONTROLS_DATA)
		, sFolderPath(sConfigFolderPath)
		, eDataScope(ePassedDataScope)
	{}

	char const*		sFolderPath;
	EATLDataScope	eDataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_PARSE_PRELOADS_DATA> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_PARSE_PRELOADS_DATA)
		, sFolderPath(NPTR)
	{}

	SAudioManagerRequestData(char const* const sConfigFolderPath, EATLDataScope const ePassedDataScope)
		: SAudioManagerRequestDataBase(eAMRT_PARSE_PRELOADS_DATA)
		, sFolderPath(sConfigFolderPath)
		, eDataScope(ePassedDataScope)
	{}

	char const*		sFolderPath;
	EATLDataScope	eDataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData(EATLDataScope const ePassedDataScope = eADS_NONE)
		: SAudioManagerRequestDataBase(eAMRT_CLEAR_CONTROLS_DATA)
		, eDataScope(ePassedDataScope)
	{}

	EATLDataScope	const eDataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_CLEAR_PRELOADS_DATA> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData(EATLDataScope const ePassedDataScope = eADS_NONE)
		: SAudioManagerRequestDataBase(eAMRT_CLEAR_PRELOADS_DATA)
		, eDataScope(ePassedDataScope)
	{}

	EATLDataScope	const eDataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_PRELOAD_SINGLE_REQUEST> : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(TAudioPreloadRequestID const nRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID, bool const bPassedAutoLoadOnly = false)
		: SAudioManagerRequestDataBase(eAMRT_PRELOAD_SINGLE_REQUEST)
		, nPreloadRequestID(nRequestID)
		, bAutoLoadOnly(bPassedAutoLoadOnly)
	{}

	TAudioPreloadRequestID const nPreloadRequestID;
	bool const bAutoLoadOnly;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_UNLOAD_SINGLE_REQUEST> : public SAudioManagerRequestDataBase
{
	explicit SAudioManagerRequestData(TAudioPreloadRequestID const nRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID)
		: SAudioManagerRequestDataBase(eAMRT_UNLOAD_SINGLE_REQUEST)
		, nPreloadRequestID(nRequestID)
	{}

	TAudioPreloadRequestID const nPreloadRequestID;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData(EATLDataScope const eScope = eADS_NONE)
		: SAudioManagerRequestDataBase(eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE)
		, eDataScope(eScope)
	{}

	EATLDataScope	const eDataScope;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_REFRESH_AUDIO_SYSTEM> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_REFRESH_AUDIO_SYSTEM)
		, sLevelName(NPTR)
	{}

	SAudioManagerRequestData(char const* const sPassedLevelName)
		: SAudioManagerRequestDataBase(eAMRT_REFRESH_AUDIO_SYSTEM)
		, sLevelName(sPassedLevelName)
	{}

	char const* sLevelName;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioManagerRequestData<eAMRT_CHANGE_LANGUAGE> : public SAudioManagerRequestDataBase
{
	SAudioManagerRequestData()
		: SAudioManagerRequestDataBase(eAMRT_CHANGE_LANGUAGE)
	{}
};

//////////////////////////////////////////////////////////////////////////
struct SAudioCallbackManagerRequestDataBase : public SAudioRequestDataBase
{
	explicit SAudioCallbackManagerRequestDataBase(EAudioCallbackManagerRequestType const ePassedType = eACMRT_NONE)
		: SAudioRequestDataBase(eART_AUDIO_CALLBACK_MANAGER_REQUEST)
		, eType(ePassedType)
	{}

	EAudioCallbackManagerRequestType const eType;
};

//////////////////////////////////////////////////////////////////////////
template <EAudioCallbackManagerRequestType T>
struct SAudioCallbackManagerRequestData : public SAudioCallbackManagerRequestDataBase
{
	SAudioCallbackManagerRequestData()
		: SAudioCallbackManagerRequestDataBase(T)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_EVENT> : public SAudioCallbackManagerRequestDataBase
{
	SAudioCallbackManagerRequestData()
		: SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_FINISHED_EVENT)
		, nEventID(INVALID_AUDIO_EVENT_ID)
		, bSuccess(false)
	{}

	SAudioCallbackManagerRequestData(TAudioEventID const nPassedEventID, bool const bPassedSuccess)
		: SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_FINISHED_EVENT)
		, nEventID(nPassedEventID)
		, bSuccess(bPassedSuccess)
	{}

	TAudioEventID	nEventID;
	bool					bSuccess;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioCallbackManagerRequestData<eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY> : public SAudioCallbackManagerRequestDataBase
{
	explicit SAudioCallbackManagerRequestData(TAudioObjectID const nPassedObjectID, size_t const nPassedRayID = (size_t)-1)
		: SAudioCallbackManagerRequestDataBase(eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY)
		, nObjectID(nPassedObjectID)
		, nRayID(nPassedRayID)
	{}

	TAudioObjectID	nObjectID;
	size_t					nRayID;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioObjectRequestDataBase : public SAudioRequestDataBase
{
	explicit SAudioObjectRequestDataBase(EAudioObjectRequestType const ePassedType = eAORT_NONE)
		: SAudioRequestDataBase(eART_AUDIO_OBJECT_REQUEST)
		, eType(ePassedType)
	{}

	EAudioObjectRequestType const eType;
};

//////////////////////////////////////////////////////////////////////////
template <EAudioObjectRequestType T>
struct SAudioObjectRequestData : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(T)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_EXECUTE_TRIGGER)
		, nTriggerID(INVALID_AUDIO_CONTROL_ID)
		, fTimeUntilRemovalInMS(0.0f)
		, eLipSyncMethod(eLSM_None)
		, pCallbackCookie(NULL)
		, pCallback(NULL)
		, nFlags(0)
	{}

	SAudioObjectRequestData(TAudioControlID const nPassedTriggerID, float const fPassedTimeUntilRemovalInMS)
		: SAudioObjectRequestDataBase(eAORT_EXECUTE_TRIGGER)
		, nTriggerID(nPassedTriggerID)
		, fTimeUntilRemovalInMS(fPassedTimeUntilRemovalInMS)
		, eLipSyncMethod(eLSM_None)
		, pCallbackCookie(NULL)
		, pCallback(NULL)
		, nFlags(0)
	{}

	SAudioObjectRequestData(TAudioControlID const nPassedTriggerID, float const fPassedTimeUntilRemovalInMS, SAudioTriggerFinishedCallbackData const& rPassedCallbackData)
		: SAudioObjectRequestDataBase(eAORT_EXECUTE_TRIGGER)
		, nTriggerID(nPassedTriggerID)
		, fTimeUntilRemovalInMS(fPassedTimeUntilRemovalInMS)
		, eLipSyncMethod(eLSM_None)
		, pCallback(rPassedCallbackData.pCallback)
		, pCallbackCookie(rPassedCallbackData.pCookie)
		, nFlags(rPassedCallbackData.nFlags)
	{}

	TAudioControlID						nTriggerID;
	float											fTimeUntilRemovalInMS;
	ELipSyncMethod						eLipSyncMethod;
	TTriggerFinishedCallback	pCallback;
	void*											pCallbackCookie;
	TATLEnumFlagsType					nFlags;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_PREPARE_TRIGGER> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_PREPARE_TRIGGER)
		, nTriggerID(INVALID_AUDIO_CONTROL_ID)
	{}

	explicit SAudioObjectRequestData(TAudioControlID const nPassedTriggerID)
		: SAudioObjectRequestDataBase(eAORT_PREPARE_TRIGGER)
		, nTriggerID(nPassedTriggerID)
	{}

	TAudioControlID nTriggerID;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_UNPREPARE_TRIGGER> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_UNPREPARE_TRIGGER)
		, nTriggerID(INVALID_AUDIO_CONTROL_ID)
	{}

	explicit SAudioObjectRequestData(TAudioControlID const nPassedTriggerID)
		: SAudioObjectRequestDataBase(eAORT_UNPREPARE_TRIGGER)
		, nTriggerID(nPassedTriggerID)
	{}

	TAudioControlID nTriggerID;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_STOP_TRIGGER> : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(TAudioControlID const nPassedTriggerID = INVALID_AUDIO_CONTROL_ID)
		: SAudioObjectRequestDataBase(eAORT_STOP_TRIGGER)
		, nTriggerID(nPassedTriggerID)
	{}

	TAudioControlID nTriggerID;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_STOP_ALL_TRIGGERS)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_SET_POSITION> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_SET_POSITION)
		, oPosition() 
	{}

	explicit SAudioObjectRequestData(SATLWorldPosition const& oPassedPosition)
		: SAudioObjectRequestDataBase(eAORT_SET_POSITION)
		, oPosition(oPassedPosition) 
	{}

	SATLWorldPosition oPosition;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_SET_RTPC_VALUE)
		, nControlID(INVALID_AUDIO_CONTROL_ID)
		, fValue(0.0f) 
	{}

	SAudioObjectRequestData(TAudioControlID const nPassedControlID, float const fValue)
		: SAudioObjectRequestDataBase(eAORT_SET_RTPC_VALUE)
		, nControlID(nPassedControlID)
		, fValue(fValue) 
	{}

	TAudioControlID	nControlID;
	float						fValue;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_SET_SWITCH_STATE)
		, nSwitchID(INVALID_AUDIO_CONTROL_ID)
		, nStateID(INVALID_AUDIO_SWITCH_STATE_ID) 
	{}

	SAudioObjectRequestData(TAudioControlID const nPassedControlID, TAudioSwitchStateID const nPassedStateID)
		: SAudioObjectRequestDataBase(eAORT_SET_SWITCH_STATE)
		, nSwitchID(nPassedControlID)
		, nStateID(nPassedStateID) 
	{}

	TAudioControlID			nSwitchID;
	TAudioSwitchStateID	nStateID;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_SET_VOLUME> : public SAudioObjectRequestDataBase
{
	explicit SAudioObjectRequestData(float const fPassedVolume = 1.0f)
		: SAudioObjectRequestDataBase(eAORT_SET_VOLUME)
		, fVolume(fPassedVolume)
	{}

	float fVolume;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_SET_ENVIRONMENT_AMOUNT)
		, nEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
		, fAmount(1.0f)
	{}

	SAudioObjectRequestData(TAudioEnvironmentID const nPassedEnvironmentID, float const fPassedAmount)
		: SAudioObjectRequestDataBase(eAORT_SET_ENVIRONMENT_AMOUNT)
		, nEnvironmentID(nPassedEnvironmentID)
		, fAmount(fPassedAmount)
	{}

	TAudioEnvironmentID	nEnvironmentID;
	float								fAmount;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_RESET_ENVIRONMENTS)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioObjectRequestData<eAORT_RELEASE_OBJECT> : public SAudioObjectRequestDataBase
{
	SAudioObjectRequestData()
		: SAudioObjectRequestDataBase(eAORT_RELEASE_OBJECT)
	{}
};

//////////////////////////////////////////////////////////////////////////
struct SAudioListenerRequestDataBase : public SAudioRequestDataBase
{
	explicit SAudioListenerRequestDataBase(EAudioListenerRequestType const ePassedType = eALRT_NONE)
		: SAudioRequestDataBase(eART_AUDIO_LISTENER_REQUEST)
		, eType(ePassedType)
	{}

	EAudioListenerRequestType const eType;
};

//////////////////////////////////////////////////////////////////////////
template<EAudioListenerRequestType T>
struct SAudioListenerRequestData : public SAudioListenerRequestDataBase
{
	SAudioListenerRequestData()
		: SAudioListenerRequestDataBase(T)
	{}
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SAudioListenerRequestData<eALRT_SET_POSITION> : public SAudioListenerRequestDataBase
{
	SAudioListenerRequestData()
		: SAudioListenerRequestDataBase(eALRT_SET_POSITION)
	{}

	explicit SAudioListenerRequestData(SATLWorldPosition const& oWorldPosition)
		: SAudioListenerRequestDataBase(eALRT_SET_POSITION)
		, oNewPosition(oWorldPosition)
	{}

	SAudioListenerRequestData(Matrix34 const& nMat, Vec3 const& vVel)
		: SAudioListenerRequestDataBase(eALRT_SET_POSITION)
		, oNewPosition(nMat, vVel)
	{}

	SATLWorldPosition	oNewPosition;
};

//////////////////////////////////////////////////////////////////////////
struct SAudioEventListener
{
	SAudioEventListener()
	:	pRequestOwner(NPTR),
		OnEvent(NPTR)
	{}

	void const*	pRequestOwner;
	void				(*OnEvent)(SAudioRequestInfo const* const);
};

//////////////////////////////////////////////////////////////////////////
struct SAudioSystemInfo
{
	SAudioSystemInfo()
	: nCountUsedAudioTriggers(0),
		nCountUnusedAudioTriggers(0),
		nCountUsedAudioEvents(0),
		nCountUnusedAudioEvents(0)
	{}

	size_t	nCountUsedAudioTriggers;
	size_t	nCountUnusedAudioTriggers;
	size_t	nCountUsedAudioEvents;
	size_t	nCountUnusedAudioEvents;

	Vec3		oListenerPos;
};

//////////////////////////////////////////////////////////////////////////
struct IAudioProxy
{

	virtual ~IAudioProxy() {}

	virtual void	Initialize(char const* const sObjectName, bool const bInitAsync = true) = 0;
	virtual void	Release() = 0;
	virtual void	Reset() = 0;
	virtual void	ExecuteTrigger(TAudioControlID const nTriggerID, ELipSyncMethod const eLipSyncMethod) = 0;
	virtual void	ExecuteTrigger(TAudioControlID const nTriggerID, ELipSyncMethod const eLipSyncMethod, TTriggerFinishedCallback const pCallback, void* const pCallbackCookie) = 0;
	virtual void	StopTrigger(TAudioControlID const nTriggerID) = 0;
	virtual void	SetSwitchState(TAudioControlID const nSwitchID, TAudioSwitchStateID const nStateID) = 0;
	virtual void	SetRtpcValue(TAudioControlID const nRtpcID, float const fValue) = 0;
	virtual void	SetObstructionCalcType(EAudioObjectObstructionCalcType const eObstructionType) = 0;
	virtual void	SetPosition(SATLWorldPosition const& rPosition) = 0;
	virtual void	SetPosition(Vec3 const& rPosition) = 0;
	virtual void	SetEnvironmentAmount(TAudioEnvironmentID const nEnvironmentID, float const fAmount) = 0;
	virtual void	SetCurrentEnvironments(EntityId const nEntityToIgnore = 0) = 0;
	virtual void	SetLipSyncProvider(ILipSyncProvider* const pILipSyncProvider) = 0;

};

//////////////////////////////////////////////////////////////////////////
struct IAudioSystem
{

	virtual ~IAudioSystem() {}

	virtual void										Initialize(IAudioSystemImplementation* const pImpl) = 0;
	virtual void										Release() = 0;
	virtual void										PushRequest(SAudioRequest const& rAudioRequestData) = 0;
	virtual void										AddRequestListener(void (*func)(SAudioRequestInfo const* const), void* const pRequestOwner) = 0;
	virtual void										RemoveRequestListener(void (*func)(SAudioRequestInfo const* const), void* const pRequestOwner) = 0;
	virtual void										ExternalUpdate() = 0;
	virtual bool										GetAudioTriggerID(char const* const sAudioTriggerName, TAudioControlID& rAudioTriggerID) const = 0;
	virtual bool										GetAudioRtpcID(char const* const sAudioRtpcName, TAudioControlID& rAudioRtpcID) const = 0;
	virtual bool										GetAudioSwitchID(char const* const sAudioSwitchName, TAudioControlID& rAudioSwitchID) const = 0;
	virtual bool										GetAudioSwitchStateID(TAudioControlID const nSwitchID, char const* const sAudioTriggerName, TAudioSwitchStateID& rAudioStateID) const = 0;
	virtual bool										GetAudioPreloadRequestID(char const* const sAudioPreloadRequestName, TAudioPreloadRequestID& rAudioPreloadRequestID) const = 0;
	virtual bool										GetAudioEnvironmentID(char const* const sAudioEnvironmentName, TAudioEnvironmentID& rAudioEnvironmentID) const = 0;
	virtual bool										ReserveAudioListenerID(TAudioObjectID& rAudioObjectID) = 0;
	virtual bool										ReleaseAudioListenerID(TAudioObjectID const nAudioObjectID) = 0;
	virtual void										OnCVarChanged(ICVar* const pCvar) = 0;
	virtual void										GetInfo(SAudioSystemInfo& rAudioSystemInfo) = 0;
	virtual char const*							GetConfigPath() const = 0;
	virtual IAudioProxy*						GetFreeAudioProxy() = 0;
	virtual void										FreeAudioProxy(IAudioProxy* const pIAudioProxy) = 0;
	virtual char const*							GetAudioControlName(EAudioControlType const eAudioEntityType, TATLIDType const nAudioEntityID) = 0;
	virtual char const*							GetAudioControlName(EAudioControlType const eAudioEntityType, TATLIDType const nAudioEntityID1, TATLIDType const nAudioEntityID2) = 0;

};

//////////////////////////////////////////////////////////////////////////
struct IAudioListener 
{

	virtual ~IAudioListener() {}

	virtual void						Init() = 0;
	virtual EntityId				GetID() const = 0;
	virtual bool						GetActive() const = 0;
	virtual void						SetActive(bool const bActive) = 0;
	virtual bool						GetMoved() const = 0;
	virtual bool						GetInside() const = 0;
	virtual void						SetInside(bool const bInside) = 0;
	virtual void						SetRecordLevel(float const fRecordLevel) = 0;
	virtual float						GetRecordLevel() const = 0;
	virtual Vec3						GetPosition() const = 0;
	virtual void						SetPosition(Vec3 const& rPosition) = 0;
	virtual Vec3 const&			GetForward() const = 0;
	virtual Vec3						GetTop() const = 0;
	virtual Vec3 const&			GetVelocity() const = 0;
	virtual void						SetVelocity(Vec3 const& vVel) = 0;
	virtual void						SetMatrix(Matrix34 const& newTransformation) = 0;
	virtual Matrix34 const&	GetMatrix() const = 0;
	virtual float						GetUnderwater() const = 0;
	virtual void						SetUnderwater(const float fUnder) = 0;
	virtual IVisArea*				GetVisArea() const = 0;
	virtual void						SetVisArea(IVisArea* const pVArea) = 0;

};

#endif // IAUDIOSYSTEM_H_INCLUDED
