/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
*************************************************************************/

#include "StdAfx.h"
#include "GameAudio.h"
#include "GameAudioUtils.h"
#include "Game.h"
#include "GameRules.h"
#include "Scriptbind_GameAudio.h"
#include <IEntitySystem.h>
#include <StlUtils.h>
#include "TypeInfo_impl.h"
#include "StringUtils.h"
#include "Network/Lobby/GameLobby.h"
 
//const CGameAudio::SSoundSemanticTranslationTableEntry CGameAudio::SoundSemanticTranslationTable[]=
//{
//	{ "None",                 eSoundSemantic_None },
//	{ "OnlyVoice",            eSoundSemantic_OnlyVoice },
//	{ "NoVoice",              eSoundSemantic_NoVoice },
//	{ "Sandbox",              eSoundSemantic_Sandbox },
//	{ "Sandbox_Browser",      eSoundSemantic_Sandbox_Browser },
//
//	{ "Unused1",              eSoundSemantic_Unused1 },
//	{ "Unused2",              eSoundSemantic_Unused2 },
//	{ "Ambience",             eSoundSemantic_Ambience },
//	{ "Ambience_OneShot",     eSoundSemantic_Ambience_OneShot },
//
//	{ "Physics_Collision",    eSoundSemantic_Physics_Collision },
//	{ "Dialog",               eSoundSemantic_Dialog },
//	{ "MP_Chat",              eSoundSemantic_MP_Chat },
//	{ "Physics_Footstep",     eSoundSemantic_Physics_Footstep },
//
//	{ "Physics_General",      eSoundSemantic_Physics_General },
//	{ "HUD",                  eSoundSemantic_HUD },
//	{ "Replay",               eSoundSemantic_Replay },
//	{ "FlowGraph",            eSoundSemantic_FlowGraph },
//
//	{ "Player_Foley_Voice",   eSoundSemantic_Player_Foley_Voice },
//	{ "Living_Entity",        eSoundSemantic_Living_Entity },
//	{ "Mechanic_Entity",      eSoundSemantic_Mechanic_Entity },
//
//	{ "SoundSpot",            eSoundSemantic_SoundSpot },
//	{ "Particle",             eSoundSemantic_Particle },
//	{ "AI_Pain_Death",        eSoundSemantic_AI_Pain_Death },
//	{ "AI_Readability",       eSoundSemantic_AI_Readability },
//
//	{ "AI_Readability_Response", eSoundSemantic_AI_Readability_Response },
//	{ "TrackView",            eSoundSemantic_TrackView },
//	{ "Projectile",           eSoundSemantic_Projectile },
//	{ "Vehicle",              eSoundSemantic_Vehicle },
//
//	{ "Weapon",               eSoundSemantic_Weapon },
//	{ "Explosion",            eSoundSemantic_Explosion },
//	{ "Player_Foley",         eSoundSemantic_Player_Foley },
//	{ "Animation",            eSoundSemantic_Animation },
//
//	{ "",                     eSoundSemantic_None },
//};
//
//const CGameAudio::SSoundFlagTranslationTableEntry CGameAudio::SoundFlagTranslationTable[]=
//{
//	{ "voice",                 FLAG_SOUND_VOICE },
//	{ "onlyUpdateDistOnStart", FLAG_SOUND_ONLY_UPDATE_DIST_ON_START },
//};

const CGameAudio::SCommandNameTranslationTableEntry CGameAudio::CommandNamesTranslataionTable[]=
{
	{ "SetSoundMood", CGameAudio::eCM_Mood_Set },
	{ "AddSoundMood", CGameAudio::eCM_Mood_Add },
	{ "RemoveSoundMood", CGameAudio::eCM_Mood_Remove },
	{ "MusicSerialize", CGameAudio::eCM_Music_Serialize },
	{ "MusicEndTheme", CGameAudio::eCM_Music_EndTheme },
	{ "MusicTheme", CGameAudio::eCM_Music_Theme },
	{ "MusicMood", CGameAudio::eCM_Music_Mood },
	{ "MusicDefaultMood", CGameAudio::eCM_Music_DefaultMood },
	{ "CacheAudio", CGameAudio::eCM_Cache_Audio },
	{ "MusicPattern", CGameAudio::eCM_Music_Pattern },
};

const CGameAudio::SThemeFadeTypeTranslationTableEntry CGameAudio::ThemeFadeTypeTranslataionTable[]=
{
	{ "stopAtOnce", EThemeFade_StopAtOnce },
	{ "fadeOut", EThemeFade_FadeOut },
	{ "playEnd", EThemeFade_PlayEnd },
	{ "playEndAtFadePoint", EThemeFade_PlayEndAtFadePoint },
	{ "keepMood", EThemeFade_KeepMood },
};


const CGameAudio::CAudioSignal::SFlagTableEntry CGameAudio::CAudioSignal::SoundFlagTable[] =
{
	{ "none", eF_None },
	{ "playRandom", eF_PlayRandom },
};

