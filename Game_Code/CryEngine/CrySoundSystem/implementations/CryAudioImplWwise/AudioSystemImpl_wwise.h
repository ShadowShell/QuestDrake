// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef AUDIO_SYSTEM_IMPL_WWISE_H_INCLUDED
#define AUDIO_SYSTEM_IMPL_WWISE_H_INCLUDED

#include "FileIOHandler_wwise.h"
#include "ATLEntities_wwise.h"
#include <IAudioSystemImplementation.h>

class CAudioSystemImpl_wwise : public IAudioSystemImplementation
{
public:
	
	CAudioSystemImpl_wwise();
	virtual ~CAudioSystemImpl_wwise();

	// IAudioSystemImplementation
	VIRTUAL void																	Update(float const fUpdateIntervalMS);

	VIRTUAL EAudioRequestStatus										Init();
	VIRTUAL EAudioRequestStatus										ShutDown();
	VIRTUAL EAudioRequestStatus										Release();
	VIRTUAL EAudioRequestStatus										OnLoseFocus();
	VIRTUAL EAudioRequestStatus										OnGetFocus();
	VIRTUAL EAudioRequestStatus										MuteAll();
	VIRTUAL EAudioRequestStatus										UnmuteAll();
	VIRTUAL EAudioRequestStatus										StopAllSounds();

	VIRTUAL EAudioRequestStatus										RegisterAudioObject(
																									IATLAudioObjectData* const pObjectData, 
																									char const* const sObjectName);
	VIRTUAL EAudioRequestStatus										RegisterAudioObject(IATLAudioObjectData* const pObjectData);
	VIRTUAL EAudioRequestStatus										UnregisterAudioObject(IATLAudioObjectData* const pObjectData);
	VIRTUAL EAudioRequestStatus										ResetAudioObject(IATLAudioObjectData* const pObjectData);
	VIRTUAL EAudioRequestStatus										UpdateAudioObject(IATLAudioObjectData* const pObjectData);

	VIRTUAL EAudioRequestStatus										PrepareTriggerSync(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLTriggerImplData const* const pTriggerDataData);
	VIRTUAL EAudioRequestStatus										UnprepareTriggerSync(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLTriggerImplData const* const pTriggerData);
	VIRTUAL EAudioRequestStatus										PrepareTriggerAsync(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLTriggerImplData const* const pTriggerData,
																									IATLEventData* const pEventData);
	VIRTUAL EAudioRequestStatus										UnprepareTriggerAsync(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLTriggerImplData const* const pTriggerData,
																									IATLEventData* const pEventData);
	VIRTUAL EAudioRequestStatus										ActivateTrigger(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLTriggerImplData const* const pTriggerData,
																									IATLEventData* const pEventData);
	VIRTUAL EAudioRequestStatus										StopEvent(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLEventData const* const pEventData);
	VIRTUAL EAudioRequestStatus										StopAllEvents(
																									IATLAudioObjectData* const pAudioObjectData);
	VIRTUAL EAudioRequestStatus										SetPosition(
																									IATLAudioObjectData* const pAudioObjectData,
																									SATLWorldPosition const& sWorldPosition);
	VIRTUAL EAudioRequestStatus										SetEnvironment(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLEnvironmentImplData const* const pEnvironmentImplData,
																									float const fAmount);
	VIRTUAL EAudioRequestStatus										SetRtpc(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLRtpcImplData const* const pRtpcData,
																									float const fValue);
	VIRTUAL EAudioRequestStatus										SetSwitchState(
																									IATLAudioObjectData* const pAudioObjectData,
																									IATLSwitchStateImplData const* const pSwitchStateData);
	VIRTUAL EAudioRequestStatus										SetObstructionOcclusion(
																									IATLAudioObjectData* const pAudioObjectData,
																									float const fObstruction,
																									float const fOcclusion);
	VIRTUAL EAudioRequestStatus										SetListenerPosition(
																									IATLListenerData* const pListenerData,
																									SATLWorldPosition const& oNewPosition);

