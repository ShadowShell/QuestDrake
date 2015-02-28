/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Here the actual input implementation gets chosen for the
							different platforms.
-------------------------------------------------------------------------
History:
- Dec 05,2005: Created by Marco Koegler
- Sep 09,2012: Updated by Dario Sancho (added support for Durango)

*************************************************************************/
#ifndef __CRYINPUT_H__
#define __CRYINPUT_H__
#pragma once


#if !defined(DEDICATED_SERVER)
	#if defined(LINUX) || defined(APPLE)
		// Linux client (not dedicated server)
		#define USE_LINUXINPUT
		#include "LinuxInput.h"
  #elif defined(ORBIS)
		#define USE_ORBIS_INPUT
		#include "OrbisInput.h"
	#elif defined (DURANGO)
		#define USE_DURANGOINPUT
		#include "DurangoInput.h"
	#else
		// Win32
		#define USE_DXINPUT
		#include "DXInput.h"
	#endif
	#if !defined(_RELEASE) && !defined(WIN32)
		#define USE_SYNERGY_INPUT
	#endif
	#include "InputCVars.h"
#endif

#endif //__CRYINPUT_H__

