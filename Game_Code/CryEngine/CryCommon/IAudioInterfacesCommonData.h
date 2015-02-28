// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef INTERFACES_COMMON_DATA_H_INCLUDED
#define INTERFACES_COMMON_DATA_H_INCLUDED

#include "Cry_Math.h"
#include "BaseTypes.h"
#include "smartptr.h"

// To be removed once we provide full C++11 support!
#if !defined(NPTR)
#if __cplusplus > 199711L
#define NPTR nullptr
#else
#define NPTR NULL
#endif
#endif // !NPTR

// To be removed once we provide full C++11 support!
#if !defined(ATL_ENUM_TYPE)
#if __cplusplus > 199711L
#define ATL_ENUM_TYPE : TATLEnumFlagsType
#else
#define ATL_ENUM_TYPE
#endif
#endif // !NPTR

typedef uint32 TATLIDType;

typedef TATLIDType TAudioObjectID;
#define INVALID_AUDIO_OBJECT_ID ((TAudioObjectID)(0))
typedef TATLIDType TAudioControlID;
#define INVALID_AUDIO_CONTROL_ID ((TAudioControlID)(0))
typedef TATLIDType TAudioSwitchStateID;
#define INVALID_AUDIO_SWITCH_STATE_ID ((TAudioSwitchStateID)(0))
typedef TATLIDType TAudioEnvironmentID;
#define INVALID_AUDIO_ENVIRONMENT_ID ((TAudioEnvironmentID)(0))
typedef TATLIDType TAudioPreloadRequestID;
#define INVALID_AUDIO_PRELOAD_REQUEST_ID ((TAudioPreloadRequestID)(0))
typedef TATLIDType TAudioEventID;
#define INVALID_AUDIO_EVENT_ID ((TAudioEventID)(0))
typedef TATLIDType TAudioFileEntryID;
#define INVALID_AUDIO_FILE_ENTRY_ID ((TAudioFileEntryID)(0))
typedef TATLIDType TAudioTriggerImplID;
#define INVALID_AUDIO_TRIGGER_IMPL_ID ((TAudioTriggerImplID)(0))
typedef TATLIDType TAudioTriggerInstanceID;
#define INVALID_AUDIO_TRIGGER_INSTANCE_ID ((TAudioTriggerInstanceID)(0))
typedef TATLIDType TATLEnumFlagsType;
#define INVALID_AUDIO_ENUM_FLAG_TYPE ((TATLEnumFlagsType)(0))
typedef TATLIDType TAudioProxyID;
#define INVALID_AUDIO_PROXY_ID ((TAudioProxyID)(0))
#define DEFAULT_AUDIO_PROXY_ID ((TAudioProxyID)(1))

struct SATLWorldPosition
{
	SATLWorldPosition()
		: mPosition(IDENTITY)
		, vVelocity(ZERO)
	{}

	SATLWorldPosition(Matrix34 const& mPos, Vec3 const& vVel)
		: mPosition(mPos)
		, vVelocity(vVel)
	{}

	ILINE Vec3	GetPositionVec() const {return mPosition.GetColumn3();}
	ILINE Vec3	GetForwardVec() const {return mPosition.GetColumn1();}

	Matrix34		mPosition;
	Vec3				vVelocity;
};

#define AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED 100// IDs below that value are used for the CATLTriggerImpl_Internal

#define	MAX_AUDIO_FILE_PATH_LENGTH 256
#define	MAX_AUDIO_FILE_NAME_LENGTH 128
#define	MAX_AUDIO_OBJECT_NAME_LENGTH 256

enum EAudioRequestFlags ATL_ENUM_TYPE
{
	eARF_NONE							= 0,
	eARF_PRIORITY_NORMAL	= BIT(0),
	eARF_PRIORITY_HIGH		= BIT(1),
	eARF_EXECUTE_BLOCKING	= BIT(2),
	eARF_SYNC_CALLBACK		= BIT(3),
	eARF_STAY_IN_MEMORY		= BIT(4),
};

enum EAudioRequestType ATL_ENUM_TYPE
{
	eART_NONE														= 0,
	eART_AUDIO_MANAGER_REQUEST					= 1,
	eART_AUDIO_CALLBACK_MANAGER_REQUEST	= 2,
	eART_AUDIO_OBJECT_REQUEST						= 3,
	eART_AUDIO_LISTENER_REQUEST					= 4,
};

enum EAudioRequestResult ATL_ENUM_TYPE
{
	eARR_NONE			= 0,
	eARR_SUCCESS	= 1,
	eARR_FAILURE	= 2,
};

struct SAudioRequestDataBase
{
	explicit SAudioRequestDataBase(EAudioRequestType const eType = eART_NONE)
		: eRequestType(eType)
	{}

	virtual ~SAudioRequestDataBase() {}

	EAudioRequestType const	eRequestType;
};

struct SAudioRequest
{
	SAudioRequest()
		: nFlags(eARF_NONE)
		, nAudioObjectID(INVALID_AUDIO_OBJECT_ID)
		, pOwner(NPTR)
		, sValue(NPTR)
		, pData(NPTR)
	{}

	~SAudioRequest() {}

	uint32									nFlags;
	TAudioObjectID					nAudioObjectID;
	void*										pOwner;
	char const*							sValue;
	SAudioRequestDataBase*	pData;

private:

	SAudioRequest(SAudioRequest const& other);
	SAudioRequest& operator=(SAudioRequest const& other);
};

struct SAudioRequestInfo
{
	explicit SAudioRequestInfo(EAudioRequestResult const ePassedResult, void* const	pPassedOwner, EAudioRequestType const ePassedAudioRequestType, TATLEnumFlagsType const nPassedSpecificAudioRequest)
		: eResult(ePassedResult)
		,	pOwner(pPassedOwner)
		, eAudioRequestType(ePassedAudioRequestType)
		, nSpecificAudioRequest(nPassedSpecificAudioRequest)
	{}

	EAudioRequestResult const	eResult;
	void* const								pOwner;
	EAudioRequestType const		eAudioRequestType;
	TATLEnumFlagsType	const		nSpecificAudioRequest;
};

#endif // INTERFACES_COMMON_DATA_H_INCLUDED
