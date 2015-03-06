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

#include "StdAfx.h"
#include "AntiCheatManager.h"
#include "Network/Lobby/GameLobby.h"
#include "GameCVars.h"
#include "Network/Lobby/GameBrowser.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesPlayerStatsModule.h"

#include "CryLibrary.h"

#define PARAM_NODE "param"
#define PARAM_VALUE "value"
#define PARAM_NUMBER "number"

#if defined(ANTI_CHEAT_ENABLED)

// SECRET
#define DECRYPTION_KEY														"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define SIGNING_SALT															"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
// SECRET

#if defined(USE_CRISP_THINKING)
#include <CrispWrapper.h>
#endif

static AUTOENUM_BUILDNAMEARRAY(s_cheatTypeNames, CheatTypeList);
static AUTOENUM_BUILDNAMEARRAY(s_cheatOperatorNames, CheatOperatorList);
static AUTOENUM_BUILDNAMEARRAY(s_cheatActionNames, CheatActionList);
static AUTOENUM_BUILDNAMEARRAY(s_cheatAssetGroupNames, CheatAssetGroupList);
static AUTOENUM_BUILDNAMEARRAY(s_antiCheatVarNamesFloat, AntiCheatVarsList_Float);
static AUTOENUM_BUILDNAMEARRAY(s_antiCheatVarNamesInt, AntiCheatVarsList_Int);

CAntiCheatManager::CAntiCheatManager()
: m_pLogFile(NULL)
, m_decayRate(100.f)
, m_hashMethod(0)
, m_totalAssetWeighting(0)
, m_enableLogUploads(true)
, m_uChatMsgsSent(0)
, m_uIncidents(0)
#if defined(USE_CRISP_THINKING)
, m_CrispLibrary(NULL)
, m_pCrispWrapper(NULL)
#endif
{
	ResetAntiCheatVars();

	int instance = gEnv->pSystem->GetApplicationInstance();
	if(instance != 0)
	{
		m_logFileName.Format("./%sAntiCheatLog(%d).xml", gEnv->pSystem->GetRootFolder(), instance);
	}
	else
	{
		m_logFileName.Format("./%sAntiCheatLog.xml", gEnv->pSystem->GetRootFolder());
	}

	ParseAntiCheatConfig("Scripts/DedicatedConfigs/AntiCheatConfig.xml");

#if defined(_RELEASE)
	CDownloadableResourcePtr res = GetDownloadableResource();
	if (res)
	{
		res->AddDataListener(this);
	}
#endif

#if defined(USE_CRISP_THINKING)

#if defined(LINUX)
    m_CrispLibrary = CryLoadLibrary("CryCrispWrapper.so");
#elif defined(APPLE)
    m_CrispLibrary = CryLoadLibrary("CryCrispWrapper.dylib");
#else
	m_CrispLibrary = CryLoadLibrary("CryCrispWrapper.dll");
#endif
    if (m_CrispLibrary)
	{
		TCryCrispGet pGetCrisp = (TCryCrispGet)CryGetProcAddress((HMODULE)m_CrispLibrary, "CryCrispGet");
        if (pGetCrisp)
		{
			m_pCrispWrapper = (*pGetCrisp)();
			m_pCrispWrapper->Initialize();
		}
		else
		{
#if !defined(_RELEASE)
			CryFatalError("CryCrispWrapper.dll needs to be present in the Dedicated Server exe directory");
#endif
		}
	}
#endif

	m_lastDownloadTime = gEnv->pTimer->GetAsyncTime();

	//Examples of usage

	/*FlagActivity(eCT_WeaponDamage, 5641, 4, 300);
	FlagActivity(eCT_WeaponDamage, 5641, 2, 300);
	FlagActivity(eCT_WeaponDamage, 5641, 4, 100);
	FlagActivity(eCT_PlayerSpeed, 6465, 40);
	FlagActivity(eCT_PlayerSpeed, 6465, 100);
	FlagActivity(eCT_AmmoUsed, 8489);*/

}

CAntiCheatManager::~CAntiCheatManager()
{
	CloseLogFile();

	CDownloadableResourcePtr res = GetDownloadableResource();
	if (res)
	{
		res->RemoveDataListener(this);
	}


#if defined(USE_CRISP_THINKING)
	if(m_pCrispWrapper)
	{
		m_pCrispWrapper->Shutdown();
		m_pCrispWrapper = NULL;
		CryFreeLibrary(m_CrispLibrary);
	}
#endif
}

void CAntiCheatManager::ResetAntiCheatVars()
{
	for(int i = 0; i < eAV_Num_Float; i++)
	{
		m_cheatVarsFloat[i] = FLT_MAX;
	}
	for(int i = 0; i < eAV_Num_Int; i++)
	{
		m_cheatVarsInt[i] = INT_MAX;
	}
}

CDownloadableResourcePtr CAntiCheatManager::GetDownloadableResource()
{
	CDownloadableResourcePtr pResult(NULL);
	CDownloadMgr *pMgr=g_pGame->GetDownloadMgr();
	if (pMgr)
	{
		pResult=pMgr->FindResourceByName("anticheatconfig");
	}
	return pResult;
}

void CAntiCheatManager::DataDownloaded(CDownloadableResourcePtr inResource)
{
	const int bufferSize = 1024*1024;
	char* pBuffer = new char[bufferSize];
	int dataLength = bufferSize;

	inResource->GetDecryptedData(pBuffer,&dataLength,DECRYPTION_KEY,int(sizeof(DECRYPTION_KEY)-1),SIGNING_SALT,int(sizeof(SIGNING_SALT)-1));

	if (dataLength > 0)
	{
		XmlNodeRef xmlData = gEnv->pSystem->LoadXmlFromBuffer(pBuffer, dataLength);
		if (xmlData)
		{
			CryLog("Parsing downloaded Anti-Cheat Configuration...");
			ParseAntiCheatConfig(xmlData);
		}
		else
		{
			CryLog("Unable to parse downloaded Anti-Cheat Configuration");
		}
	}
	else
	{
		CryLog("Error decrypting Anti-Cheat Configuration");
	}
}

void CAntiCheatManager::DataFailedToDownload(CDownloadableResourcePtr inResource)
{
	CryLog("Failed to download Anti-Cheat Configuration");
}

