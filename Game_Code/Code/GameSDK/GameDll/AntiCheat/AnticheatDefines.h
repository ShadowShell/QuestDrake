/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Collects together all the anticheat defines used in the game
						 project. 
-------------------------------------------------------------------------
History:
- 09:11:2011: Created by Stewart Needham

*************************************************************************/
#ifndef __ANTICHEAT_DEFINES_H___
#define __ANTICHEAT_DEFINES_H___

//========================================================================
// ------------------------------ OVERRIDES ------------------------------
// Set these to 1 if you want to force an anticheat system to be enabled
//========================================================================
#define		FORCE__SERVER_CHEAT_MONITOR_ENABLED		0
#define		FORCE__CLIENT_CHEAT_MONITOR_ENABLED		0

//========================================================================
//
// SERVER_CHEAT_MONITOR_ENABLED - Enables server anticheat monitoring code
//
//========================================================================
#ifdef SERVER_CHECKS
#define SERVER_CHEAT_MONITOR_ENABLED		1
#elif FORCE__SERVER_CHEAT_MONITOR_ENABLED
#define SERVER_CHEAT_MONITOR_ENABLED		1
#else
#define SERVER_CHEAT_MONITOR_ENABLED		0
#endif

//========================================================================
//
// CLIENT_CHEAT_MONITOR_ENABLED - Enables client anticheat monitoring code
//
//========================================================================
#if (defined(WIN32) || defined(WIN64)) && !PC_CONSOLE_NET_COMPATIBLE
#define CLIENT_CHEAT_MONITOR_ENABLED		1
#elif FORCE__CLIENT_CHEAT_MONITOR_ENABLED
#define CLIENT_CHEAT_MONITOR_ENABLED		1
#else
#define CLIENT_CHEAT_MONITOR_ENABLED		0
#endif

#endif //__ANTICHEAT_DEFINES_H___ 
