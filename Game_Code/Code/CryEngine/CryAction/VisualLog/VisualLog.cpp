/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2007.
---------------------------------------------------------------------
File name:   VisualLog.h
$Id$
$DateTime$
Description: Logs images of game and related logging text, to be viewed
in the Editor Visual Log Viewer
---------------------------------------------------------------------
History:
- 10:09:2007 : Created by Adam Rutkowski

*********************************************************************/
#include "StdAfx.h"
#include "VisualLog.h"

CVisualLog* CVisualLog::  m_pInstance = NULL;

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CVisualLog::CVisualLog() : m_bInit( false ), m_fLogFile( NULL ), m_fLogParamsFile( NULL )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CVisualLog::~CVisualLog()
{
}

// Description:
//   Create
// Arguments:
//
// Return:
//
CVisualLog& CVisualLog::ref()
{
	if ( NULL == m_pInstance )
	{
		Create();
	}
	CRY_ASSERT(m_pInstance);
	PREFAST_ASSUME(m_pInstance);

	return( *m_pInstance );
}

// Description:
//   Create
// Arguments:
//
// Return:
//
bool CVisualLog::Create()
{
	if( NULL == m_pInstance )
	{
		m_pInstance = new CVisualLog();
	}
	else
	{
		CRY_ASSERT( "Trying to Create() the singleton more than once" );
	}

	return( m_pInstance != NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Init()
{
	if( m_bInit == false )
	{
		InitCVars();
		Reset();

		m_bInit = true;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Shutdown()
{
	if( m_bInit == true )
	{
		Reset();
		SAFE_DELETE( m_pInstance );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Reset()
{
	CloseLogs();
	m_bLoggingEnabled = false;
	m_defaultParams = SVisualLogParams();
	m_iFrameId = 0;
	m_eFormat = EVLF_NONE;
	m_sFormat.clear();
	m_sLogBuffer.clear();
	m_sLogParamsBuffer.clear();
	m_sLogFolder.clear();
	m_sLogPath.clear();
	m_iLogFolderNum = 0;
	m_iLogFrameNum = 0;

	m_pCVVisualLog->Set( 0 ); // Turn off logging
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Update()
{
	if( m_bInit == false )
	{
		GameWarning( "[VisualLog] Update called before Visual Log has been initialized" );
		return;
	}

	// Check if we already called update this frame
	if ( m_iFrameId == gEnv->pRenderer->GetFrameID() )
	{
		return; 
	}
	m_iFrameId = gEnv->pRenderer->GetFrameID();

	bool bDoLogging = m_pCVVisualLog->GetIVal() != 0;
	if ( !bDoLogging && !m_bLoggingEnabled )
	{
		// Logging is off, so make this the fastest path
		return;
	}

	if ( bDoLogging && !m_bLoggingEnabled )
	{
		// Start of logging run, open log files before doing an update
		m_bLoggingEnabled = OpenLogs();
		m_iLogFrameNum = -1; // Set this so number is incremented to correct value on first update
	}

	if ( m_bLoggingEnabled )
	{
		if ( bDoLogging )
		{
			// Regular logging frame update
			m_bLoggingEnabled = UpdateLogs();
		}
		else 
		{
			// End of logging run
			m_bLoggingEnabled = false;
		}
	}

	if ( m_bLoggingEnabled )
	{
		// tell renderer to capture a frame
		m_pCV_capture_frames->Set( 1 ); 
		m_pCV_capture_frame_once->Set( 1 );

		// display onscreen message
		ShowDisplayMessage();
	}
	else
	{
		// Logging just ended, or an error occurred
		Reset();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CVisualLog::OpenLogs()
{
	m_sFormat = m_pCVVisualLogImageFormat->GetString();
	m_eFormat = GetFormatType( m_sFormat );
	m_sLogFolder = m_pCVVisualLogFolder->GetString();
	int iLogFolderLen = m_sLogFolder.length();

	// Check we have good params to use
	if ( m_eFormat == EVLF_NONE || iLogFolderLen == 0 )
	{
		GameWarning( "[VisualLog] File format or log folder value invalid" );
		return false;
	}

	// Create base directory if necessary
	CryCreateDirectory( m_sLogFolder, 0 );

	// Figure out next number in sequence m_sLogFolderName/m_sLogFolderNameXXXX, where XXXX is 0000, 0001, etc.
	int iSeqNum = 0;
	__finddata64_t fd;
	intptr_t handle = _findfirst64( PathUtil::Make( m_sLogFolder , "*.*" ), &fd );
	if ( handle != -1 )
	{
		do 
		{
			// Is it a directory with our base name as a prefix?
			if ( fd.attrib & _A_SUBDIR && fd.name[0]!='.' && 
					 !_strnicmp( m_sLogFolder, fd.name, iLogFolderLen ) )
			{
				iSeqNum = max( iSeqNum, atoi( fd.name + iLogFolderLen ) + 1 );
			}
		}
		while (0 == _findnext64 (handle, &fd));
		_findclose(handle);
	}

	// Now create directory
	char sLogPath[256];
	_snprintf( sLogPath, sizeof(sLogPath), "%s\\%s%04d", m_sLogFolder.c_str(), m_sLogFolder.c_str(), iSeqNum );
	if ( 0 == CryCreateDirectory( sLogPath, 0 ) )
	{
		GameWarning( "[VisualLog] Unable to create directory for log files: %s", sLogPath );
		return false;
	}

	m_sLogPath = sLogPath;
	m_iLogFolderNum = iSeqNum;
	
	char sLogFileName[256];
	_snprintf( sLogFileName, sizeof(sLogFileName), "%s\\%s%04d.log", m_sLogPath.c_str(), m_sLogFolder.c_str(), m_iLogFolderNum );	
	char sLogParamsFileName[256];
	_snprintf( sLogParamsFileName, sizeof(sLogParamsFileName), "%s\\%s%04d_params.log", m_sLogPath.c_str(), m_sLogFolder.c_str(), m_iLogFolderNum );	

	// Open Log Files
	m_fLogFile = fxopen(sLogFileName, "w");
	m_fLogParamsFile = fxopen(sLogParamsFileName, "w");
	if ( !m_fLogFile || !m_fLogParamsFile )
	{
		GameWarning( "[VisualLog] Unable to open log files [%s] [%s]", sLogFileName, sLogParamsFileName );
		CloseLogs();
		return false;
	}

	WriteFileHeaders( sLogFileName, sLogParamsFileName );

	return true;
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::CloseLogs()
{
	if ( m_fLogFile )
	{
		fclose( m_fLogFile );
		m_fLogFile = NULL;
	}

	if ( m_fLogParamsFile )
	{
		fclose( m_fLogParamsFile );
		m_fLogParamsFile = NULL;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CVisualLog::UpdateLogs()
{
	if( m_bInit == false )
	{
		GameWarning( "[VisualLog] Update called before Visual Log has been initialized" );
		return false;
	}

	// Don't do any updating if the last frame wasn't captured for some reason
	if ( m_pCV_capture_frames->GetIVal() != 0 )
	{
		return true;
	}

	// Do some checks to make sure logging parameters haven't been tampered with
	if ( stricmp( m_sFormat,  m_pCVVisualLogImageFormat->GetString() ) )
	{
		// Attempted image file format change during capture session, put it back
		m_pCVVisualLogImageFormat->Set( m_sFormat );
	}
	if ( stricmp( m_sLogFolder,  m_pCVVisualLogFolder->GetString() ) )
	{
		// Attempted log folder change during capture session, put it back
		m_pCVVisualLogFolder->Set( m_sLogFolder );
	}
		
	m_iLogFrameNum++;
	
	char sImageFileName[256];
	_snprintf( sImageFileName, sizeof(sImageFileName), "Frame%06d.%s", m_iLogFrameNum, m_sFormat.c_str() );	
	
	char sImageFullFileName[512];
	_snprintf( sImageFullFileName, sizeof(sImageFullFileName), "%s\\%s", m_sLogPath.c_str(), sImageFileName );	
	m_pCV_capture_file_name->Set( sImageFullFileName );

	// Set image scale CVars
	float fScale = m_pCVVisualLogImageScale->GetFVal();
	if ( fScale != 1.f )
	{
		m_pCV_capture_use_scale->Set( 1 );
		m_pCV_capture_image_scale->Set( fScale );
	}

	// Write log for this frame
	WriteFrameHeaders( m_iLogFrameNum, sImageFileName );	
	fprintf( m_fLogFile, m_sLogBuffer.c_str() );
	fprintf( m_fLogParamsFile, m_sLogParamsBuffer.c_str() );
	fflush( m_fLogFile );
	fflush( m_fLogParamsFile );

	// Clear buffers now they have been written
	m_sLogBuffer.clear();
	m_sLogParamsBuffer.clear();

	return true;
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Log( const char* format, ... )
{
	if( m_bInit == false )
	{
		GameWarning( "[VisualLog] Update called before Visual Log has been initialized" );
		return;
	}
	if ( !m_bLoggingEnabled )
	{
		return;
	}

	va_list ArgList;
	char    szBuffer[MAX_WARNING_LENGTH];
	
	va_start( ArgList, format );
	vsprintf_s( szBuffer, format, ArgList );
	va_end( ArgList );

	// Add log to buffer
	m_sLogBuffer += szBuffer;
	m_sLogBuffer += "\n";

	// Add empty params string to params buffer
	m_sLogParamsBuffer += "[]\n";
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::Log( const SVisualLogParams& params, const char *format, ... )
{
	if ( !m_bLoggingEnabled )
	{
		return;
	}

	va_list ArgList;
	char    szBuffer[MAX_WARNING_LENGTH];
	char    szParamsBuffer[256];

	va_start( ArgList, format );
	vsprintf_s( szBuffer, format, ArgList );
	va_end( ArgList );

	// Add log to buffer
	m_sLogBuffer += szBuffer;
	m_sLogBuffer += "\n";


	// Add params string to params buffer, only including non-default params
	bool bComma = false;
	m_sLogParamsBuffer += "[";
	if ( params.color != m_defaultParams.color )
	{
		_snprintf( szParamsBuffer, sizeof(szParamsBuffer), "color(%0.3f,%0.3f,%0.3f,%0.3f)",
							 params.color.r, params.color.g, params.color.b, params.color.a );
		m_sLogParamsBuffer += szParamsBuffer;
		bComma = true;
	}
	if ( params.size != m_defaultParams.size )
	{
		_snprintf( szParamsBuffer, sizeof(szParamsBuffer), "%ssize(%0.3f)", 
							 bComma ? "," : "", max( 0.1f, params.size ) );
		m_sLogParamsBuffer += szParamsBuffer;
		bComma = true;
	}
	if ( params.column != m_defaultParams.column )
	{
		_snprintf( szParamsBuffer, sizeof(szParamsBuffer), "%scolumn(%d)", 
							 bComma ? "," : "", max( 1, params.column ) );
		m_sLogParamsBuffer += szParamsBuffer;
		bComma = true;
	}
	if ( params.alignColumnsToThis != m_defaultParams.alignColumnsToThis )
	{
		_snprintf( szParamsBuffer, sizeof(szParamsBuffer), "%salign_columns_to_this(%d)", 
							 bComma ? "," : "", params.alignColumnsToThis ? 1 : 0  );
		m_sLogParamsBuffer += szParamsBuffer;
		bComma = true;
	}
	m_sLogParamsBuffer += "]\n";
}

// Description:
//
// Arguments:
//
// Return:
//

void CVisualLog::WriteFileHeaders( const char *sLogFileName, const char *sLogParamsFileName )
{
	// Headers start the same for both the log file and log params file
	// Just a listing of both of these file names
	fprintf( m_fLogFile, "%s\n%s\n", sLogFileName, sLogParamsFileName );  
	fprintf( m_fLogParamsFile, "%s\n%s\n", sLogFileName, sLogParamsFileName );  

	// Params file now lists all of the default parameters
	fprintf( m_fLogParamsFile, "color(%0.3f,%0.3f,%0.3f,%0.3f)\nsize(%0.3f)\ncolumn(%d)\nalign_columns_to_this(%d)\n", 
		m_defaultParams.color.r, m_defaultParams.color.g, m_defaultParams.color.b, m_defaultParams.color.a,
		m_defaultParams.size,
		m_defaultParams.column,
		m_defaultParams.alignColumnsToThis ? 1 : 0 );
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::WriteFrameHeaders( int iFrameNum, const char *sImageName )
{
	// Frame header example:  Frame 0 [Frame000000.jpg] [34.527]
	CTimeValue time = gEnv->pTimer->GetFrameStartTime();

	fprintf( m_fLogFile, "Frame %d [%s] [%0.3f]\n", iFrameNum, sImageName, time.GetSeconds() );  
	fprintf( m_fLogParamsFile, "Frame %d [%s] [%0.3f]\n", iFrameNum, sImageName, time.GetSeconds() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CVisualLog::InitCVars()
{
	m_pCVVisualLog = REGISTER_INT( "cl_visualLog", 0, 0, "Enables Visual Logging." );
	m_pCVVisualLogFolder = REGISTER_STRING( "cl_visualLogFolder", "VisualLog", 0, "Specifies sub folder to write logs to." );
	m_pCVVisualLogImageFormat = REGISTER_STRING( "cl_visualLogImageFormat", "bmp", 0, "Specifies file format of captured files (jpg, bmp)." );
	m_pCVVisualLogImageScale = REGISTER_FLOAT( "cl_visualLogImageScale", 128, 0, "Image size. [0-1] = scale value. >1 = actual pixels for image width" );

	assert(gEnv->pConsole);
	PREFAST_ASSUME(gEnv->pConsole);
	m_pCV_capture_frames = gEnv->pConsole->GetCVar("capture_frames");
	m_pCV_capture_file_format = gEnv->pConsole->GetCVar("capture_file_format");
	m_pCV_capture_frame_once = gEnv->pConsole->GetCVar("capture_frame_once");
	m_pCV_capture_use_scale = gEnv->pConsole->GetCVar("capture_use_scale");
	m_pCV_capture_image_scale = gEnv->pConsole->GetCVar("capture_image_scale");
	m_pCV_capture_file_name = gEnv->pConsole->GetCVar("capture_file_name");

	CRY_ASSERT( m_pCV_capture_frames );
	CRY_ASSERT( m_pCV_capture_file_format );
	CRY_ASSERT( m_pCV_capture_frame_once );
	CRY_ASSERT( m_pCV_capture_use_scale );
	CRY_ASSERT( m_pCV_capture_image_scale );
	CRY_ASSERT( m_pCV_capture_file_name );
}

// Description:
//
// Arguments:
//
// Return:
//
CVisualLog::EVisualLogFormat CVisualLog::GetFormatType( const string& sFormat )
{
	EVisualLogFormat eFormat = EVLF_NONE;

	if ( !sFormat.compareNoCase( "jpg" ) )
	{
		eFormat = EVLF_JPG;
	}
	else if ( !sFormat.compareNoCase( "bmp" ) )
	{
		eFormat = EVLF_BMP;
	}

	return eFormat;
}

void CVisualLog::ShowDisplayMessage()
{
	float red[4] = {1,0,0,1};
	gEnv->pRenderer->Draw2dLabel( gEnv->pRenderer->GetWidth()*0.5f, gEnv->pRenderer->GetHeight()*0.9f, 2, red, true, "VISUAL LOGGING ENABLED" );
}

#include UNIQUE_VIRTUAL_WRAPPER(IVisualLog)