/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Manager used on dedicated servers to log and action any
suspicious activity by clients in an attempt to clamp down on cheaters.

-------------------------------------------------------------------------
History:
- 01:02:2011  : Created by Martin Sherburn

*************************************************************************/

#ifndef __ANTICHEATMANAGER_H__
#define __ANTICHEATMANAGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "DownloadMgr.h"
#include "IAntiCheatManager.h"

#if defined(DEDICATED_SERVER) || defined(SERVER_CHECKS)
#define ANTI_CHEAT_ENABLED
#endif

#if defined(ANTI_CHEAT_ENABLED)

#if defined(DEDICATED_SERVER)
//#define USE_CRISP_THINKING
#endif

#include "Network/Lobby/SessionNames.h"
#include "Network/Lobby/IGameLobbyEventListener.h"
#include <IXml.h>
#include <ICryMatchMaking.h>

#define ENUM_PREFIX_LENGTH 4
#define MAX_FLAG_ACTIVITY_PARAMS 2

#define CheatTypeList(f) \
	f(eCT_WeaponDamage) \
	f(eCT_HitLocationMismatch) \
	f(eCT_FireRate) \
	f(eCT_HitShotOwnerMismatch) \
	f(eCT_HitShotIdMismatch) \
	f(eCT_MissingShotId) \
	f(eCT_MultipleEntityHits) \
	f(eCT_MeleeRange) \
	f(eCT_MeleeLungeRange) \
	f(eCT_PlayerSpeed) \
	f(eCT_ServerSpawnedAmmoUsed) \
	f(eCT_ShootingWithoutFiremode) \
  f(eCT_FileDataProbe) \
	f(eCT_CvarDataNameProbe) \
  f(eCT_CvarDataValueProbe) \
  f(eCT_DataProbeTimeout) \
  f(eCT_ValidHitInfo) \
  f(eCT_Headshots_ContiguousHits) \
  f(eCT_Headshots_RatioTooHigh) \
	f(eCT_UserFlying) \
  f(eCT_IncorrectXPAwarded) \
	f(eCT_AimBot) \
	f(eCT_Tide) \
	f(eCT_NoKillCam) \
	f(eCT_TamperedKillCam) \
	f(eCT_BlindFireKill) \
	f(eCT_VoteKicked) \
	f(eCT_FiringFromCloak) \
	f(eCT_InconsistentProfile_XP) \
	f(eCT_InconsistentProfile_SuitXP) \
	f(eCT_InconsistentProfile_Kills) \
	f(eCT_InconsistentProfile_Deaths) \
	f(eCT_InconsistentProfile_Assists) \
	f(eCT_InconsistentProfile_UnChangedGameStats) \
	f(eCT_InconsistentProfile_Wins) \
	f(eCT_InconsistentProfile_Draws) \
	f(eCT_InconsistentProfile_Losses) \
	f(eCT_OnlineAttributes_CorruptRead) \
	f(eCT_OnlineAttributes_CorruptXP) \
	f(eCT_OnlineAttributes_CorruptChecksum) \
	f(eCT_GlobalInfractions) \
	f(eCT_External) \
	
AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(ECheatType, CheatTypeList, eCT_Invalid, eCT_Num);

#define CheatOperatorList(f) \
	f(eCO_Equal) \
	f(eCO_NotEqual) \
	f(eCO_GreaterThan) \
	f(eCO_LessThan) \
	f(eCO_GreaterThanEqual) \
	f(eCO_LessThanEqual) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(ECheatOperator, CheatOperatorList, eCO_Invalid, eCO_Num);

#define CheatActionList(f) \
	f(eCA_Log) \
	f(eCA_Log_Remote) \
	f(eCA_Infraction) \
	f(eCA_Kick) \
	f(eCA_Kick_Delayed) \
	f(eCA_Ban) \
	f(eCA_Global_Ban) \
	f(eCA_Dev_CheatDetected) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(ECheatAction, CheatActionList, eCA_Invalid, eCA_Num);