void CAntiCheatManager::ParseAntiCheatConfig(const char * filename)
{
	CCryFile file;

	CryFixedStringT<128> realFileName;
	realFileName.Format("%s/%s", PathUtil::GetGameFolder().c_str(), filename);
	if (file.Open( realFileName.c_str(), "rb", ICryPak::FOPEN_HINT_QUIET | ICryPak::FOPEN_ONDISK ))
	{
		const size_t fileSize = file.GetLength();
		char* pBuffer = new char [fileSize];

		file.ReadRaw(pBuffer, fileSize);

		XmlNodeRef xmlData = gEnv->pSystem->LoadXmlFromBuffer(pBuffer, fileSize);

		SAFE_DELETE_ARRAY(pBuffer);

		if(xmlData)
		{
			CryLog("Parsing Anti-Cheat Configuration...");
			ParseAntiCheatConfig(xmlData);
		}
		else
		{
			CryLog("Unable to parse Anti-Cheat Configuration");
		}
	}
	else
	{
		CryLog("Unable to load '%s'", realFileName.c_str());
	}
}

TCheatType CAntiCheatManager::FindCheatType(const char* name)
{
	for (TCheatType i=0; i<eCT_Num; ++i)
	{
		if (strcmp(s_cheatTypeNames[i] + ENUM_PREFIX_LENGTH, name) == 0)
		{
			return i;
		}
	}
	return eCT_Invalid;
}

TAntiCheatVarIdx CAntiCheatManager::FindAntiCheatVarIdx_Float(const char * name)
{
	for (TAntiCheatVarIdx i = 0; i < eAV_Num_Float; ++i)
	{
		if (strcmp(s_antiCheatVarNamesFloat[i] + ENUM_PREFIX_LENGTH, name) == 0)
		{
			return i;
		}
	}
	
	return eAV_Invalid_Float;
}

TAntiCheatVarIdx CAntiCheatManager::FindAntiCheatVarIdx_Int(const char * name)
{
	for (TAntiCheatVarIdx i = 0; i < eAV_Num_Int; ++i)
	{
		if (strcmp(s_antiCheatVarNamesInt[i] + ENUM_PREFIX_LENGTH, name) == 0)
		{
			return i;
		}
	}
	
	return eAV_Invalid_Int;
}

TCheatAssetGroup CAntiCheatManager::FindAssetTypeByExtension(const char * ext)
{
  if ( !ext )
    return eAG_Invalid;

  for ( TCheatAssetGroup i = 0; i<eAG_Num; ++i )
  {
    if (strcmp(s_cheatAssetGroupNames[i] + ENUM_PREFIX_LENGTH, ext) == 0)
      return i;
  }

  return eAG_other;
}

TCheatAssetGroup CAntiCheatManager::FindAssetTypeByWeight()
{
  int weightedValue = cry_rand32()%m_totalAssetWeighting;
  int size = m_assetGroupWeights.size();
  for (TCheatAssetGroup idx = 0; idx < size; ++idx)
  {
    weightedValue -= m_assetGroupWeights[idx];
    if ( weightedValue <= 0 )
      return idx;
  }
  return eAG_Invalid;
}

ECheatOperator CAntiCheatManager::FindCheatOperator(const char* name)
{
	for (int i=0; i<eCO_Num; ++i)
	{
		if (strcmp(s_cheatOperatorNames[i] + ENUM_PREFIX_LENGTH, name) == 0)
		{
			return (ECheatOperator)i;
		}
	}
	return eCO_Invalid;
}

ECheatAction CAntiCheatManager::FindCheatAction(const char* name)
{
	for (int i=0; i<eCA_Num; ++i)
	{
		if (strcmp(s_cheatActionNames[i] + ENUM_PREFIX_LENGTH, name) == 0)
		{
			return (ECheatAction)i;
		}
	}
	return eCA_Invalid;
}

void CAntiCheatManager::ParseAntiCheatConfig(XmlNodeRef xmlData)
{
	ResetAntiCheatVars();

	// Reset the previous configuration so that we can call this function multiple times
	for (int i=0; i<eCT_Num; ++i)
	{
		m_cheats[i].actions.clear();
	}
	m_globalActions.clear();
	m_assetGroupWeights.clear();
	m_assetGroupWeights.resize(eAG_Num);
	m_totalAssetWeighting = 0;
	m_fileProtectConfig = NULL;

	int numChildren = xmlData->getChildCount();
	for (int i=0; i<numChildren; ++i)
	{
		XmlNodeRef child = xmlData->getChild(i);
		if (child->isTag("Cheat"))
		{
			int numActions = child->getChildCount();
			const char* sTypeName = child->getAttr("type");
			TCheatType cheatType = FindCheatType(sTypeName);
			if (cheatType != eCT_Invalid)
			{
				SCheatType &cheat = m_cheats[cheatType];
				cheat.actions.reserve(numActions);
				for (int j=0; j<numActions; ++j)
				{
					XmlNodeRef actionChild = child->getChild(j);
					if (actionChild->isTag("Action"))
					{
						bool addAction = true;
						int numConditions = actionChild->getChildCount();
						SCheatAction cheatAction;
						const char* sActionName = actionChild->getAttr("value");
						cheatAction.action = FindCheatAction(sActionName);
						if (cheatAction.action != eCA_Invalid)
						{
							int nConfidence = kDefaultConfidence;
							actionChild->getAttr("confidence", nConfidence);

							cheatAction.confidence = nConfidence;

							if (cheatAction.action == eCA_Infraction)
							{
								actionChild->getAttr("severity", cheatAction.severity);
								if (cheatAction.severity <= 0.f)
								{
									CryLog("Invalid severity %f", cheatAction.severity);
									cheatAction.severity = 0.f;
								}
							}
							else if (cheatAction.action == eCA_Ban)
							{
								actionChild->getAttr("timeout", cheatAction.banTimeout);
							}
							cheatAction.conditions.reserve(numConditions);
							for (int k=0; k<numConditions; ++k)
							{
								XmlNodeRef conditionChild = actionChild->getChild(k);
								if (conditionChild->isTag("Condition"))
								{
									SCheatCondition cheatCondition;
									const char* sOperatorName = conditionChild->getAttr("operator");
									cheatCondition.op = FindCheatOperator(sOperatorName);
									if (cheatCondition.op != eCO_Invalid)
									{
										conditionChild->getAttr("value", cheatCondition.value);
										conditionChild->getAttr("param", cheatCondition.paramNum);
										cheatAction.conditions.push_back(cheatCondition);
									}
									else
									{
										CryLog("Unrecognised operator %s", sOperatorName);
									}
								}
								else if (conditionChild->isTag("DediVersion"))
								{
#if defined(DEDICATED_SERVER)
									const char* sOperatorName = conditionChild->getAttr("operator");
									ECheatOperator op = FindCheatOperator(sOperatorName);
									if (op != eCO_Invalid)
									{
										int value;
										if (conditionChild->getAttr("value", value))
										{
											if (!MeetsCondition(op, DEDI_VERSION, value))
											{
												// Ignore this action if it doesn't meet the gamespy dedi version check
												addAction = false;
												break;
											}
										}
									}
									else
									{
										CryLog("Unrecognised operator %s", sOperatorName);
									}
#endif
								}
								else
								{
									CryLog("Unrecognised child node %s", conditionChild->getTag());
								}
							}
							if (addAction)
							{
								cheat.actions.push_back(cheatAction);
							}
						}
						else
						{
							CryLog("Unrecognised action %s", sActionName);
						}
					}
					else
					{
						CryLog("Unrecognised child node %s", actionChild->getTag());
					}
				}
			}
			else
			{
				CryLog("Unrecognised cheat type %s", sTypeName);
			}
		}
		else if (child->isTag("Global"))
		{
			child->getAttr("decay_rate", m_decayRate);
			int numActions = child->getChildCount();
			m_globalActions.reserve(numActions);
			for (int j=0; j<numActions; ++j)
			{
				XmlNodeRef actionChild = child->getChild(j);
				if (actionChild->isTag("Action"))
				{
					SGlobalCheatAction cheatAction;
					const char* sActionName = actionChild->getAttr("value");
					cheatAction.action = FindCheatAction(sActionName);
					if (cheatAction.action != eCA_Invalid)
					{
						if (cheatAction.action == eCA_Ban)
						{
							actionChild->getAttr("timeout", cheatAction.banTimeout);
						}
						actionChild->getAttr("threshhold", cheatAction.threshhold);
						if (cheatAction.threshhold > 0.f)
						{
							m_globalActions.push_back(cheatAction);
						}
						else
						{
							CryLog("Invalid threshhold %f", cheatAction.threshhold);
						}
					}
					else
					{
						CryLog("Unrecognised action %s", sActionName);
					}
				}
				else
				{
					CryLog("Unrecognised child node %s", actionChild->getTag());
				}
			}
		}
		else if (child->isTag("DataProbe"))
		{
			child->getAttr("hash_method", m_hashMethod);
			int numChildren = child->getChildCount();
			for ( int groupidx = 0; groupidx < numChildren; ++groupidx )
			{
				XmlNodeRef groupChild = child->getChild(groupidx);
				if (groupChild->isTag("Asset"))
				{
					const char* sGroupExtName = groupChild->getAttr("ext");
					TCheatAssetGroup assetGroup = FindAssetTypeByExtension(sGroupExtName);
					if (assetGroup != eAG_Invalid )
					{
						int weight = 0;
						if ( groupChild->getAttr("weight",weight) )
						{
							m_totalAssetWeighting += m_assetGroupWeights[assetGroup] = weight;
						}
						else
						{
							CryLog("Extension had no weighting %s", sGroupExtName);
						}
					}
					else
					{
						CryLog("Unrecognised extension %s", sGroupExtName);
					}
				}
				else if (groupChild->isTag("protect"))
				{
					m_fileProtectConfig = groupChild;
				}
				else
				{
					CryLog("Unrecognised child node %s", groupChild->getTag());
				}
			}
		}
		else if (child->isTag("AntiCheatVars"))
		{
			LoadAntiCheatVars(child);
		}
		else
		{
			CryLog("Unrecognised child node %s", child->getTag());
		}
	}
}

