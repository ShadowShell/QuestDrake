// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef AUDIO_LOGGER_H_INCLUDED
#define AUDIO_LOGGER_H_INCLUDED

#include <platform.h>
#include <ITimer.h>
#include <IConsole.h>

enum EAudioLogType
{
	eALT_NONE			= 0,
	eALT_COMMENT	= 1,
	eALT_WARNING	= 2,
	eALT_ERROR		= 3,
	eALT_ALWAYS		= 4,
};

enum EAudioLoggingOptions
{
	eALO_NONE     = 0,
	eALO_ERRORS   = BIT(6),  // a
	eALO_WARNINGS = BIT(7),  // b
	eALO_COMMENTS = BIT(8),  // c
};

//////////////////////////////////////////////////////////////////////////
// <title CAudioLogger>
// Summary:
//			A simpler logger wrapper that adds and audio tag and a timestamp
//////////////////////////////////////////////////////////////////////////
class CAudioLogger 
{
public:

	//DOC-IGNORE-BEGIN	
	CAudioLogger() {}
	virtual ~CAudioLogger(){}
	//DOC-IGNORE-END

	//////////////////////////////////////////////////////////////////////////
	// <title Log>
	// Summary:
	//      Log a message
	// Arguments:
	//      eType					- log message type (eALT_COMMENT, eALT_WARNING, eALT_ERROR or eALT_ALWAYS)
	//      sFormat, ...	- printf-style format string and its arguments
	//////////////////////////////////////////////////////////////////////////
	void Log(EAudioLogType const eType, char const* const sFormat, ...) const
	{
#if defined(ENABLE_AUDIO_LOGGING)
		if (sFormat && sFormat[0] && gEnv->pLog->GetVerbosityLevel() != -1)
		{
			FRAME_PROFILER("CAudioLogger::Log", GetISystem(), PROFILE_AUDIO);

			char sBuffer[256];
			va_list	ArgList;
			va_start(ArgList, sFormat);
			vsnprintf_s(sBuffer, sizeof(sBuffer), sizeof(sBuffer) - 1, sFormat, ArgList);
			sBuffer[sizeof(sBuffer)-1] = '\0';
			va_end(ArgList);

			float fCurrTime = gEnv->pTimer->GetAsyncCurTime();

			EAudioLoggingOptions audioLoggingOptions = (EAudioLoggingOptions)gEnv->pConsole->GetCVar("s_AudioLoggingOptions")->GetIVal();

			switch (eType)
			{
			case eALT_WARNING:
				{
					if( audioLoggingOptions & eALO_WARNINGS )
						gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, NPTR, "<Audio> <%.3f>: %s", fCurrTime, sBuffer);

					break;
				}
			case eALT_ERROR:
				{
					if( audioLoggingOptions & eALO_ERRORS )
						gEnv->pSystem->Warning(VALIDATOR_MODULE_AUDIO, VALIDATOR_ERROR, VALIDATOR_FLAG_AUDIO, NPTR, "<Audio> <%.3f>: %s", fCurrTime, sBuffer);

					break;
				}
			case eALT_COMMENT: 
				{
					if ((gEnv->pLog != NPTR) && (gEnv->pLog->GetVerbosityLevel() >= 4) && (audioLoggingOptions & eALO_COMMENTS))
					{
						CryLogAlways("<Audio> <%.3f>: %s", fCurrTime, sBuffer);
					}

					break;
				}
			case eALT_ALWAYS:
				{
					CryLogAlways("<Audio> <%.3f>: %s", fCurrTime, sBuffer);

					break;
				}
			default:
				{
					assert(0);

					break;
				}
			}
		}
#endif // ENABLE_AUDIO_LOGGING
	}

private:

	CAudioLogger(CAudioLogger const&);						// Copy protection
	CAudioLogger& operator=(CAudioLogger const&);	// Copy protection
};

#endif // AUDIO_LOGGER_H_INCLUDED
