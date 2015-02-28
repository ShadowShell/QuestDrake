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
#ifndef _VISUAL_LOG_H_
#define _VISUAL_LOG_H_

#include "IVisualLog.h"

#define VISUAL_LOG_FRAME_PREFIX "Frame"

class CVisualLog : public IVisualLog
{

public:

	/*$1- Singleton Stuff ----------------------------------------------------*/
	static CVisualLog&  ref();
	static bool         Create();
	void                Shutdown();

	/*$1- Basics -------------------------------------------------------------*/
	void  Init();
	void	Update();
	void	Reset();

	/*$1- Utils --------------------------------------------------------------*/
	void Log( const char *format, ... )  PRINTF_PARAMS(2, 3);
	void Log( const SVisualLogParams& params, const char *format, ... )  PRINTF_PARAMS(3, 4);
	
protected:

	/*$1- Creation and destruction via singleton -----------------------------*/
	CVisualLog();
	virtual ~CVisualLog();

private:
	enum EVisualLogFormat
	{
		EVLF_NONE,
		EVLF_JPG,								
		EVLF_BMP					
	};

	void InitCVars();
	bool OpenLogs();
	void CloseLogs();
	bool UpdateLogs();

	// Log writing functions
	void WriteFileHeaders( const char *sLogFileName, const char *sLogParamsFileName );
	void WriteFrameHeaders( int iFrameNum, const char *sImageName );

	EVisualLogFormat GetFormatType( const string& sFormat );

	void ShowDisplayMessage();

	/*$1- Members ------------------------------------------------------------*/
	bool                m_bInit;
	static CVisualLog*	m_pInstance;
	SVisualLogParams		m_defaultParams;
	
	bool								m_bLoggingEnabled;
	int									m_iFrameId;
	EVisualLogFormat		m_eFormat;
	string							m_sFormat;
	string							m_sLogFolder;
	int									m_iLogFolderNum;
	int									m_iLogFrameNum;
	string							m_sLogPath;

	string							m_sLogBuffer;
	string							m_sLogParamsBuffer;

	FILE*								m_fLogFile;
	FILE*								m_fLogParamsFile;

	// Visual Logger CVars
	ICVar*							m_pCVVisualLog;
	ICVar*							m_pCVVisualLogFolder;
	ICVar*							m_pCVVisualLogImageFormat;
	ICVar*							m_pCVVisualLogImageScale;
	ICVar*							m_pCVVisualLogShowHUD;

	// CVars to control renderer 'capture frames' facility
	ICVar* m_pCV_capture_frames;
	ICVar* m_pCV_capture_file_format;
	ICVar* m_pCV_capture_frame_once;
	ICVar* m_pCV_capture_use_scale;
	ICVar* m_pCV_capture_image_scale;
	ICVar* m_pCV_capture_file_name;
};
#endif // _VISUAL_LOG_H_