void CAntiCheatManager::LoadAntiCheatVars(XmlNodeRef child)
{
	int numVars = child->getChildCount();
	for (int i=0; i<numVars; ++i)
	{
		XmlNodeRef varsChild = child->getChild(i);
		if (varsChild->isTag("Var"))
		{
			const char* sVarName = varsChild->getAttr("name");
			TAntiCheatVarIdx antiCheatVarIdx = FindAntiCheatVarIdx_Float(sVarName);
			if (antiCheatVarIdx != eAV_Invalid_Float)
			{
				varsChild->getAttr("value", m_cheatVarsFloat[antiCheatVarIdx]);
			}
			else
			{
				antiCheatVarIdx = FindAntiCheatVarIdx_Int(sVarName);
				if (antiCheatVarIdx != eAV_Invalid_Int)
				{
					varsChild->getAttr("value", m_cheatVarsInt[antiCheatVarIdx]);
				}
				else
				{
					CryLog("Unrecognised anti cheat var '%s'", sVarName);
				}
			}
		}
		else
		{
			CryLog("Unrecognised child node '%s'", varsChild->getTag());
		}
	}
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId, const char * message)
{
  QueueFlagActivity(type, channelId, NULL, 0, message);
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId)
{
	QueueFlagActivity(type, channelId, NULL, 0, NULL);
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId, float param1)
{
	float params[] = {param1};
	QueueFlagActivity(type, channelId, params, 1, NULL);
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId, float param1, float param2)
{
	float params[] = {param1, param2};
	QueueFlagActivity(type, channelId, params, 2, NULL);
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId, float param1, const char * message)
{
	float params[] = {param1};
	QueueFlagActivity(type, channelId, params, 1, message);
}

void CAntiCheatManager::FlagActivity(TCheatType type, uint16 channelId, float param1, float param2, const char * message)
{
	float params[] = {param1, param2};
	QueueFlagActivity(type, channelId, params, 2, message);
}

void CAntiCheatManager::QueueFlagActivity(TCheatType type, uint16 channelId, const float *params, int numParams, const char * message)
{
	if (numParams > MAX_FLAG_ACTIVITY_PARAMS)
	{
		numParams = MAX_FLAG_ACTIVITY_PARAMS;
		CRY_ASSERT_MESSAGE(0, "Too many parameters");
	}
	SQueuedFlagActivity item;
	item.type = type;
	item.channelId = channelId;
	for (int i=0; i<numParams; ++i)
	{
		item.params[i] = params[i];
	}
	item.numParams = numParams;
	if (message)
	{
		item.message = message;
	}
	CryAutoCriticalSection lock(m_mutex);
	m_queuedFlagActivity.push(item);
}

void CAntiCheatManager::PlayerVotedOff( EntityId playerId, const TVoteDataList& voteData, float fSecondsToBanFor )
{
	//verify it is from the main thread. This cannot be called from other threads as it bypasses the queue

	if ( IActor * pTargetActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId))
	{
		uint16 targetActorChannel = pTargetActor->GetChannelId();

		XmlNodeRef playersVoteDataXml;
		playersVoteDataXml = GetISystem()->CreateXmlNode("VoteData");

		for(TVoteDataList::const_iterator iter = voteData.begin(), end = voteData.end(); iter != end; ++iter)
		{
			XmlNodeRef playerVoteXml = playersVoteDataXml->newChild("Vote");

			if ( IActor * pVotingActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(iter->voterId) )
			{
				uint16 channelId = pVotingActor->GetChannelId();

				if( INetChannel * pNetChannel = g_pGame->GetIGameFramework()->GetNetChannel(channelId) )
				{
					playerVoteXml->setAttr("player_nickname", pNetChannel->GetNickname() );
					playerVoteXml->setAttr("vote", iter->bVotedToKick ? "for" : "against");
				}
			}
		}

		ProcessFlagActivity( eCT_VoteKicked, targetActorChannel, 0, 0, "Player voted off by their peers", playersVoteDataXml );
	}	
}

