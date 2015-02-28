////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   BasicEventListener.h
//  Version:     v1.00
//  Created:     Fabio
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef BASIC_EVENT_LISTENER_H
#define BASIC_EVENT_LISTENER_H

#include "IBasicEventListener.h"

class CBasicEventListener : public IBasicEventListener
{
public:
	CBasicEventListener();
	virtual EAction OnClose(HWND hWnd);
	virtual EAction OnMouseActivate(HWND hWnd);
	virtual EAction OnEnterSizeMove(HWND hWnd);
	virtual EAction OnExitSizeMove(HWND hWnd);
	virtual EAction OnEnterMenuLoop(HWND hWnd);
	virtual EAction OnExitMenuLoop(HWND hWnd);
	virtual EAction OnHotKey(HWND hWnd);
	virtual EAction OnSycChar(HWND hWnd);
	virtual EAction OnChar(HWND hWnd, WPARAM wParam);
	virtual EAction OnIMEChar(HWND hWnd);
	virtual EAction OnSysKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam);
	virtual EAction OnSetCursor(HWND hWnd);
	virtual EAction OnMouseMove(HWND hWnd, LPARAM lParam);
	virtual EAction OnLeftButtonDown(HWND hWnd, LPARAM lParam);
	virtual EAction OnLeftButtonUp(HWND hWnd, LPARAM lParam);
	virtual EAction OnLeftButtonDoubleClick(HWND hWnd, LPARAM lParam);
	virtual EAction OnMouseWheel(HWND hWnd, LPARAM lParam, short WheelDelta);
	virtual EAction OnMove(HWND hWnd, LPARAM lParam);
	virtual EAction OnSize(HWND hWnd, LPARAM lParam);
	virtual EAction OnActivate(HWND hWnd, WPARAM wParam);
	virtual EAction OnSetFocus(HWND hWnd);
	virtual EAction OnKillFocus(HWND hWnd);
	virtual EAction OnWindowPositionChanged(HWND hWnd);
	virtual EAction OnWindowStyleChanged(HWND hWnd);
	virtual EAction OnInputLanguageChanged(HWND hWnd, WPARAM wParam, LPARAM lParam);
	virtual EAction OnSysCommand(HWND hWnd, WPARAM wParam);
private:
	static const bool g_debugWindowsMessages;
	int m_iLastMoveX;
	int m_iLastMoveY;
};

#endif
