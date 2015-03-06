////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   IBasicEventListener.h
//  Version:     v1.00
//  Created:     Fabio
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef IBASIC_EVENT_LISTENER_H
#define IBASIC_EVENT_LISTENER_H

#ifdef WIN32
#include "CryWindows.h"
#endif

struct IBasicEventListener
{
	enum EAction
	{
		eA_Default,
		eA_None,
		eA_ActivateAndEat
	};

	virtual ~IBasicEventListener();
	virtual EAction OnClose(HWND hWnd) = 0;
	virtual EAction OnMouseActivate(HWND hWnd) = 0;
	virtual EAction OnEnterSizeMove(HWND hWnd) = 0;
	virtual EAction OnExitSizeMove(HWND hWnd) = 0;
	virtual EAction OnEnterMenuLoop(HWND hWnd) = 0;
	virtual EAction OnExitMenuLoop(HWND hWnd) = 0;
	virtual EAction OnHotKey(HWND hWnd) = 0;
	virtual EAction OnSycChar(HWND hWnd) = 0;
	virtual EAction OnChar(HWND hWnd, WPARAM wParam) = 0;
	virtual EAction OnIMEChar(HWND hWnd) = 0;
	virtual EAction OnSysKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
	virtual EAction OnSetCursor(HWND hWnd) = 0;
	virtual EAction OnMouseMove(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnLeftButtonDown(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnLeftButtonUp(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnLeftButtonDoubleClick(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnMouseWheel(HWND hWnd, LPARAM lParam, short WheelDelta) = 0;
	virtual EAction OnMove(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnSize(HWND hWnd, LPARAM lParam) = 0;
	virtual EAction OnActivate(HWND hWnd, WPARAM wParam) = 0;
	virtual EAction OnSetFocus(HWND hWnd) = 0;
	virtual EAction OnKillFocus(HWND hWnd) = 0;
	virtual EAction OnWindowPositionChanged(HWND hWnd) = 0;
	virtual EAction OnWindowStyleChanged(HWND hWnd) = 0;
	virtual EAction OnInputLanguageChanged(HWND hWnd, WPARAM wParam, LPARAM lParam) = 0;
	virtual EAction OnSysCommand(HWND hWnd, WPARAM wParam) = 0;
};

#endif