void CAntiCheatManager::Update(float dt)
{
	while (!m_queuedFlagActivity.empty())
	{
		if (m_mutex.TryLock())
		{
			if (!m_queuedFlagActivity.empty())
			{
				// Get a copy of the item off the queue
				SQueuedFlagActivity item = m_queuedFlagActivity.front();
				m_queuedFlagActivity.pop();
				// Unlock the queue
				m_mutex.Unlock();
				// And process it
				const char* sMessage = NULL;
				if (!item.message.empty())
				{
					sMessage = item.message.c_str();
				}
				ProcessFlagActivity(item.type, item.channelId, item.params, item.numParams, sMessage, XmlNodeRef());
			}
			else
			{
				// Queue no longer has items in it? Shouldn't reach this point really because this is the only place items are popped off.
				assert(false);
				m_mutex.Unlock();
			}
		}
		else
		{
			// Try lock failed, let's do it next frame
			break;
		}
	}

	if(!m_DelayedKickData.empty())
		HandleDelayedKicks(dt);

}

void CAntiCheatManager::HandleDelayedKicks(float dt)
{
	for(int i = m_DelayedKickData.size() - 1; i >= 0; i--)
	{
		SDelayedKickData& rKickData = m_DelayedKickData[i];
		const float fTimeUntilKick = rKickData.fKickCountdown - dt;
		if(fTimeUntilKick <= 0.0f)
		{
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
				if (pMatchmaking)
				{
					pMatchmaking->Kick(&rKickData.userId, rKickData.reason);
				}
			}

			m_DelayedKickData[i] = m_DelayedKickData[m_DelayedKickData.size()-1];
			m_DelayedKickData.pop_back();
		}
		else
		{
			rKickData.fKickCountdown = fTimeUntilKick;
		}
	}
}

template <class T>
bool CAntiCheatManager::MeetsCondition(ECheatOperator op, T left, T right)
{
	switch (op)
	{
	case eCO_Equal:
		if (left == right)
		{
			return true;
		}
		break;
	case eCO_NotEqual:
		if (left != right)
		{
			return true;
		}
		break;
	case eCO_GreaterThan:
		if (left > right)
		{
			return true;
		}
		break;
	case eCO_LessThan:
		if (left < right)
		{
			return true;
		}
		break;
	case eCO_GreaterThanEqual:
		if (left >= right)
		{
			return true;
		}
		break;
	case eCO_LessThanEqual:
		if (left <= right)
		{
			return true;
		}
		break;
	}
	return false;
}

/*static*/ void CAntiCheatManager::GenerateMessageString(stack_string& outString, const float *params, int numParams, const char * message)
{
	stack_string temp;
	for (int i=0; i<numParams; ++i)
	{
		if (i > 0)
		{
			outString.append(", ");
		}
		temp.Format("%f", params[i]);
		outString.append(temp);
	}

	if ( message )
	{
		if ( numParams > 0 )
		{
			outString.append(", ");
		}
		outString.append(message);
	}
}

void CAntiCheatManager::ProcessFlagActivity(TCheatType type, uint16 channelId, const float *params, int numParams, const char * pDescription, XmlNodeRef additionalXmlData)
{
	SCheatType &cheat = m_cheats[type];
	
	XmlNodeRef incidentXML;

	float fInfractionsSeverity = 0.0f;
	int		nMaxConfidence = 0, nMaxInfractionConfidence = 0;

	typedef std::vector<const SCheatAction*> TCheatActionPtrList;
	TCheatActionPtrList actionsToTake;

	EDisconnectionCause kickReason = eDC_Kicked;
	
	for (std::vector<SCheatAction>::iterator itAction = cheat.actions.begin(); itAction != cheat.actions.end(); ++itAction)
	{
		bool meetsConditions = true;
		
		for (std::vector<SCheatCondition>::const_iterator itCondition = itAction->conditions.begin(); itCondition != itAction->conditions.end(); ++itCondition)
		{
			assert(itCondition->paramNum <= numParams);
			float paramValue = params[itCondition->paramNum-1];
			meetsConditions = MeetsCondition(itCondition->op, paramValue, itCondition->value);
			if (!meetsConditions)
			{
				break;
			}
		}

		if (meetsConditions)
		{
			actionsToTake.push_back(&(*itAction));

			if(incidentXML == NULL)
			{
				incidentXML = CreateIncidentXML(channelId, type, params, numParams, pDescription);

				if(additionalXmlData != NULL && incidentXML != NULL)
					incidentXML->addChild(additionalXmlData);
			}
		}

		if(itAction->action == eCA_Global_Ban)
		{
			kickReason = GetBanKickType(channelId);
		}
	}

	bool bDoRemoteLog = false;

	for(TCheatActionPtrList::const_iterator itActionPtr = actionsToTake.begin(); itActionPtr != actionsToTake.end(); ++itActionPtr)
	{
		const SCheatAction * actionPtr = *itActionPtr;

		nMaxConfidence = max(actionPtr->confidence, nMaxConfidence);
		
		switch (actionPtr->action)
		{
		case eCA_Log:
			//The presence of this action will cause the incident to be logged, we don't want to add 
			//	any more output as it won't provide any more data
			break;
		case eCA_Log_Remote:
			bDoRemoteLog = true;
			break;
		case eCA_Infraction:
			{
				fInfractionsSeverity += actionPtr->severity;
				nMaxInfractionConfidence =  max(actionPtr->confidence, nMaxInfractionConfidence);
			}
			break;
		case eCA_Kick:
			CheatLogAction(incidentXML, eCA_Kick, actionPtr->confidence);
			KickPlayer(channelId, kickReason, actionPtr->confidence);
			break;
		case eCA_Kick_Delayed:
			CheatLogAction(incidentXML, eCA_Kick_Delayed, actionPtr->confidence);
			KickPlayerDelayed(channelId, kickReason, actionPtr->confidence);
			break;
		case eCA_Ban:
			CheatLogAction(incidentXML, eCA_Ban, actionPtr->confidence);
			BanPlayer_Internal(channelId, actionPtr->banTimeout, actionPtr->confidence);
			break;
		case eCA_Global_Ban:
			break;
		case eCA_Dev_CheatDetected:
			{
#if defined(DEV_CHEAT_HANDLING)
				CheatLogAction(incidentXML, eCA_Dev_CheatDetected, actionPtr->confidence);

				stack_string paramsString;
				CAntiCheatManager::GenerateMessageString(paramsString, params, numParams, pDescription);
				paramsString.append(" ");
				paramsString.append(s_cheatTypeNames[type]);
				HandleDevCheat(channelId, paramsString);
#endif
				break;
			}
		}
	}

	//If multiple infractions have been triggered by the parameters, we want to log 
	if(fInfractionsSeverity > 0.0f)
	{
		XmlNodeRef actionXml = CheatLogAction(incidentXML, eCA_Infraction, nMaxInfractionConfidence);
		RegisterInfraction(actionXml, channelId, type, fInfractionsSeverity);
	}

	if(incidentXML)
	{
#if defined(USE_CRISP_THINKING)
		if(m_pCrispWrapper && bDoRemoteLog)
		{
			HandleCrispThinkingOutput(type, channelId, incidentXML, nMaxConfidence);
		}
#endif

		CheatLogInternalXml(incidentXML);
	}
}