#define CheatAssetGroupList(f) \
  f(eAG_pak) \
  f(eAG_dds) \
  f(eAG_cgf) \
  f(eAG_cga) \
  f(eAG_caf) \
  f(eAG_xml) \
  f(eAG_lua) \
  f(eAG_other) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(ECheatAssetGroup, CheatAssetGroupList, eAG_Invalid, eAG_Num);

#define AntiCheatVarsList_Float(f) \
	f(eAV_AB_AimTime) \
	f(eAV_AB_AimDot) \
	f(eAV_AB_MinAcc) \
	f(eAV_M_ConsistencyMax) \
	f(eAV_KD_Min) \
	f(eAV_KD_Max) \
	f(eAV_HS_MinHitsRequired) \
	f(eAV_HS_MaxContiguous) \
	f(eAV_HS_MaxRatio) \
	f(eAV_IP_XPScale) \
	f(eAV_KK_Dot) \
	f(eAV_KK_Dist) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EAntiCheatVar_Float, AntiCheatVarsList_Float, eAV_Invalid_Float, eAV_Num_Float);

#define AntiCheatVarsList_Int(f) \
	f(eAV_IP_UseTest_XP) \
	f(eAV_IP_UseTest_SuitXP) \
	f(eAV_IP_UseTest_Kills) \
	f(eAV_IP_UseTest_Deaths) \
	f(eAV_IP_UseTest_Assists) \
	f(eAV_IP_UseTest_Wins) \
	f(eAV_IP_UseTest_Losses) \
	f(eAV_IP_UseTest_Draws) \
	f(eAV_IP_UseTest_UnChangedGameStats) \
	f(eAV_IP_UseTest_ValidatePredicatedSpawn) \
	f(eAV_IP_UseTest_ValidateXP) \
	f(eAV_IP_UseTest_ResetCorruptProfile) \
	f(eAV_IP_VoteKickConfidence) \
	f(eAV_3P_StatsUpload) \

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EAntiCheatVar_Int, AntiCheatVarsList_Int, eAV_Invalid_Int, eAV_Num_Int);
	
static const int kDefaultConfidence = 100;

struct SCrysis3SettingsLoadResult;

struct SCheatCondition
{
	SCheatCondition() : op(eCO_Invalid), paramNum(1), value(0.f) {}
	ECheatOperator op;
	float value;
	int paramNum;
};

struct SCheatAction
{
	SCheatAction() : action(eCA_Invalid), severity(0.f), banTimeout(0.f), confidence(kDefaultConfidence) {}
	ECheatAction action;
	float severity;
	float banTimeout;
	int		confidence;
	std::vector<SCheatCondition> conditions;
};

struct SCheatType
{
	std::vector<SCheatAction> actions;
};

struct SCheatInfraction
{
	SCheatInfraction() : cumulativeSeverity(0), numInfractions(0) {}
	float cumulativeSeverity;
	int numInfractions;
};

struct SCheatRecord
{
	SCheatInfraction infractions[eCT_Num];
};

struct SGlobalCheatAction
{
	SGlobalCheatAction() : action(eCA_Invalid), threshhold(0.f), banTimeout(0.f) {}
	ECheatAction action;
	float threshhold;
	float banTimeout;
};

struct SQueuedFlagActivity
{
	SQueuedFlagActivity() : type(0), channelId(0), numParams(0) {}
	TCheatType type;
	uint16 channelId;
	float params[MAX_FLAG_ACTIVITY_PARAMS];
	int numParams;
	XmlNodeRef xmlNode;
	CryStringLocal message;
};

class IGameRulesPlayerStatsModule;

class CAntiCheatManager : IDataListener, public IAntiCheatManager, public IGameLobbyEventListener
{
public:
	CAntiCheatManager();
	~CAntiCheatManager();

