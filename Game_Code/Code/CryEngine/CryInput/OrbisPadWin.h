/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2014.
---------------------------------------------------------------------
File name:   OrbisPadWin.h
Description: Pad for Orbis in Windows
---------------------------------------------------------------------
History:
- 16.06.2014 : Added by Matthijs van der Meide

*********************************************************************/
#ifndef __ORBISPADWIN_H__
#define __ORBISPADWIN_H__
#pragma once

// TOOLS_SUPPORT_ORBIS is defined only when the developer is supposed to have ORBIS SDK access
// Note: By default, the include files required are located at C:\Program Files (x86)\SCE\Common\External Tools\Pad Library for Windows(PS4)
//       Copy the \include folder from there to <ROOT>\Code\SDKs\OrbisPad\include or otherwise add this header to the include path
// If you don't want this feature at all, just comment the following line
#define WANT_ORBISPAD_WIN

#if defined(TOOLS_SUPPORT_ORBIS) && defined(USE_DXINPUT)  && defined(WIN64)  && defined(WANT_ORBISPAD_WIN)
#ifndef USE_ORBISPAD_WIN
#define USE_ORBISPAD_WIN
#endif
#else
#ifdef USE_ORBISPAD_WIN
#undef USE_ORBISPAD_WIN
#endif
#endif

#ifdef USE_ORBISPAD_WIN
#include "CryInput.h"
#include "InputDevice.h"

// Construct a new COrbisPadWin instance
extern CInputDevice *CreateOrbisPadWin(ISystem *pSystem, CBaseInput &input, int deviceNo);

#endif //#ifdef USE_ORBISPAD_WIN
#endif //#ifdef __ORBISPADWIN_H__