#if defined(USE_CRISP_THINKING)

CryStackStringT<char, 256> keyString;

void CAntiCheatManager::OutputXMLToCrispThinking(XmlNodeRef xml)
{	
	for(int i = 0, count = xml->getChildCount(); i < count; i++)
	{
		OutputXMLToCrispThinking(xml->getChild(i));
	}
}

void CAntiCheatManager::HandleCrispThinkingOutput(TCheatType type, uint16 channelId, XmlNodeRef incidentXML, int nMaxConfidence)
{
	if(CGameLobby * pGameLobby = g_pGame->GetGameLobby())
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);

		CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH> userGUID("<unknown>");
		
		if(userId.IsValid())
		 userGUID = userId.get()->GetGUIDAsString().c_str();
		else
		{
			CryLog("Failed to get GUID for user on channel %u", channelId);
		}

		if(ITelemetryCollector * pTelemetryCollector = g_pGame->GetITelemetryCollector())
		{
			const char * pTelemetrySessionName = pTelemetryCollector->GetSessionId().c_str();

			CryHashStringId sessionHash(pTelemetrySessionName);

			CryFixedStringT<128> eventUID;
			eventUID.Format("%X-%s-%u", sessionHash.id, userGUID.c_str(), m_uIncidents++);

			CryStackStringT<char, 128> cheatName;
			int nConfidenceBucket = nMaxConfidence / 10;
			cheatName.FormatFast("%s_%d", s_cheatTypeNames[type], nConfidenceBucket);

			CryLog("[ANTICHEAT] %s, %s", eventUID.c_str(), userGUID.c_str());
			CryLog("[ANTICHEAT]   %s, %s", pTelemetrySessionName, cheatName.c_str());
			m_pCrispWrapper->StartEvent(eventUID.c_str(), userGUID.c_str(), pTelemetrySessionName, cheatName.c_str());

			OutputXMLToCrispThinking(incidentXML);

			bool bQueued = m_pCrispWrapper->EndEvent();

			if(!bQueued)
				CryLogAlways("[ERROR] Failed to queue Crisp Thinking event");
		}			
	}
}
#endif

EDisconnectionCause CAntiCheatManager::GetBanKickType(uint16 channelId)
{
	EDisconnectionCause disconnectCause = eDC_Banned;

	if (CGameLobby* pGameLobby = g_pGame->GetGameLobby())
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);

		disconnectCause = eDC_Global_Ban1;
	}

	CryLog("CAntiCheatManager::GetBanKickType() user '%s' %s", "", disconnectCause == eDC_Banned ? "Locally banned" : disconnectCause == eDC_Global_Ban1 ? "Globally banned stage 1" : "Globally banned stage 2");

	return disconnectCause;
}

void CAntiCheatManager::DumpPlayerRecords()
{
	XmlNodeRef playerRecords = GetISystem()->CreateXmlNode("PlayerSessionRecords");

	CGameRules *pGameRules = g_pGame->GetGameRules();
	IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();

	for (TPlayerSessionDataMap::iterator itPlayerRecord = m_PlayerSessionData.begin(); itPlayerRecord != m_PlayerSessionData.end(); ++itPlayerRecord)
	{
		XmlNodeRef playerSession = playerRecords->newChild("PlayerSession");

		playerSession->setAttr("local_session_id", itPlayerRecord->first);

		SPlayerSessionData& rPlayerSessionData = itPlayerRecord->second;
		playerSession->setAttr("player_nickname", rPlayerSessionData.playerName);
		playerSession->setAttr("connect_game_time", rPlayerSessionData.connectTime.GetMilliSecondsAsInt64());
		
		int64 nDisconnectTime = itPlayerRecord->second.disconnectTime.GetMilliSecondsAsInt64();
		
		//nDisconnectTime == 0 means that it has not been set, and so the player did not disconnect and was present at round end
		if(nDisconnectTime != 0)
		{
			playerSession->setAttr("disconnect_game_time", nDisconnectTime);
		}
		else
		{
			if(IActor * pActor = pGameRules->GetActorByChannelId(itPlayerRecord->first))
			{
				GetPlayerStats(pPlayerStatsModule, pActor->GetEntityId(), rPlayerSessionData);
			}			
		}

		playerSession->setAttr("kills", rPlayerSessionData.kills);
		playerSession->setAttr("deaths", rPlayerSessionData.deaths);
		playerSession->setAttr("points", rPlayerSessionData.points);
	}

	CheatLogInternalXml(playerRecords);

#if defined(USE_CRISP_THINKING)
	if(GetAntiCheatVar(eAV_3P_StatsUpload, 1) > 0)
	{
		if(ITelemetryCollector * pTelemetryCollector = g_pGame->GetITelemetryCollector())
		{
			if(const char * pTelemetrySessionName = pTelemetryCollector->GetSessionId().c_str())
			{
				CryHashStringId sessionHash(pTelemetrySessionName);

				CryFixedStringT<128> eventUID;

				eventUID.Format("%X-%s-%u", sessionHash.id, "SessionStats", m_uIncidents++);

				CryLog("[ANTICHEAT] %s, %s, %s", eventUID.c_str(), pTelemetrySessionName, "SessionStats");
				m_pCrispWrapper->StartEvent(eventUID.c_str(), pTelemetrySessionName, pTelemetrySessionName, "SessionStats");

				OutputXMLToCrispThinking(playerRecords);

				bool bQueued = m_pCrispWrapper->EndEvent();
			}
		}
	}
#endif

	m_PlayerSessionData.clear();
}