	VIRTUAL EAudioRequestStatus										RegisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry);
	VIRTUAL EAudioRequestStatus										UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry);

	VIRTUAL EAudioRequestStatus										ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SATLAudioFileEntryInfo* const pFileEntryInfo);
	VIRTUAL void																	DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntryData);
	VIRTUAL char const* const											GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo);

	VIRTUAL SATLTriggerImplData_wwise const*			NewAudioTriggerImplData(XmlNodeRef const pAudioTriggerNode);
	VIRTUAL void																	DeleteAudioTriggerImplData(IATLTriggerImplData const* const pOldTriggerImplData);

	VIRTUAL SATLRtpcImplData_wwise const*					NewAudioRtpcImplData(XmlNodeRef const pAudioRtpcNode);
	VIRTUAL void																	DeleteAudioRtpcImplData(IATLRtpcImplData const* const pOldRtpcImplData);

	VIRTUAL SATLSwitchStateImplData_wwise const*	NewAudioSwitchStateImplData(XmlNodeRef const pAudioSwitchStateImplNode);
	VIRTUAL void																	DeleteAudioSwitchStateImplData(IATLSwitchStateImplData const* const pOldAudioSwitchStateImplNode);

	VIRTUAL SATLEnvironmentImplData_wwise const*	NewAudioEnvironmentImplData(XmlNodeRef const pAudioEnvironmentNode);
	VIRTUAL void																	DeleteAudioEnvironmentImplData(IATLEnvironmentImplData const* const pOldEnvironmentImplData);

	VIRTUAL SATLAudioObjectData_wwise*						NewGlobalAudioObjectData(TAudioObjectID const nObjectID);
	VIRTUAL SATLAudioObjectData_wwise*						NewAudioObjectData(TAudioObjectID const nObjectID);
	VIRTUAL void																	DeleteAudioObjectData(IATLAudioObjectData* const pOldObjectData);

	VIRTUAL SATLListenerData_wwise*								NewDefaultAudioListenerObjectData();
	VIRTUAL SATLListenerData_wwise*								NewAudioListenerObjectData(uint const nIndex);
	VIRTUAL void																	DeleteAudioListenerObjectData(IATLListenerData* const pOldListenerData);

	VIRTUAL SATLEventData_wwise*									NewAudioEventData(TAudioEventID const nEventID);
	VIRTUAL void																	DeleteAudioEventData(IATLEventData* const pOldEventData);
	VIRTUAL void																	ResetAudioEventData(IATLEventData* const pEventData);

	VIRTUAL void																	OnAudioSystemRefresh();
	VIRTUAL void																	SetLanguage(char const* const sLanguage);

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	VIRTUAL char const* const											GetImplementationNameString() const;
	VIRTUAL void																	GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const;
	// ~IAudioSystemImplementation

private:

	static char const* const	sWwiseEventTag;
	static char const* const	sWwiseRtpcTag;
	static char const* const	sWwiseSwitchTag;
	static char const* const	sWwiseStateTag;
	static char const* const	sWwiseRtpcSwitchTag;
	static char const* const	sWwiseFileTag;
	static char const* const	sWwiseAuxBusTag;
	static char const* const	sWwiseValueTag;

	static char const* const	sWwiseNameAttribute;
	static char const* const	sWwiseValueAttribute;
	static char const* const	sWwiseMutiplierAttribute;
	static char const* const	sWwiseShiftAttribute;
	static char const* const	sWwiseLocalisedAttribute;

	struct SEnvPairCompare
	{
		bool operator() (std::pair<AkAuxBusID, float> const& oPair1, std::pair<AkAuxBusID, float> const& oPair2) const;
	};

	SATLSwitchStateImplData_wwise const*	ParseWwiseSwitchOrState(XmlNodeRef pNode,	EWwiseSwitchType eType);
	SATLSwitchStateImplData_wwise const*	ParseWwiseRtpcSwitch(XmlNodeRef pNode);
	void																	ParseRtpcImpl(XmlNodeRef pNode, AkRtpcID& rAkRtpcID, float& rMult, float& rShift);
	EAudioRequestStatus										PrepUnprepTriggerSync(
																					IATLTriggerImplData const* const pTriggerData,
																					bool bPrepare);
	EAudioRequestStatus										PrepUnprepTriggerAsync(
																					IATLTriggerImplData const* const pTriggerData,
																					IATLEventData* const pEventData,
																					bool bPrepare);

	EAudioRequestStatus										PostEnvironmentAmounts(IATLAudioObjectData* const pAudioObjectData);

	AkGameObjectID												m_nDummyGameObjectID;
	AkBankID															m_nInitBankID;
	CFileIOHandler_wwise									m_oFileIOHandler;

	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH>	m_sRegularSoundBankFolder;
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH>	m_sLocalizedSoundBankFolder;

#if !defined(WWISE_FOR_RELEASE)
	bool m_bCommSystemInitialized;
#endif // !WWISE_FOR_RELEASE

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> m_sFullImplString;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};

#endif // AUDIO_SYSTEM_IMPL_WWISE_H_INCLUDED