  // hashing methods support 0 to <5 methods 
  int RetrieveHashMethod(){ return m_hashMethod; }
  int GetAssetGroupCount(){ return eAG_Num; }
  void FlagActivity(TCheatType type, uint16 channelId, const char * message);
	void FlagActivity(TCheatType type, uint16 channelId);
	void FlagActivity(TCheatType type, uint16 channelId, float param1);
	void FlagActivity(TCheatType type, uint16 channelId, float param1, float param2);	// Please update MAX_FLAG_ACTIVITY_PARAMS if you add more parameters
	void FlagActivity(TCheatType type, uint16 channelId, float param1, const char * message);
	void FlagActivity(TCheatType type, uint16 channelId, float param1, float param2, const char * message);

	virtual bool HasDownloadedEssentials(void) { return true; }

	void ChatMessage(uint16 channelId, const char * pString);

	void PlayerVotedOff( EntityId playerId, const TVoteDataList& voteData, float fSecondsToBanFor );

  TCheatType FindCheatType(const char* name);
	TAntiCheatVarIdx FindAntiCheatVarIdx_Float(const char * name);
	TAntiCheatVarIdx FindAntiCheatVarIdx_Int(const char * name);
  TCheatAssetGroup FindAssetTypeByExtension(const char * ext);
  TCheatAssetGroup FindAssetTypeByWeight();
	
	virtual void InitSession();
	virtual void OnSessionEnd();
	
	void SetEnableLogUploads(bool enabled) { m_enableLogUploads = enabled; }
	void Update(float dt);

	bool IsCheatProtectionEnabled(TCheatType cheatType);
	XmlNodeRef GetFileProbeConfiguration() { return m_fileProtectConfig; }

	ILINE float	GetAntiCheatVar	(EAntiCheatVar_Float acvEnum, float fDefault) { return m_cheatVarsFloat[(int)acvEnum] != FLT_MAX ? m_cheatVarsFloat[(int)acvEnum] : fDefault; }
	ILINE int	GetAntiCheatVar	(EAntiCheatVar_Int acvEnum, int iDefault) { return m_cheatVarsInt[(int)acvEnum] != INT_MAX ? m_cheatVarsInt[(int)acvEnum] : iDefault; }

	// IDataListener interface implementation
	virtual void DataDownloaded(CDownloadableResourcePtr inResource);
	virtual void DataFailedToDownload(CDownloadableResourcePtr inResource);

	void BanPlayer(uint16 channelId, const char* reason, float timeout);

	void OnClientConnect(INetChannel& rNetChannel);
	void OnClientDisconnect(INetChannel& rNetChannel, IActor * pActor);

	// IGameLobbyEventListener
	virtual void InsertedUser(CryUserID userId, const char *userName);
	virtual void SessionChanged(const CrySessionHandle inOldSession, const CrySessionHandle inNewSession) {}
	// ~IGameLobbyEventListener
	
private:
	void QueueFlagActivity(TCheatType type, uint16 channelId, const float *params, int numParams, const char * message);
	void ProcessFlagActivity(TCheatType type, uint16 channelId, const float *params, int numParams, const char * pDescription, XmlNodeRef additionalXmlData = NULL);
	void ParseAntiCheatConfig(const char * filename);
	void ParseAntiCheatConfig(XmlNodeRef xmlData);
	ECheatOperator FindCheatOperator(const char* name);
	ECheatAction FindCheatAction(const char* name);
	void RegisterInfraction(XmlNodeRef incidentXml, uint16 channelId, TCheatType type, float severity);
	void VerifyInfractions(XmlNodeRef incidentXml, uint16 channelId, const SCheatRecord &cheatRecord, float severity);
	void KickPlayer(uint16 channelId, EDisconnectionCause reason, int nConfidence = kDefaultConfidence);
	void KickPlayer(const CryUserID& userId, EDisconnectionCause reason, int nConfidence);
	void KickPlayerDelayed(uint16 channelId, EDisconnectionCause reason, int nConfidence = kDefaultConfidence);
	void BanPlayer_Internal(uint16 channelId, float timeout, int nConfidence = kDefaultConfidence);

	EDisconnectionCause GetBanKickType(uint16 channelId);