void CAntiCheatManager::DumpCheatRecords()
{
	XmlNodeRef records = GetISystem()->CreateXmlNode("CheatRecords");

	std::map<CryUserID, SCheatRecord>::iterator itCheatRecord;
	for (itCheatRecord = m_cheatRecords.begin(); itCheatRecord != m_cheatRecords.end(); ++itCheatRecord)
	{
		XmlNodeRef playerRecord = records->newChild("Player");

		const char* szGuid = itCheatRecord->first.get()->GetGUIDAsString();
		CAntiCheatManager::Xml_SetGuid(playerRecord, szGuid);

		for (int i=0; i<eCT_Num; ++i)
		{
			SCheatInfraction &infraction = itCheatRecord->second.infractions[i];
			if (infraction.numInfractions > 0 || infraction.cumulativeSeverity  > 0.f)
			{
				XmlNodeRef infractionXml = playerRecord->newChild("Infraction");

				CAntiCheatManager::Xml_SetCheatType(infractionXml, i);

				infractionXml->setAttr("incidences", infraction.numInfractions);
				infractionXml->setAttr("cumulative_severity", infraction.cumulativeSeverity);
			}
		}
	}

	CheatLogInternalXml(records);
}

XmlNodeRef CAntiCheatManager::CreateIncidentXML(uint16 channelId, TCheatType type, const float *params, int numParams, const char * pDescription)
{
	XmlNodeRef incidentXml = GetISystem()->CreateXmlNode("Incident");

	CAntiCheatManager::Xml_SetDisplayName(incidentXml, channelId);
	CAntiCheatManager::Xml_SetGuid(incidentXml, channelId);
	CAntiCheatManager::Xml_SetCheatType(incidentXml, type);

	if(pDescription && pDescription[0])
		incidentXml->setAttr("description", pDescription);

	for(int i = 0; i < numParams; i++)
	{
		CryStackStringT<char, 16> paramNodeName;
		paramNodeName.FormatFast("%s-%d", PARAM_NODE, i+1);
		XmlNodeRef child = incidentXml->newChild(paramNodeName.c_str());
		child->setAttr(PARAM_VALUE, params[i]);
	}

	return incidentXml;
}

void CAntiCheatManager::DecayCheatRecords()
{
	std::map<CryUserID, SCheatRecord>::iterator itCheatRecord;
	for (itCheatRecord = m_cheatRecords.begin(); itCheatRecord != m_cheatRecords.end(); )
	{
		bool remove = true;
		for (int i=0; i<eCT_Num; ++i)
		{
			SCheatInfraction &infraction = itCheatRecord->second.infractions[i];
			infraction.cumulativeSeverity -= m_decayRate;
			if (infraction.cumulativeSeverity <= 0.f)
			{
				infraction.cumulativeSeverity = 0.f;
			}
			else
			{
				remove = false;
			}
		}
		if (remove)
		{
			std::map<CryUserID, SCheatRecord>::iterator itRemove = itCheatRecord;
			++itCheatRecord;
			m_cheatRecords.erase(itRemove);
		}
		else
		{
			++itCheatRecord;
		}
	}
}

void CAntiCheatManager::InitSession()
{
	OpenLogFile();

	const char * pSessionName = NULL;
	char sessionID[CRYSESSIONID_STRINGLEN];
	sessionID[0] = 0;
	
	if(CGameLobby * pGameLobby = g_pGame->GetGameLobby())
	{
		pSessionName = pGameLobby->GetSessionName();
		if (pSessionName && pSessionName[0]==0)
			pSessionName=NULL;
		pGameLobby->GetSessionIDAsString(sessionID, CRYSESSIONID_STRINGLEN);
	} 

	//This can't be logged via the cleaner XML method as we need to open the tag now, and close it later on, without leaving everything in memory the whole time
	CryStackStringT<char, 512> sessionStartTag;

	const char * pTelemetrySessionName = NULL;

	if(ITelemetryCollector * pTelemetryCollector = g_pGame->GetITelemetryCollector())
	{
		pTelemetrySessionName = pTelemetryCollector->GetSessionId().c_str();
	}

	sessionStartTag.Format("<Session name=\"%s\" telemetry_name=\"%s\" id=\"%s\">\n", pSessionName ? pSessionName : "UNKNOWN", pTelemetrySessionName ? pTelemetrySessionName : "UNKNOWN", sessionID[0] ? sessionID : "UNKNOWN");

	if (m_pLogFile)
	{
		gEnv->pCryPak->FWrite(sessionStartTag.c_str(), 1, sessionStartTag.length(), m_pLogFile);
	}

	if(CGameRules * pGameRules = g_pGame->GetGameRules())
	{
		XmlNodeRef sessionGameInfo = GetISystem()->CreateXmlNode("GameInfo");

		ILevel * pLevel = g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();

		sessionGameInfo->setAttr("map",				pLevel->GetLevelInfo()->GetName());
		sessionGameInfo->setAttr("gamemode",	CGameRules::S_GetGameModeNamesArray()[pGameRules->GetGameMode()]);

		CheatLogInternalXml(sessionGameInfo);
	}
}

void CAntiCheatManager::OnSessionEnd()
{
	DumpPlayerRecords();
	DumpCheatRecords();
	DecayCheatRecords();
	
	CloseLogFile();
	BackupLogFileAndSubmit();	

	CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();
	float deltaSeconds = currentTime.GetDifferenceInSeconds(m_lastDownloadTime);
	if (deltaSeconds > g_pGameCVars->g_dataRefreshFrequency * 3600)
	{
		CDownloadableResourcePtr res = GetDownloadableResource();
		if (res)
		{
			// Clear the downloaded data and start download again
			res->Purge();
			res->StartDownloading();
		}
		m_lastDownloadTime = currentTime;
	}
}

bool CAntiCheatManager::IsCheatProtectionEnabled(TCheatType cheatType)
{
	SCheatType &cheat = m_cheats[cheatType];
	return !cheat.actions.empty();
}

void CAntiCheatManager::RegisterInfraction(XmlNodeRef xmlNode, uint16 channelId, TCheatType type, float severity)
{
	if (CGameLobby* pGameLobby = g_pGame->GetGameLobby())
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);
		if (userId.IsValid())
		{
			std::map<CryUserID, SCheatRecord>::iterator itCheatRecord = m_cheatRecords.find(userId);
			if (itCheatRecord == m_cheatRecords.end())
			{
				itCheatRecord = m_cheatRecords.insert(std::make_pair(userId, SCheatRecord())).first;
			}
			SCheatInfraction &infraction = itCheatRecord->second.infractions[type];
			infraction.cumulativeSeverity += severity;
			// Never go above 100
			if (infraction.cumulativeSeverity > 100.f)
				infraction.cumulativeSeverity = 100.f;
			infraction.numInfractions++;

			VerifyInfractions(xmlNode, channelId, itCheatRecord->second, severity);
		}
	}
}

