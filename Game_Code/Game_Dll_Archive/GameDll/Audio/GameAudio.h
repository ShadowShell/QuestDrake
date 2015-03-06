/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
*************************************************************************/

#pragma once

#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <IMusicSystem.h>

typedef uint32 TAudioSignalID; // internally, this is now just the index into the audioSignals vector for SP ones, and 'index+amount_SP_signals' for MP ones.
const TAudioSignalID INVALID_AUDIOSIGNAL_ID = -1;
class CScriptbind_GameAudio;
class CGameAudioUtils;

#if !defined(_RELEASE)
struct SSignalAutoComplete;
#endif

class CGameAudio
{
	friend class CAudioSignalPlayer;
#if !defined(_RELEASE)
	friend struct SSignalAutoComplete;
#endif
public:
	CGameAudio();
	~CGameAudio();
	
	SC_API TAudioSignalID GetSignalID (const char* pSignalName, bool outputWarning = true);
	void Reset();
	void ClearSoundMoods();

	void GetMemoryUsage( ICrySizer *pSizer ) const;

	XmlNodeRef LoadMusicXml( const char* filename );
	void LoadMPAudioSignals();
	void UnloadMPAudioSignals();

	CGameAudioUtils* GetUtils() { return m_pUtils; }

#if !defined(_RELEASE)
	static void CmdPlaySignal(IConsoleCmdArgs* pCmdArgs);
	static void CmdPlaySignalOnEntity(IConsoleCmdArgs* pCmdArgs);
	static void CmdStopSignalsOnEntity(IConsoleCmdArgs* pCmdArgs);
	static void CmdPlayAllSignals(IConsoleCmdArgs* pCmdArgs);

	static void CmdCacheAudioFile(IConsoleCmdArgs* pCmdArgs);
	static void CmdRemoveCacheAudioFile(IConsoleCmdArgs* pCmdArgs);
#endif

private:

	//.............................
	class CMood
	{
	public:
		CMood()
			: m_fadeInTimeMSecs(0)
			, m_fadeOutTimeMSecs(0)
		{}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_name);
		}
		string m_name;
		uint32 m_fadeInTimeMSecs;
		uint32 m_fadeOutTimeMSecs;
	};



	enum ECommand
	{
		eCM_Mood_Add,
		eCM_Mood_Remove,
		eCM_Mood_Set,
		eCM_Music_Serialize,
		eCM_Music_EndTheme,
		eCM_Music_Theme,
		eCM_Music_Mood,
		eCM_Music_DefaultMood,
		eCM_Cache_Audio,
		eCM_Music_Pattern,
		
		eCM_NumCommands,
		eCM_NoCommand = -1
	};

	class CCommand
	{
	public:
	
		CCommand() : m_command( eCM_NoCommand ) {}
		virtual ~CCommand() {}

		virtual void Execute() const = 0;
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio ) = 0;
		bool IsACommand() const { return m_command!=eCM_NoCommand; }
		
		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
		}
		
		ECommand m_command;
	};

	
	//.................................
	class CMoodCommand : public CCommand
	{
	public:

		CMoodCommand() 
			: m_pMood( 0 )
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:	
		void EngineAddSoundMood( const CMood* pMood ) const;
		void EngineRemoveSoundMood( const CMood* pMood ) const;

	public:	
		const CMood* m_pMood;
	};
	

	//.................................
	class CMusicSerialize : public CCommand
	{
	public:

		CMusicSerialize() 
		: m_save( false )
		, m_saveID( 0 )
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:
		bool m_save; // false = load
		uint32 m_saveID;  // used to make sure that loads match the saves: a 'load' only can actually load if the last ID in m_MusicSaveLoadStack is the one specified here.
	};
	

	//.................................
	class CMusicEndTheme : public CCommand
	{
	public:

		CMusicEndTheme()
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:		
		EThemeFadeType m_fadeType;
	};
	

	//.................................
	class CMusicTheme : public CCommand
	{
	public:

		CMusicTheme()
		: m_force( false )
		, m_keepMood( false )
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;
		
	private:
		string m_theme;
		string m_startMood;
		bool m_force;
		bool m_keepMood;
	};
	

	//.................................
	class CMusicMood : public CCommand
	{
	public:

		CMusicMood()
			: m_force( true )
			, m_playFromStart( true )
			, m_playGameModeThemeBeforeMood( false )
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:
		string m_mood;
		bool m_playFromStart;
		bool m_force;
		bool m_playGameModeThemeBeforeMood;
	};
	
	//.................................
	class CMusicDefaultMood : public CCommand
	{
	public:

		CMusicDefaultMood()
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:
		string m_mood;
	};
	
	//.................................
	class CCacheAudio : public CCommand
	{
	public:
		CCacheAudio() 
			: m_bCache(false)
			, m_bNow(false)
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:
		string	m_gameHint;
		bool		m_bCache;
		bool		m_bNow;
	};


	//.................................
	class CMusicPattern : public CCommand
	{
	public:

		CMusicPattern()
			: m_stopPrevious( false )
			, m_synced( false )
			, m_ontop( false )
		{}
		virtual bool Init( const XmlNodeRef& commandNode, CGameAudio* pGameAudio );
		virtual void Execute() const;

	private:
		string m_pattern;
		bool m_stopPrevious;
		bool m_synced;
		bool m_ontop;
	};


	//..................................
	/*class CSound
	{
	public:
		CSound()
			: m_semantic( eSoundSemantic_None ),
			m_flags(FLAG_SOUND_EVENT)
		{}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_name);
		}

		string m_name;
		ESoundSemantic m_semantic;
		uint32 m_flags;
	};*/

	//..................................
	class CAudioSignal
	{
	public:
		enum EFlags
		{
			eF_None = 0,
			eF_PlayRandom = BIT(0),
		};

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_signalName);
			//pSizer->AddContainer(m_sounds);			
			pSizer->AddContainer(m_commands);
		}
		uint32 static GetSignalFlagsFromXML(const XmlNodeRef& currentCommandNode);

		uint32 m_flags;
		string m_signalName;
		//std::vector<CSound> m_sounds;
		std::vector<const CCommand*> m_commands;

		struct SFlagTableEntry
		{
			const char* name;
			const uint32 flag;
		};
		static const SFlagTableEntry SoundFlagTable[];

	};

	//..................................
	struct SSignalFile
	{
		SSignalFile(XmlNodeRef _nodeRef, const char* _filename)
			: nodeRef(_nodeRef)
			, filename(_filename)
		{ }
		XmlNodeRef nodeRef;
		const char* filename;
	};

