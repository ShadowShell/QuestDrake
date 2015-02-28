/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	CDXInput is now an IInput implementation which is DirectInput
							specific. This removes all these ugly #ifdefs from this code,
							which is a good thing.
-------------------------------------------------------------------------
History:
- Jan 31,2001:	Created by Marco Corbetta
- Dec 01,2005:	Major rewrite by Marco Koegler
- Dec 05,2005:	Rename CInput to CDXInput ... now platform-specific
- Dec 18,2005:	Many refinements and abstraction

*************************************************************************/
#ifndef __DXINPUT_H__
#define __DXINPUT_H__
#pragma once

#include "BaseInput.h"
#include <map>
#include <queue>

#ifdef USE_DXINPUT

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

struct	ILog;
struct	ISystem;

class CDXInput : public CBaseInput
{
public:
	CDXInput(ISystem *pSystem, HWND hwnd); 
	virtual ~CDXInput();

	// IInput overrides
	virtual bool	Init();
	virtual void	Update(bool bFocus);
	virtual void	ShutDown();
	virtual void	ClearKeyState();
	virtual void SetExclusiveMode(EInputDeviceType deviceType, bool exclusive, void *pUser);
	// ~IInput

	HWND						GetHWnd() const	{	return m_hwnd;	}
	LPDIRECTINPUT8	GetDirectInput() const	{	return m_pDI;	}

private:
	// Window procedure handling
	static LRESULT CALLBACK InputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT OnInputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// very platform specific params
	HWND							m_hwnd;
	LPDIRECTINPUT8		m_pDI;	
	WNDPROC						m_prevWndProc;
	static CDXInput*	This;
};

#endif //USE_DXINPUT

#endif // __DXINPUT_H__