const CGameAudio::SMPGameRulesThemeEntry CGameAudio::MPGameRulesThemes[] =
{
	{ eGM_SinglePlayer, "" },												// at index eGM_SinglePlayer
	{ eGM_AllOrNothing, ""},												// at index eGM_AllOrNothing 
	{ eGM_Assault, "assault"},											// at index eGM_Assault   
	{ eGM_BombTheBase, ""},													// at index eGM_BombTheBase 
	{ eGM_CaptureTheFlag, "ctr"},										// at index eGM_CaptureTheFlag
	{ eGM_Countdown, ""},														// at index eGM_Countdown
	{ eGM_CrashSite, "crashsite"},									// at index eGM_CrashSite
	{ eGM_Extraction, "extraction"},								// at index eGM_Extraction 
	{ eGM_InstantAction, "instantaction"},					// at index eGM_InstantAction 
	{ eGM_TeamInstantAction, "teaminstantaction"},	// at index eGM_TeamInstantAction
	{ eGM_PowerStruggle, "spears"},									// at index eGM_PowerStruggle
	{ eGM_Gladiator, "predator"},										// at index eGM_Gladiator
	{ eGM_DeathMatch, "predator"},										// at index eGM_Gladiator
};

#define MOODS_FILE "Libs/GameAudio/Moods.xml"
#define SIGNAL_FILES_FILE "Libs/GameAudio/Files.xml"
#define SIGNAL_FILES_FILE_MP "Libs/GameAudio/FilesMP.xml"

#define XMLSIGNALS_ERROR(xmlFilename, XmlNode, error, ...) CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Reading File: %s Line: %d. " error, xmlFilename, XmlNode->getLine(), __VA_ARGS__ );

#if !defined(_RELEASE)
struct SSignalAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const 
	{ 
		return g_pGame->GetGameAudio()->m_SPSignalsData.audioSignals.size() + g_pGame->GetGameAudio()->m_MPSignalsData.audioSignals.size(); 
	};
	virtual const char* GetValue( int _nIndex ) const 
	{ 
		uint32 numSPSignals = g_pGame->GetGameAudio()->m_SPSignalsData.audioSignals.size();
		uint32 nIndex = (uint32)_nIndex;
		if (nIndex<numSPSignals)
			return g_pGame->GetGameAudio()->m_SPSignalsData.audioSignals[nIndex].m_signalName; 
		else
			return g_pGame->GetGameAudio()->m_MPSignalsData.audioSignals[nIndex-numSPSignals].m_signalName; 
	};
};

static SSignalAutoComplete s_signalAutoComplete;
static CAudioSignalPlayer s_debugPlaySignalOnEntitySignal;

#endif

//////////////////////////////////////////////////////////////////////////

CGameAudio::CGameAudio()
: m_pScriptbind( NULL )
, m_pUtils( NULL )
, m_MPAudioSignalsLoaded ( false )
{
	CRY_ASSERT(gEnv->pAudioSystem != 0);
	//CRY_ASSERT(gEnv->pSoundSystem != 0);

	LoadMoodsFromXML( MOODS_FILE );
	LoadSignalFilesFromXml( SIGNAL_FILES_FILE, m_SPSignalsData );

	bool shouldLoadMPAudioSignals = false;
#if !defined(_RELEASE)
	if (g_pGameCVars->g_keepMPAudioSignalsPermanently==1)
		shouldLoadMPAudioSignals = true;
#endif

	if (gEnv->IsEditor())
		shouldLoadMPAudioSignals = true;

	if (shouldLoadMPAudioSignals)
		LoadMPAudioSignals();

	m_pScriptbind = new CScriptbind_GameAudio;
	m_pUtils = new CGameAudioUtils;
	m_MusicSaveLoadIDStack.reserve( 8 ); // actually should never probably use more than 2-3

#if !defined(_RELEASE)
	if (gEnv->pConsole)
	{
		REGISTER_COMMAND("PlaySignal", CGameAudio::CmdPlaySignal, 0, "Play an signal");
		REGISTER_COMMAND("PlaySignalOnEntity", CGameAudio::CmdPlaySignalOnEntity, 0, "Play an signal on an entity");
		REGISTER_COMMAND("StopSignalsOnEntity", CGameAudio::CmdStopSignalsOnEntity, 0, "Stop playing signals on an entity");
		REGISTER_COMMAND("PlayAllSignals", CGameAudio::CmdPlayAllSignals, 0, "Play all signals...");
		REGISTER_COMMAND("CacheAudioFile", CGameAudio::CmdCacheAudioFile, 0, "CacheAudio GameHint");
		REGISTER_COMMAND("UnCacheAudioFile", CGameAudio::CmdRemoveCacheAudioFile, 0, "Remove CacheAudio GameHint");
		gEnv->pConsole->RegisterAutoComplete("PlaySignal", & s_signalAutoComplete);
		gEnv->pConsole->RegisterAutoComplete("PlaySignalOnEntity", & s_signalAutoComplete);
	}

	// sanity check our MPGameRulesThemes
	for (int i=0; i<eGM_NUM_GAMEMODES; i++)
	{
		if (CGameAudio::MPGameRulesThemes[i].gameRules != i)
		{
			CryLog("CGameAudio::CGameAudio() MPGameRulesThemes[] is out of sync with the EGameMode enum! This will lead to undesirable MP music playback");
			CryFatalError("CGameAudio::CGameAudio() MPGameRulesThemes[] is out of sync with the EGameMode enum! This will lead to undesirable MP music playback");
		}
	}
#endif
}