void CAntiCheatManager::VerifyInfractions(XmlNodeRef incidentXml, uint16 channelId, const SCheatRecord &cheatRecord, float fDeltaSeverity)
{
	float overallSeverity = 0.f;
	int numInfractions = 0;
	for (int i=0; i<eCT_Num; ++i)
	{
		overallSeverity += min(cheatRecord.infractions[i].cumulativeSeverity, 100.f);
		numInfractions	+= cheatRecord.infractions[i].numInfractions;
	}
	
	CheatLogSeverity(incidentXml, channelId, overallSeverity, numInfractions, fDeltaSeverity);
	std::vector<SGlobalCheatAction>::iterator itGlobalAction;
	for (itGlobalAction = m_globalActions.begin(); itGlobalAction != m_globalActions.end(); ++itGlobalAction)
	{
		if (overallSeverity >= itGlobalAction->threshhold)
		{
			switch (itGlobalAction->action)
			{
			case eCA_Kick:
				KickPlayer(channelId, eDC_Kicked);
				break;
			case eCA_Kick_Delayed:
				KickPlayerDelayed(channelId, eDC_Kicked);
				break;
			case eCA_Ban:
				BanPlayer_Internal(channelId, itGlobalAction->banTimeout);
				break;
			}

			CheatLogAction(incidentXml, itGlobalAction->action, kDefaultConfidence);
		}
	}
}

void CAntiCheatManager::KickPlayer(uint16 channelId, EDisconnectionCause reason, int nConfidence)
{
	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);

		KickPlayer(userId, reason, nConfidence);
	}
}

void CAntiCheatManager::KickPlayer(const CryUserID& userId, EDisconnectionCause reason, int nConfidence)
{
	if (userId.IsValid())
	{
		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
			if (pMatchmaking)
			{
				pMatchmaking->Kick(&userId, reason);
			}
		}
	}
}

void CAntiCheatManager::BanPlayer(uint16 channelId, const char* reason, float timeout)
{
	XmlNodeRef incidentXML = CreateIncidentXML(channelId, eCT_External, NULL, 0, reason);
	CheatLogAction(incidentXML, eCA_Ban, GetAntiCheatVar(eAV_IP_VoteKickConfidence, 10));
	BanPlayer_Internal(channelId, timeout);
}

void CAntiCheatManager::BanPlayer_Internal(uint16 channelId, float timeout, int nConfidence)
{
	CGameLobby* pGameLobby = g_pGame->GetGameLobby();
	if (pGameLobby)
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);
		if (userId.IsValid())
		{
			ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
			if (pLobby)
			{
				ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
				if (pMatchmaking)
				{
					pMatchmaking->Ban(&userId, timeout);
				}
			}
		}
	}
}

#if defined(DEV_CHEAT_HANDLING)
void CAntiCheatManager::HandleDevCheat(uint16 channelId, const char * message)
{
	if(CGameRules * pGameRules = g_pGame->GetGameRules())
	{
		pGameRules->HandleDevCheat(channelId, message);
	}
}
#endif

void CAntiCheatManager::KickPlayerDelayed(uint16 channelId, EDisconnectionCause reason, int nConfidence)
{
	if (CGameLobby* pGameLobby = g_pGame->GetGameLobby())
	{
		CryUserID userId = pGameLobby->GetUserIDFromChannelID(channelId);

		SDelayedKickData kickData;
		const float fMinDelay = GetAntiCheatVar(eAV_KD_Min, 5.0f);
		kickData.fKickCountdown = (cry_frand() * (GetAntiCheatVar(eAV_KD_Max, 25.0f) - fMinDelay)) + fMinDelay;	
		kickData.userId = userId;
		kickData.channelId = channelId;
		kickData.reason = reason;

		m_DelayedKickData.push_back(kickData);
	}
}

/*static*/ void CAntiCheatManager::Xml_SetGuid(XmlNodeRef xmlNode, uint16 channelId)
{
	const char * pGuid = "Unknown";
	if (CGameLobby* pGameLobby = g_pGame->GetGameLobby())
	{
		CryUserID cryUserId = pGameLobby->GetUserIDFromChannelID(channelId);
		if (cryUserId.IsValid())
		{
			pGuid = cryUserId.get()->GetGUIDAsString().c_str();
		}
	}

	xmlNode->setAttr("user_guid", pGuid);
}

/*static*/ void CAntiCheatManager::Xml_SetGuid(XmlNodeRef xmlNode, const char * pGuid)
{
	xmlNode->setAttr("user_guid", pGuid);
}

/*static*/ void CAntiCheatManager::Xml_SetDisplayName(XmlNodeRef xmlNode, uint16 channelId)
{
	CryFixedStringT<DISPLAY_NAME_LENGTH> displayName = "Unknown";
	xmlNode->setAttr("user_name", displayName);
}


/*static*/ void CAntiCheatManager::Xml_SetCheatType(XmlNodeRef xmlNode, TCheatType eCheatType)
{
	xmlNode->setAttr("cheat_type", s_cheatTypeNames[eCheatType]);
}

/*static*/ void CAntiCheatManager::Xml_SetActionType(XmlNodeRef xmlNode, ECheatAction eCheatAction)
{
	xmlNode->setAttr("type", s_cheatActionNames[eCheatAction]);
}

XmlNodeRef CAntiCheatManager::CheatLogAction(XmlNodeRef incidentNode, ECheatAction eCheatAction, int nConfidence)
{
	XmlNodeRef actionNode = incidentNode->newChild("Action");

	CAntiCheatManager::Xml_SetActionType(actionNode, eCheatAction);
	
	actionNode->setAttr("confidence", nConfidence);

	return actionNode;
}

void CAntiCheatManager::CheatLogSeverity(XmlNodeRef incidentXml, uint16 channelId, float fTotalSeverity, int numInfractions, float fDeltaSeverity)
{
	XmlNodeRef severityNode = incidentXml->newChild("Severity");
	severityNode->setAttr("change", fDeltaSeverity);
	severityNode->setAttr("total", fTotalSeverity);
	severityNode->setAttr("num_infractions", numInfractions);	
}

void CAntiCheatManager::CheatLogInternalXml(XmlNodeRef xmlNode)
{
	char sTime[128];
	time_t ltime;
	time( &ltime );
	struct tm *today = localtime( &ltime );
	strftime( sTime, 20, "%H:%M:%S", today );

	xmlNode->setAttr("time", sTime);

	const int64 time = gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI).GetMilliSecondsAsInt64();
	xmlNode->setAttr("game_time", time);


	// Print to the console and normal game log
	IXmlStringData * pXMLStringData = xmlNode->getXMLData();
	CryLog("AntiCheatManager: %s", pXMLStringData->GetString());

	// But also log to AntiCheatLog.xml which will be uploaded via telemetry
	if (m_pLogFile)
	{
		gEnv->pCryPak->FWrite(pXMLStringData->GetString(), 1, pXMLStringData->GetStringLength(), m_pLogFile);
	}

	pXMLStringData->Release();
}