private:

	struct SAudioSignalsData;

	void LoadMoodsFromXML( const char* xmlFilename );
	void LoadSignalFilesFromXml( const char* xmlFilename, SAudioSignalsData& data );
	void LoadSignalsFromXMLNode( const char* xmlFilename, const XmlNodeRef& xmlNode, SAudioSignalsData& data );
	const CMood* FindMoodByName(const string& name) const;
	//ESoundSemantic TranslateNameToSemantic( const string& name );
	uint32 TranslateXMLToFlags(const XmlNodeRef& currentCommandNode);
	const CAudioSignal* GetAudioSignal( TAudioSignalID signalID );
	static CCommand* CreateCommand( const char* pCommand );
	

private:

	std::vector<CMood> m_moodsLibrary;
	std::vector<const CMood*> m_activeMoods;
	std::vector<TAudioSignalID> m_MusicSaveLoadIDStack;  // used to make sure that music save/loads match.

	typedef std::vector<CAudioSignal> TAudioSignalsVector;
	typedef std::vector<CCommand*> TCommandsVector;
	typedef std::map<string, uint32> TNameToSignalIndexMap;

	struct SAudioSignalsData
	{
		~SAudioSignalsData() { Unload(); }
		void Unload();
		TCommandsVector commands;
		TNameToSignalIndexMap nameToSignalIndexMap;  // just to optimize searchs into audioSignals. the stored numeric value is the index into 'audioSignals', which is NOT (or not always) the ID of the signal
		TAudioSignalsVector audioSignals;
	};

	SAudioSignalsData m_SPSignalsData;
	SAudioSignalsData m_MPSignalsData;
	bool	m_MPAudioSignalsLoaded;

	struct SSoundSemanticTranslationTableEntry
	{
		const char* name;
		//const ESoundSemantic semantic;
	};
	static const SSoundSemanticTranslationTableEntry SoundSemanticTranslationTable[];
	
	struct SSoundFlagTranslationTableEntry
	{
		const char* name;
		const uint32 flag;
	};
	//static const SSoundFlagTranslationTableEntry SoundFlagTranslationTable[];

	struct SCommandNameTranslationTableEntry
	{
		const char* name;
		ECommand command;
	};
	static const SCommandNameTranslationTableEntry CommandNamesTranslataionTable[eCM_NumCommands];

	struct SThemeFadeTypeTranslationTableEntry
	{
		const char* name;
		EThemeFadeType fadeType;
	};
	static const SThemeFadeTypeTranslationTableEntry ThemeFadeTypeTranslataionTable[EThemeFade_MAX];

	struct SMPGameRulesThemeEntry
	{
		uint32 gameRules;
		const char *themeName;
	};
	static const SMPGameRulesThemeEntry MPGameRulesThemes[];

	CScriptbind_GameAudio* m_pScriptbind;
	CGameAudioUtils* m_pUtils;
};


#endif