//////////////////////////////////////////////////////////////////////////
void CGameAudio::LoadMPAudioSignals()
{
	if (!m_MPAudioSignalsLoaded)
	{
		LoadSignalFilesFromXml( SIGNAL_FILES_FILE_MP, m_MPSignalsData );
		m_MPAudioSignalsLoaded = true;
	}
}


//////////////////////////////////////////////////////////////////////////
void CGameAudio::UnloadMPAudioSignals()
{
#if !defined(_RELEASE)
	if (g_pGameCVars->g_keepMPAudioSignalsPermanently==1)
		return;
#endif

	if (!m_MPAudioSignalsLoaded || gEnv->IsEditor())
		return;

	m_MPSignalsData.Unload();
	m_MPAudioSignalsLoaded = false;
}


//////////////////////////////////////////////////////////////////////////
void CGameAudio::SAudioSignalsData::Unload()
{
	stl::free_container( nameToSignalIndexMap );
	stl::free_container( audioSignals );
	for (uint32 i=0; i<commands.size(); ++i)
	{
		CCommand* pCommand = commands[i];
		delete pCommand;
	}
	stl::free_container( commands );
}


//////////////////////////////////////////////////////////////////////////

CGameAudio::~CGameAudio()
{
	SAFE_DELETE( m_pScriptbind );
	SAFE_DELETE( m_pUtils );
}


//////////////////////////////////////////////////////////////////////////