void CAntiCheatManager::ChatMessage(uint16 channelId, const char * pString)
{
#if defined(USE_CRISP_THINKING)
	if(m_pCrispWrapper)
	{
		const char * pTelemetrySessionName = NULL;

		if(ITelemetryCollector * pTelemetryCollector = g_pGame->GetITelemetryCollector())
		{
			pTelemetrySessionName = pTelemetryCollector->GetSessionId().c_str();
		}

		CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH> userGUID;
		if (CGameLobby* pGameLobby = g_pGame->GetGameLobby())
		{
			CryUserID cryUserId = pGameLobby->GetUserIDFromChannelID(channelId);
			if (cryUserId.IsValid())
			{
				userGUID = cryUserId.get()->GetGUIDAsString();
			}
		}

		if(pTelemetrySessionName && userGUID.length() > 0)
		{
			CryHashStringId sessionHash(pTelemetrySessionName);
			CryHashStringId chatHash(pString);

			CryFixedStringT<128> chatUID;
			chatUID.Format("%X-%X-%u", sessionHash.id, chatHash.id, m_uChatMsgsSent++);

			CryLog("CrispWrapper::ChatMessage( %s, %s, %s, %s)", chatUID.c_str(), userGUID.c_str(), pTelemetrySessionName, pString);
			m_pCrispWrapper->ChatMessage(chatUID.c_str(), userGUID.c_str(), pTelemetrySessionName, pString);
		}
	}
#endif
}

void CAntiCheatManager::InsertedUser(CryUserID userId, const char *userName)
{
}

void CAntiCheatManager::OnClientConnect(INetChannel& rNetChannel)
{
	uint16 channelId = rNetChannel.GetLocalChannelID();

	TPlayerSessionDataMap::const_iterator existingData = m_PlayerSessionData.find(channelId);
	if(existingData == m_PlayerSessionData.end())
	{
		SPlayerSessionData sessionData;
		sessionData.connectTime = gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI);
		sessionData.playerName	= rNetChannel.GetNickname();

		m_PlayerSessionData.insert(TPlayerSessionPair(channelId, sessionData));
	}
}

void CAntiCheatManager::OnClientDisconnect(INetChannel& rNetChannel, IActor * pActor)
{
	uint16 channelId = rNetChannel.GetLocalChannelID();

	TPlayerSessionDataMap::iterator existingData = m_PlayerSessionData.find(channelId);
	if(existingData != m_PlayerSessionData.end())
	{
		existingData->second.disconnectTime = gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI);

		CGameRules *pGameRules = g_pGame->GetGameRules();
		IGameRulesPlayerStatsModule *pPlayerStatsModule = pGameRules->GetPlayerStatsModule();
		pGameRules->GetActorByChannelId(channelId);
		GetPlayerStats(pPlayerStatsModule, pActor->GetEntityId(), existingData->second);
	}
}

void CAntiCheatManager::GetPlayerStats(IGameRulesPlayerStatsModule * pPlayerStatsModule, EntityId playerId, SPlayerSessionData& sessionData)
{
	if (pPlayerStatsModule)
	{
		if(const SGameRulesPlayerStat *playerStats = pPlayerStatsModule->GetPlayerStats(playerId))
		{
			sessionData.points	= playerStats->points;
			sessionData.kills		=	playerStats->kills;
			sessionData.deaths	= playerStats->deaths;
		}
	}
}

void CAntiCheatManager::OpenLogFile()
{
	assert(!m_pLogFile);
	CDebugAllowFileAccess allowFileAccess;
	m_pLogFile = gEnv->pCryPak->FOpen(m_logFileName.c_str(), "wt");
}

void CAntiCheatManager::CloseLogFile()
{
	if (m_pLogFile)
	{
		const char * pEndXMLTag = "</Session>";

		gEnv->pCryPak->FWrite(pEndXMLTag, 1, strlen(pEndXMLTag), m_pLogFile);

		gEnv->pCryPak->FClose(m_pLogFile);
		m_pLogFile = NULL;
	}
}

int CAntiCheatManager::GetLogFileSize()
{
	if (m_pLogFile)
	{
		return (int)gEnv->pCryPak->FTell(m_pLogFile);
	}
	return -1;
}

void CAntiCheatManager::BackupLogFileAndSubmit()
{
	// Get time.
	time_t ltime;
	time( &ltime );
	tm *today = localtime( &ltime );
	char szDateStamp[1024];
	strftime(szDateStamp, 1024, "%d %b %y (%H %M %S)", today);

	stack_string szBackupName;

	int instance = gEnv->pSystem->GetApplicationInstance();
	if(instance != 0)
	{
		szBackupName.Format("./%sAntiCheat/AntiCheatLog(%d) %s.xml", gEnv->pSystem->GetRootFolder(), instance, szDateStamp);
	}
	else
	{
		szBackupName.Format("./%sAntiCheat/AntiCheatLog %s.xml", gEnv->pSystem->GetRootFolder(), szDateStamp);
	}	

	CDebugAllowFileAccess allowFileAccess;
	FILE* fFrom = gEnv->pCryPak->FOpen(m_logFileName.c_str(), "rb", ICryPak::FOPEN_ONDISK);
	if (NULL == fFrom)
	{
		return;
	}

	FILE* fTo = gEnv->pCryPak->FOpen(szBackupName.c_str(), "wb");
	if (NULL == fTo)
	{
		gEnv->pCryPak->FClose(fFrom);
		return;
	}

	const int COPY_FILE_BUF_SIZE = 1024;
	char buf[COPY_FILE_BUF_SIZE];
	size_t lenRead;

	while (!feof(fFrom))
	{
		lenRead = gEnv->pCryPak->FRead(buf, COPY_FILE_BUF_SIZE, fFrom);
		gEnv->pCryPak->FWrite(buf, lenRead, fTo);
	}

	gEnv->pCryPak->FClose(fFrom);
	gEnv->pCryPak->FClose(fTo);

	if (m_enableLogUploads)
	{
		CryLog("Uploading AC records");
		// Upload file to telemetry-collector server
		CTelemetryCollector	* pTelemetryCollector = static_cast<CTelemetryCollector*>(g_pGame->GetITelemetryCollector());
		if (pTelemetryCollector)
		{
			ITelemetryProducer *pProducer = new CTelemetryFileReader(szBackupName.c_str(), 0);
			if (pProducer)
			{
				ITelemetryProducer *pCompress = new CTelemetryCompressor(pProducer);
				if (pCompress)
				{
					pTelemetryCollector->SubmitTelemetryProducer(pCompress, "AntiCheatLog.xml.gz");
				}
				else
				{
					delete pProducer;
				}
			}
		}
	}
	else
	{
		CryLog("Not uploading AC records; uploading disabled");
	}
}

#endif