	struct SPlayerSessionData
	{
		SPlayerSessionData() : kills(0), deaths(0), points(0) {}
		string playerName;
		CTimeValue connectTime;
		CTimeValue disconnectTime;
		uint16 kills;
		uint16 deaths;
		uint32 points;
	};

	void GetPlayerStats(IGameRulesPlayerStatsModule * pPlayerStatsModule, EntityId playerId, SPlayerSessionData& sessionData);
	static void GenerateMessageString(stack_string& outString, const float *params, int numParams, const char * message);

	typedef std::pair<uint16, SPlayerSessionData> TPlayerSessionPair;
	typedef std::map<uint16, SPlayerSessionData> TPlayerSessionDataMap;
	TPlayerSessionDataMap m_PlayerSessionData;

	static void Xml_SetDisplayName(XmlNodeRef xmlNode, uint16 channelId);
	static void Xml_SetGuid(XmlNodeRef xmlNode, uint16 channelId);
	static void Xml_SetGuid(XmlNodeRef xmlNode, const char * pGuid);
	static void Xml_SetCheatType(XmlNodeRef xmlNode, TCheatType eCheatType);
	static void Xml_SetActionType(XmlNodeRef xmlNode, ECheatAction eCheatAction);
	static void Xml_TimestampNode(XmlNodeRef xmlNode);

	XmlNodeRef CreateIncidentXML(uint16 channelId, TCheatType type, const float *params, int numParams, const char * pDescription);

#if defined(DEV_CHEAT_HANDLING)
	void HandleDevCheat(uint16 channelId, const char * message);
#endif

	XmlNodeRef CheatLogAction(XmlNodeRef incidentXml, ECheatAction eCheatAction, int nConfidence);
	void CheatLogSeverity(XmlNodeRef incidentXml, uint16 channelId, float fTotalSeverity, int numInfractions, float fDeltaSeverity);
	void DumpCheatRecords();
	void DumpPlayerRecords();
	void DecayCheatRecords();
	void CheatLogInternalXml(XmlNodeRef xmlNode);
	void OpenLogFile();
	void CloseLogFile();

	void HandleDelayedKicks(float dt);

	int GetLogFileSize();
	void BackupLogFileAndSubmit();
	CDownloadableResourcePtr GetDownloadableResource();
	void LoadAntiCheatVars(XmlNodeRef child);
	void ResetAntiCheatVars();
	template <class T>
	bool MeetsCondition(ECheatOperator op, T left, T right);

	SCheatType m_cheats[eCT_Num];
	float m_cheatVarsFloat[eAV_Num_Float];
	int m_cheatVarsInt[eAV_Num_Int];
	std::map<CryUserID, SCheatRecord> m_cheatRecords;
	std::vector<SGlobalCheatAction> m_globalActions;
	std::queue<SQueuedFlagActivity> m_queuedFlagActivity;
	CryCriticalSection m_mutex;
	FILE *m_pLogFile;
	float m_decayRate;
	int m_hashMethod;
	string m_logFileName;
	CTimeValue m_lastDownloadTime;
  int m_totalAssetWeighting;
  std::vector<int> m_assetGroupWeights;

	struct SDelayedKickData
	{
		CryUserID userId;
		float			fKickCountdown;
		uint16		channelId;
		EDisconnectionCause reason;
	};

	std::vector<TVoteDataList>		m_VoteDataCache;
	std::vector<SDelayedKickData> m_DelayedKickData;
	bool m_enableLogUploads;
	XmlNodeRef	m_fileProtectConfig;

	uint32			m_uChatMsgsSent;
	uint32			m_uIncidents;

	//TAntiCheatLoggers m_AntiCheatLoggers;

#if defined(USE_CRISP_THINKING)
	void HandleCrispThinkingOutput(TCheatType type, uint16 channelId, XmlNodeRef incidentXML, int nMaxConfidence);
	void OutputXMLToCrispThinking(XmlNodeRef xml);

	void * m_CrispLibrary;
	class ICrispWrapper* m_pCrispWrapper;
#endif
};
#endif

#endif // __ANTICHEATMANAGER_H__