TAudioSignalID CGameAudio::GetSignalID( const char* pSignalName, bool outputWarning)
{
	CRY_ASSERT(pSignalName && pSignalName[0]);

	TAudioSignalID ID = INVALID_AUDIOSIGNAL_ID;

	TNameToSignalIndexMap::const_iterator iter = m_SPSignalsData.nameToSignalIndexMap.find( CONST_TEMP_STRING(pSignalName) );
	if (iter==m_SPSignalsData.nameToSignalIndexMap.end())
	{
		iter = m_MPSignalsData.nameToSignalIndexMap.find( CONST_TEMP_STRING(pSignalName) );
		if (iter!=m_MPSignalsData.nameToSignalIndexMap.end())
			ID = iter->second + m_SPSignalsData.audioSignals.size(); // MP IDs start after SPs ones.
	}
	else
		ID = iter->second;
		
#if !defined(_RELEASE)
	if(ID==INVALID_AUDIOSIGNAL_ID && outputWarning)
	{
		//CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignal: '%s' not found", pSignalName);
	}	
#endif
	return ID;
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::Reset()
{
	m_pScriptbind->Reset();
	m_pUtils->Reset();
	stl::free_container(m_MusicSaveLoadIDStack);
	ClearSoundMoods();
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::ClearSoundMoods()
{
	REINST("do we still need this?");
	/*ISoundMoodManager* moodManager = gEnv->pSoundSystem->GetIMoodManager();
	for (uint32 moodIndex=0; moodIndex < m_activeMoods.size(); ++moodIndex)
	{
		moodManager->UnregisterSoundMood(m_activeMoods[moodIndex]->m_name.c_str());
	}*/
	stl::free_container(m_activeMoods);
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::LoadMoodsFromXML( const char* xmlFilename )
{
	XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile(xmlFilename);

	if (xmlNode==NULL)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CGameAudio::LoadMoodsFromXML: Failed to load'%s'", xmlFilename);
		return;
	}

	const uint32 moodCount = xmlNode->getChildCount();

	m_moodsLibrary.reserve(m_moodsLibrary.size() + moodCount);
	for (uint32 moodIndex = 0; moodIndex < moodCount; ++moodIndex)
	{
		const XmlNodeRef currentMoodNode = xmlNode->getChild(moodIndex); 

		CMood newMood;
		newMood.m_name = currentMoodNode->getAttr("name");
		currentMoodNode->getAttr("fadeInTimeMSecs", newMood.m_fadeInTimeMSecs);
		currentMoodNode->getAttr("fadeOutTimeMSecs", newMood.m_fadeOutTimeMSecs);

		m_moodsLibrary.push_back( newMood );
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameAudio::LoadSignalFilesFromXml( const char* xmlFilename, SAudioSignalsData& data  )
{
#if !defined(_RELEASE)
	float fStartTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
#endif //#if !defined(_RELEASE)

	XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile(xmlFilename);

	if (xmlNode==NULL)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CGameAudio::LoadSignalFilesFromXml: Failed to load'%s'", xmlFilename);
		return;
	}

	//Build a list of the xmlNodeRef and filenames from the file
	std::vector<SSignalFile> signalFiles;
	const uint32 filesCount = xmlNode->getChildCount();
	signalFiles.reserve(filesCount);

	for (uint32 fileIndex = 0; fileIndex < filesCount; ++fileIndex)
	{
		const XmlNodeRef currentFileNode = xmlNode->getChild(fileIndex);
		const char* signalsFilename = currentFileNode->getAttr("name");
		if(signalsFilename && signalsFilename[0])
		{
			XmlNodeRef xmlSignalsNode = GetISystem()->LoadXmlFromFile(signalsFilename);

			if (xmlSignalsNode==NULL)
			{
				CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CGameAudio::LoadSignalFilesFromXml: Failed to load'%s'", signalsFilename);
			}
			else
			{
				SSignalFile signalFile(xmlSignalsNode, signalsFilename);
				signalFiles.push_back(signalFile);
			}
		}
		else
		{
			XMLSIGNALS_ERROR(xmlFilename, currentFileNode, "expected name attribute for %s", xmlNode->getTag());
		}
	}

	const uint32 signalFilesCount = signalFiles.size();
	//Reserve space for all the signals and commands
	{
		size_t nTotalSignals = 0;
		size_t nTotalCommands = 0;
		for(uint32 signalFileIndex = 0; signalFileIndex < signalFilesCount; ++signalFileIndex)
		{
			CRY_ASSERT(signalFiles[signalFileIndex].nodeRef != 0);
			uint32 nSignals =  signalFiles[signalFileIndex].nodeRef->getChildCount();
			nTotalSignals += nSignals;

			const XmlNodeRef& xmlFileNode = signalFiles[signalFileIndex].nodeRef;

			for (uint32 signalIndex=0; signalIndex<nSignals; ++signalIndex)
			{
				const XmlNodeRef signalNode = xmlFileNode->getChild(signalIndex); 
				const uint32 signalCommandCount = signalNode->getChildCount();
				for (uint32 commandIndex=0; commandIndex<signalCommandCount; ++commandIndex)
				{
					const XmlNodeRef commandNode = signalNode->getChild(commandIndex);
					string lineTag = commandNode->getTag();
					if (lineTag != "Sound")
						++nTotalCommands;
				}
			}
		}
		data.audioSignals.reserve( nTotalSignals );
		data.commands.reserve( nTotalCommands );
	}

	//Load the signals
	for(uint32 signalFileIndex = 0; signalFileIndex < signalFilesCount; ++signalFileIndex)
	{
		CRY_ASSERT(signalFiles[signalFileIndex].nodeRef != 0);
		LoadSignalsFromXMLNode(signalFiles[signalFileIndex].filename, signalFiles[signalFileIndex].nodeRef, data );
	}

#if !defined(_RELEASE)
	CryLog("CGameAudio - Loaded %d audiosignal files, containing %" PRISIZE_T " audiosignals in %.4f seconds", signalFilesCount, data.audioSignals.size(), gEnv->pTimer->GetAsyncTime().GetSeconds() - fStartTime);
#endif
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::LoadSignalsFromXMLNode( const char* xmlFilename, const XmlNodeRef& xmlNode, SAudioSignalsData& data  )
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Audio Signals XML (%s)", xmlFilename);

	const uint32 signalCount = xmlNode->getChildCount();


	for (uint32 signalIndex = 0; signalIndex < signalCount; ++signalIndex)
	{
		const XmlNodeRef currentSignalNode = xmlNode->getChild(signalIndex); 
		
		CAudioSignal audioSignal;
		audioSignal.m_signalName = currentSignalNode->getAttr("signal");
		audioSignal.m_flags = CAudioSignal::GetSignalFlagsFromXML(currentSignalNode);
		
		const uint32 signalCommandCount = currentSignalNode->getChildCount();

		size_t nSounds = 0, nCommands = 0;
		for (uint32 commandIndex = 0; commandIndex < signalCommandCount; ++commandIndex)
		{
			const XmlNodeRef currentCommandNode = currentSignalNode->getChild(commandIndex);
			string lineTag = currentCommandNode->getTag();
			if (lineTag == "Sound")
				++nSounds;
			else
				++nCommands;
		}
		//audioSignal.m_sounds.reserve(nSounds);
		audioSignal.m_commands.reserve(nCommands);

		for (uint32 commandIndex = 0; commandIndex < signalCommandCount; ++commandIndex)
		{
			const XmlNodeRef currentCommandNode = currentSignalNode->getChild(commandIndex);

			string lineTag = currentCommandNode->getTag();
			
			bool tagRecognized = false;

			if (lineTag == "Sound")
			{
				//CSound newSound;
				//newSound.m_name = currentCommandNode->getAttr("name");
				//newSound.m_semantic = TranslateNameToSemantic( currentCommandNode->getAttr("semantic") );
				//newSound.m_flags = TranslateXMLToFlags(currentCommandNode);
				//audioSignal.m_sounds.push_back(newSound);
				//
				//EPrecacheResult result = ePrecacheResult_None;
				//
				//// Only always pre-cache if we're running the editor
				//if (gEnv->IsEditor())
				//	result = gEnv->pSoundSystem->Precache(newSound.m_name, 0u, FLAG_SOUND_PRECACHE_EVENT_DEFAULT);
				//
				//if(result == ePrecacheResult_Error)
				//{
				//	CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignal: '%s' failed to Precache the sound: '%s'", audioSignal.m_signalName.c_str(), newSound.m_name.c_str());
				//}
				tagRecognized = true;
			}
			else
			{
				CCommand* pCommand = CreateCommand( lineTag );
				if (pCommand)
				{
					tagRecognized = true;
					bool isOk = pCommand->Init( currentCommandNode, this );
					if (isOk)
					{
						data.commands.push_back( pCommand );
						audioSignal.m_commands.push_back( pCommand );
					}
					else
					{
						CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignal: '%s' failed to init a command. tag: '%s'", audioSignal.m_signalName.c_str(), lineTag.c_str());
						delete pCommand;
					}
				}
			}
			
			if (!tagRecognized)
			{
				XMLSIGNALS_ERROR(xmlFilename, xmlNode, "AudioSignal: '%s'   tag: '%s'  unknown", audioSignal.m_signalName.c_str(), lineTag.c_str());
			}
		}

		data.audioSignals.push_back(audioSignal);
		
		std::pair< TNameToSignalIndexMap::iterator, bool > val = data.nameToSignalIndexMap.insert( std::make_pair( audioSignal.m_signalName, data.audioSignals.size()-1 ) );
		if (!val.second)
		{
			XMLSIGNALS_ERROR(xmlFilename, xmlNode, "Duplicated signal: %s", audioSignal.m_signalName.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
uint32 CGameAudio::TranslateXMLToFlags(const XmlNodeRef& currentCommandNode)
{
	REINST("do we still need this?");
	uint32 flags = 0;//FLAG_SOUND_EVENT;

	const uint32 flagTranslationCount = 0;//ARRAY_COUNT(SoundFlagTranslationTable);
	for (uint32 index = 0; index < flagTranslationCount; index++)
	{
		//const SSoundFlagTranslationTableEntry& translationTableEntry = SoundFlagTranslationTable[index];

		//if (currentCommandNode->haveAttr(translationTableEntry.name))
		//{
		//	int enable = 0;
		//	currentCommandNode->getAttr(translationTableEntry.name, enable);
		//	if(enable)
		//	{
		//		flags |= translationTableEntry.flag;
		//	}
		//}
	}

	return flags;
}


//////////////////////////////////////////////////////////////////////////

//ESoundSemantic CGameAudio::TranslateNameToSemantic( const string& name )
//{
//	const uint32 semanticTranslationCount = ARRAY_COUNT(SoundSemanticTranslationTable);
//	for (uint32 index = 0; index < semanticTranslationCount; index++)
//	{
//		const SSoundSemanticTranslationTableEntry& translationTableEntry = SoundSemanticTranslationTable[index];
//
//		if (translationTableEntry.name == name)
//		{
//			return translationTableEntry.semantic;
//		}
//	}
//
//	return eSoundSemantic_None;
//}


//////////////////////////////////////////////////////////////////////////

const CGameAudio::CMood* CGameAudio::FindMoodByName( const string& name ) const
{
	for (uint32 moodIndex = 0; moodIndex < m_moodsLibrary.size(); ++moodIndex)
	{
		const CMood* currentMood = &m_moodsLibrary[moodIndex];
		if (currentMood->m_name == name)
		{
			return currentMood;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////

const CGameAudio::CAudioSignal* CGameAudio::GetAudioSignal( TAudioSignalID signalID )
{
	if (signalID<m_SPSignalsData.audioSignals.size())
	{
		return &m_SPSignalsData.audioSignals[signalID];
	}
	else 
	{
		uint32 MPIndex = signalID - m_SPSignalsData.audioSignals.size();
		if (MPIndex<m_MPSignalsData.audioSignals.size())
			return &m_MPSignalsData.audioSignals[MPIndex];
	} 

	return NULL;
}

void CGameAudio::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddContainer(m_moodsLibrary);	
	pSizer->AddContainer(m_activeMoods);	
	pSizer->AddContainer(m_SPSignalsData.audioSignals);	
	pSizer->AddContainer(m_SPSignalsData.commands);	
	pSizer->AddContainer(m_SPSignalsData.nameToSignalIndexMap);	
	pSizer->AddContainer(m_MPSignalsData.audioSignals);	
	pSizer->AddContainer(m_MPSignalsData.commands);	
	pSizer->AddContainer(m_MPSignalsData.nameToSignalIndexMap);	
}


//////////////////////////////////////////////////////////////////////////

CGameAudio::CCommand* CGameAudio::CreateCommand( const char* pCommandName )
{
	ECommand commandType = eCM_NoCommand;
	CCommand* pCommand = NULL;

	for (int i=0; i<eCM_NumCommands && commandType==eCM_NoCommand; ++i)
	{
		if (strcmpi( CommandNamesTranslataionTable[i].name, pCommandName )==0)
			commandType = CommandNamesTranslataionTable[i].command;
	}

	switch (commandType)
	{
		case eCM_Mood_Set:
		case eCM_Mood_Add:
		case eCM_Mood_Remove:
			pCommand = new CMoodCommand; break;
		case eCM_Music_Serialize: pCommand = new CMusicSerialize; break;
		case eCM_Music_EndTheme: pCommand = new CMusicEndTheme; break;
		case eCM_Music_Theme: pCommand = new CMusicTheme; break;
		case eCM_Music_Mood: pCommand = new CMusicMood; break;
		case eCM_Music_DefaultMood: pCommand = new CMusicDefaultMood; break;
		case eCM_Cache_Audio: pCommand = new CCacheAudio; break;
		case eCM_Music_Pattern: pCommand = new CMusicPattern; break;
	}

	if (pCommand)
		pCommand->m_command = commandType;

	return pCommand;	
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMoodCommand::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const CMood* pMood = pGameAudio->FindMoodByName( commandNode->getAttr("name") );
	if (!pMood)
		return false;

	m_pMood = pMood;
	return true;
}

void CGameAudio::CMoodCommand::EngineAddSoundMood( const CMood* pMood ) const
{
	/*ISoundMoodManager* moodManager = gEnv->pSoundSystem->GetIMoodManager();
	moodManager->RegisterSoundMood( pMood->m_name.c_str() );
	moodManager->UpdateSoundMood( pMood->m_name.c_str(), 1.0f, pMood->m_fadeInTimeMSecs );*/
}

void CGameAudio::CMoodCommand::EngineRemoveSoundMood( const CMood* pMood ) const
{
	/*ISoundMoodManager* moodManager = gEnv->pSoundSystem->GetIMoodManager();
	moodManager->UpdateSoundMood( pMood->m_name.c_str(), 0.0f, pMood->m_fadeOutTimeMSecs );*/
}


void CGameAudio::CMoodCommand::Execute() const
{
	if (CGameAudio* pGameAudio = g_pGame->GetGameAudio())
	{
		switch( m_command )
		{
			case eCM_Mood_Add:
				EngineAddSoundMood( m_pMood );
				stl::push_back_unique( pGameAudio->m_activeMoods, m_pMood );
				break;

			case eCM_Mood_Remove:
				EngineRemoveSoundMood( m_pMood );
				stl::find_and_erase( pGameAudio->m_activeMoods, m_pMood);
				break;

			case eCM_Mood_Set:
			{
				for (uint32 moodIndex=0; moodIndex < pGameAudio->m_activeMoods.size(); ++moodIndex)
				{
					EngineRemoveSoundMood( pGameAudio->m_activeMoods[moodIndex] );
				}

				pGameAudio->m_activeMoods.clear();
				pGameAudio->m_activeMoods.push_back( m_pMood );

				EngineAddSoundMood( m_pMood );
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicSerialize::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pActionName = NULL;
	bool success = commandNode->getAttr("saveload", &pActionName);
	if (!success || !pActionName )
		return false;

	if (strcmpi(pActionName, "load")==0)
		m_save = false;
	else 
		if (strcmpi(pActionName, "save")==0)
			m_save = true;
		else
			return false;
			
	const char* pSaveIDString = NULL;
	success = commandNode->getAttr("saveID", &pSaveIDString);
	if (!success || !pSaveIDString )
		return false;
	m_saveID = CryStringUtils::CalculateHash( pSaveIDString );

	return true;
}

void CGameAudio::CMusicSerialize::Execute() const
{
	if (m_save)
	{
		g_pGame->GetGameAudio()->m_MusicSaveLoadIDStack.push_back( m_saveID );
		gEnv->pMusicSystem->Serialize( m_save );
	}
	else // loading 
	{
		if (g_pGame->GetGameAudio()->m_MusicSaveLoadIDStack.size()>0)
		{
			uint32 lastSaveID = g_pGame->GetGameAudio()->m_MusicSaveLoadIDStack.back();
			if (lastSaveID==m_saveID)
			{
				g_pGame->GetGameAudio()->m_MusicSaveLoadIDStack.pop_back();
				gEnv->pMusicSystem->Serialize( m_save );
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicTheme::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pMusicThemeName = NULL;
	bool success = commandNode->getAttr("themeName", &pMusicThemeName);
	if (!success || !pMusicThemeName)
		return false;

	const char *pStartMoodName = NULL;
	success = commandNode->getAttr("startMood", &pStartMoodName);
	if (success)
	{
		m_startMood = pStartMoodName;
	}

	m_theme = pMusicThemeName;
	commandNode->getAttr( "force", m_force );
	commandNode->getAttr( "keepmood", m_keepMood );
	return true;
}

void CGameAudio::CMusicTheme::Execute() const
{
	if (m_startMood.length() > 0)
	{
		gEnv->pMusicSystem->SetTheme( m_theme, m_force, m_keepMood, -1, m_startMood.c_str());
	}
	else
	{
		gEnv->pMusicSystem->SetTheme( m_theme, m_force, m_keepMood);
	}
}


//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicMood::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pMoodName = NULL;
	bool success = commandNode->getAttr("moodName", &pMoodName);
	if (!success || !pMoodName)
		return false;

	m_mood = pMoodName;
	commandNode->getAttr( "playFromStart", m_playFromStart );
	commandNode->getAttr( "force", m_force );
	commandNode->getAttr( "playGameModeThemeBeforeMood", m_playGameModeThemeBeforeMood ); 
	return true;
}

void CGameAudio::CMusicMood::Execute() const
{
	if (gEnv->bMultiplayer && m_playGameModeThemeBeforeMood)
	{
		int curGameMode=eGM_INVALID_GAMEMODE;

		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules)
		{
			curGameMode = pGameRules->GetGameMode();
		}
		else
		{
			CGameLobby *pGameLobby = g_pGame->GetGameLobby();

			if(pGameLobby)
			{
				const char* currentGameModeName = pGameLobby->GetLoadingGameModeName(); 

				CryLog("CGameAudio::CMusicMood::Execute() found loading gamemode name from lobby is %s", currentGameModeName ? currentGameModeName : "NULL");

				if (currentGameModeName)
				{
					const bool modeOk = AutoEnum_GetEnumValFromString(currentGameModeName, CGameRules::S_GetGameModeNamesArray(), eGM_NUM_GAMEMODES, &curGameMode);

					CRY_ASSERT_MESSAGE(modeOk, string().Format("failed to find gamemode enum value for %s", currentGameModeName));
				}
			}
			else
			{
				CryLog("CGameAudio::CMusicMood::Execute() ERROR - has m_playGameModeThemeBeforeMood but we don't have a valid gamerules!");
			}
		}

		if (curGameMode != eGM_INVALID_GAMEMODE)
		{
			CRY_ASSERT(curGameMode >= 0);
			CRY_ASSERT(curGameMode < ARRAY_COUNT(MPGameRulesThemes));
			const char *gameModeTheme = MPGameRulesThemes[curGameMode].themeName;
			
			CryLog("CGameAudio::CMusicMood::Execute() has found a valid gamemode of %d (%s) who's theme is %s. Setting it with our mood %s as the startingMood", curGameMode, CGameRules::S_GetGameModeNamesArray()[curGameMode], gameModeTheme, m_mood.c_str());

			gEnv->pMusicSystem->SetTheme( gameModeTheme, m_force, false, -1, m_mood.c_str());
		}
	}
	else
	{
		gEnv->pMusicSystem->SetMood( m_mood, m_playFromStart, m_force );
	}
}


//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicDefaultMood::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pMoodName = NULL;
	bool success = commandNode->getAttr("moodName", &pMoodName);
	if (!success || !pMoodName)
		return false;

	m_mood = pMoodName;
	return true;
}

void CGameAudio::CMusicDefaultMood::Execute() const
{
	gEnv->pMusicSystem->SetDefaultMood( m_mood );
}


bool CGameAudio::CCacheAudio::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pGameHintName = NULL;
	bool success = commandNode->getAttr("gameHint", &pGameHintName);
	if (!success || !pGameHintName)
		return false;

	success = commandNode->getAttr("cache", m_bCache );
	if (!success)
	{
		return false;
	}

	commandNode->getAttr("now", m_bNow);

	m_gameHint = pGameHintName;
	return true;
}

void CGameAudio::CCacheAudio::Execute() const
{
	/*if (m_bCache)
	{
		CryLog("CGameAudio::CCacheAudio::Execute() is caching audioFile GameHint=%s", m_gameHint.c_str());
		gEnv->pSoundSystem->CacheAudioFile(m_gameHint.c_str(), eAFCT_GAME_HINT);
	}
	else
	{
		CryLog("CGameAudio::CCacheAudio::Execute() is uncaching audioFile GameHint=%s bNow=%d", m_gameHint.c_str(), m_bNow);
		gEnv->pSoundSystem->RemoveCachedAudioFile(m_gameHint.c_str(), m_bNow);
	}*/
}


//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicPattern::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pPatternName = NULL;
	bool success = commandNode->getAttr("patternName", &pPatternName);
	if (!success || !pPatternName)
		return false;

	m_pattern = pPatternName;
	commandNode->getAttr( "stopPrevious", m_stopPrevious );
	commandNode->getAttr( "synced", m_synced );
	commandNode->getAttr( "ontop", m_ontop );
	return true;
}

void CGameAudio::CMusicPattern::Execute() const
{
	gEnv->pMusicSystem->PlayPattern( m_pattern, m_stopPrevious, m_synced, m_ontop );
}


//////////////////////////////////////////////////////////////////////////

bool CGameAudio::CMusicEndTheme::Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio )
{
	const char* pFadeTypeName = NULL;
	bool success = commandNode->getAttr("fadeType", &pFadeTypeName);
	if (!success || !pFadeTypeName)
		return false;

	// translate the string into the enum value
	const uint32 size = ARRAY_COUNT(ThemeFadeTypeTranslataionTable);
	bool found = false;
	for (uint32 i = 0; i<size; ++i)
	{
		if (strcmpi( ThemeFadeTypeTranslataionTable[i].name, pFadeTypeName )==0)
		{
			m_fadeType = ThemeFadeTypeTranslataionTable[i].fadeType;
			found = true;
			break;
		}
	}

	return found;
}

void CGameAudio::CMusicEndTheme::Execute() const
{
	gEnv->pMusicSystem->EndTheme( m_fadeType, 0, true );
}


//////////////////////////////////////////////////////////////////////////
uint32 CGameAudio::CAudioSignal::GetSignalFlagsFromXML(const XmlNodeRef& currentCommandNode)
{
	uint32 flags = eF_None;

	const uint32 flagCount = ARRAY_COUNT(SoundFlagTable);
	for (uint32 index = 0; index < flagCount; index++)
	{
		const SFlagTableEntry& tableEntry = SoundFlagTable[index];

		if (currentCommandNode->haveAttr(tableEntry.name))
		{
			int enable = 0;
			currentCommandNode->getAttr(tableEntry.name, enable);
			if(enable)
			{
				flags |= tableEntry.flag;
			}
		}
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////

#if !defined(_RELEASE)
void CGameAudio::CmdPlaySignal(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArgCount() == 2)
	{
		TAudioSignalID signalId = g_pGame->GetGameAudio()->GetSignalID(pCmdArgs->GetArg(1), true);
		CAudioSignalPlayer::JustPlay(signalId);
	}
	else
	{
		CryLogAlways("Usage - PlaySignal <signal name>");
	}
}
#endif

#if !defined(_RELEASE)
void CGameAudio::CmdPlaySignalOnEntity(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArgCount() == 3 || pCmdArgs->GetArgCount() == 5)
	{
		const char* pEntityName = pCmdArgs->GetArg(2);
		TAudioSignalID signalId = g_pGame->GetGameAudio()->GetSignalID(pCmdArgs->GetArg(1), true);
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(pEntityName);
		if(pEntity)
		{
			s_debugPlaySignalOnEntitySignal.SetSignal(signalId);

			const char* paramName = NULL;
			float paramValue = 0.0f;

			if(pCmdArgs->GetArgCount() == 5)
			{
				paramName = pCmdArgs->GetArg(3);
				paramValue = (float) atof(pCmdArgs->GetArg(4));
			}

			//s_debugPlaySignalOnEntitySignal.Play(pEntity->GetId(), paramName, paramValue);
		}
		else
		{
			CryLogAlways("Unable to find entity '%s'", pEntityName);
		}
	}
	else
	{
		CryLogAlways("Usage - PlaySignalOnEntity <signal name> <entity name> [param name] [param value]");
	}
}
#endif

#if !defined(_RELEASE)
void CGameAudio::CmdStopSignalsOnEntity(IConsoleCmdArgs* pCmdArgs)
{
	if (pCmdArgs->GetArgCount() == 2)
	{
		const char* pEntityName = pCmdArgs->GetArg(1);
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(pEntityName);
		if(pEntity)
		{
			//s_debugPlaySignalOnEntitySignal.Stop(pEntity->GetId());
		}
		else
		{
			CryLogAlways("Unable to find entity '%s'", pEntityName);
		}
	}
	else
	{
		CryLogAlways("Usage - StopSignalsOnEntity <entity name>");
	}
}
#endif

#if !defined(_RELEASE)
void CGameAudio::CmdPlayAllSignals(IConsoleCmdArgs* pCmdArgs)
{
	const int argCount = pCmdArgs->GetArgCount();
	if(argCount > 0 && argCount <= 2)
	{
		int count = s_signalAutoComplete.GetCount();

		if(pCmdArgs->GetArgCount() == 2)
		{
			count= atoi(pCmdArgs->GetArg(1));
		}
		for(int i = 0; i < count; i++)
		{
			TAudioSignalID signalId = g_pGame->GetGameAudio()->GetSignalID(s_signalAutoComplete.GetValue(i), true);
			CAudioSignalPlayer::JustPlay(signalId);
		}
	}
	else
	{
		CryLogAlways("Usage - PlayAllSignals <optional number of signals to play>");
	}
}
#endif

#if !defined(_RELEASE)
void CGameAudio::CmdCacheAudioFile(IConsoleCmdArgs* pCmdArgs)
{
	const int argCount = pCmdArgs->GetArgCount();
	if(argCount == 2)
	{
		//gEnv->pSoundSystem->CacheAudioFile(pCmdArgs->GetArg(1), eAFCT_GAME_HINT);
	}
	else
	{
		CryLogAlways("Usage CacheAudioFile <gamehint>");
	}
}
#endif

#if !defined(_RELEASE)
void CGameAudio::CmdRemoveCacheAudioFile(IConsoleCmdArgs* pCmdArgs)
{
	const int argCount = pCmdArgs->GetArgCount();
	if(argCount == 2)
	{
		//gEnv->pSoundSystem->RemoveCachedAudioFile(pCmdArgs->GetArg(1), false);
	}
	else
	{
		CryLogAlways("Usage UnCacheAudioFile <gamehint>");
	}
}
#endif
