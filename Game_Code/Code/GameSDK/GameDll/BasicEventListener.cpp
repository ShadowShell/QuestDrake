#include "StdAfx.h"
#include "IHardwareMouse.h"
#include "Game.h"
#include "BasicEventListener.h"
#include "IPlayerProfiles.h"
#include "GameCVars.h"

IBasicEventListener::~IBasicEventListener()
{

}

const bool CBasicEventListener::g_debugWindowsMessages = false;

CBasicEventListener::CBasicEventListener() : IBasicEventListener()
{
	m_iLastMoveX = 0;
	m_iLastMoveY = 0;
}

CBasicEventListener::EAction CBasicEventListener::OnClose(HWND hWnd)
{
	if (gEnv && gEnv->pSystem)
		gEnv->pSystem->Quit();
	return eA_None;
}

#ifdef WIN32

CBasicEventListener::EAction CBasicEventListener::OnMouseActivate(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_MOUSEACTIVATE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	return eA_ActivateAndEat;
}

CBasicEventListener::EAction CBasicEventListener::OnEnterSizeMove(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_ENTERSIZEMOVE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
	{
		gEnv->pSystem->GetIHardwareMouse()->IncrementCounter();
	}
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnExitSizeMove(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_EXITSIZEMOVE (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
	{
		gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
	}
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnEnterMenuLoop(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_ENTERMENULOOP (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
	{
		gEnv->pSystem->GetIHardwareMouse()->IncrementCounter();
	}
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnExitMenuLoop(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_EXITMENULOOP (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if (gEnv && gEnv->pSystem && gEnv->pSystem->GetIHardwareMouse())
	{
		gEnv->pSystem->GetIHardwareMouse()->DecrementCounter();
	}
	return  eA_None;
}


CBasicEventListener::EAction CBasicEventListener::OnHotKey(HWND hWnd)
{
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnSycChar(HWND hWnd)
{
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnChar(HWND hWnd, WPARAM wParam)
{
	if (gEnv && gEnv->pInput)
	{
		// TODO: This code doesn't belong here, should be integrated into CKeyboard to make OnInputEvent and OnInputEventUI consistent across all devices and when in the editor
		SInputEvent event;
		event.modifiers = gEnv->pInput->GetModifiers();
		event.deviceType = eIDT_Keyboard;
		event.state = eIS_UI;
		event.value = 1.0f;
		event.pSymbol = 0;//m_rawKeyboard->GetSymbol((lParam>>16)&0xff);
		if (event.pSymbol)
			event.keyId = event.pSymbol->keyId;


		wchar_t tmp[2] = { 0 };
		MultiByteToWideChar(CP_ACP, 0, (char*)&wParam, 1, tmp, 2);

		char szKeyName[4] = {0};
		if (wctomb(szKeyName, (WCHAR)wParam) != -1)
		{
			if (szKeyName[1]==0 && ((unsigned char)szKeyName[0])>=32)
			{
				event.inputChar = tmp[0];
				event.keyName = szKeyName;
				gEnv->pInput->PostInputEvent(event);
			}
		}
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnIMEChar(HWND hWnd)
{
	return  eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnSysKeyDown(HWND hWn, WPARAM wParam, LPARAM lParam)
{
	if (wParam != VK_RETURN && wParam != VK_F4)
	{
		return eA_None;
	}
	else
	{
		if (wParam == VK_RETURN && (lParam & (1<<29)) != 0)
		{
			if (gEnv && gEnv->pRenderer)
			{
				ICVar *pVar = gEnv->pConsole->GetCVar("r_Fullscreen");
				if (pVar)
				{
					if(IPlayerProfileManager *pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager())
					{
						if(const char *pCurrentUser = pPlayerProfileManager->GetCurrentUser())
						{
							int	fullscreen = pVar->GetIVal();

							pVar->Set((int)(fullscreen == 0));

							IPlayerProfileManager::EProfileOperationResult	result;

							pPlayerProfileManager->SaveProfile(pCurrentUser, result, ePR_Options);
						}
					}
				}
			}
		}
		// let the F4 pass through to default handler (it will send an WM_CLOSE)
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSetCursor(HWND hWnd)
{
	int iResource = 105;
	HCURSOR hCursor = LoadCursor(GetModuleHandle(0),MAKEINTRESOURCE(iResource));
	::SetCursor(hCursor);
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMouseMove(HWND hWnd, LPARAM lParam)
{
	if(gEnv && gEnv->pHardwareMouse)
	{
		m_iLastMoveX = LOWORD(lParam);
		m_iLastMoveY = HIWORD(lParam);
		gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_MOVE);
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonDown(HWND hWnd, LPARAM lParam)
{
	if(gEnv && gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONDOWN);
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMouseWheel(HWND hWnd, LPARAM lParam, short WheelDelta)
{
	if(gEnv && gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->Event(m_iLastMoveX, m_iLastMoveY,HARDWAREMOUSEEVENT_WHEEL, WheelDelta);
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonUp(HWND hWnd, LPARAM lParam)
{
	if(gEnv && gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONUP);
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonDoubleClick(HWND hWnd, LPARAM lParam)
{
	if(gEnv && gEnv->pHardwareMouse)
	{
		gEnv->pHardwareMouse->Event(LOWORD(lParam),HIWORD(lParam),HARDWAREMOUSEEVENT_LBUTTONDOUBLECLICK);
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMove(HWND hWnd, LPARAM lParam)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_MOVE %d %d (%s %s)", LOWORD(lParam), HIWORD(lParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE,LOWORD(lParam), HIWORD(lParam));
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSize(HWND hWnd, LPARAM lParam)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_SIZE %d %d (%s %s)", LOWORD(lParam), HIWORD(lParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE,LOWORD(lParam), HIWORD(lParam));
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnActivate(HWND hWnd, WPARAM wParam)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_ACTIVATE %d (%s %s)", LOWORD(wParam), (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");

	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		// Pass HIWORD(wParam) as well to indicate whether this window is minimized or not
		// HIWORD(wParam) != 0 is minimized, HIWORD(wParam) == 0 is not minimized
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_ACTIVATE, LOWORD(wParam) != WA_INACTIVE, HIWORD(wParam));
	}
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnSetFocus(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_SETFOCUS (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 1, 0);
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnKillFocus(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_KILLFOCUS (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_CHANGE_FOCUS, 0, 0);
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnWindowPositionChanged(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_WINDOWPOSCHANGED (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_POS_CHANGED, 1, 0);
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnWindowStyleChanged(HWND hWnd)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_STYLECHANGED (%s %s)", (GetFocus()==hWnd)?"focused":"", (GetForegroundWindow()==hWnd)?"foreground":"");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_STYLE_CHANGED, 1, 0);
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnInputLanguageChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_INPUTLANGCHANGE");
	if(gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
	{
		bool bSendEvent = true;

		if(bSendEvent)
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LANGUAGE_CHANGE, wParam, lParam);
		}
	}
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSysCommand(HWND hWnd, WPARAM wParam)
{
	if (g_debugWindowsMessages && gEnv && gEnv->pLog)
		gEnv->pLog->Log("MSG: WM_SYSCOMMAND");

	switch (wParam & 0xFFF0)
	{
	case SC_SCREENSAVE:
		{
			const ICVar* pCVar = (gEnv && gEnv->pConsole) ? gEnv->pConsole->GetCVar("sys_screensaver_allowed") : 0;
			return (pCVar && pCVar->GetIVal() == 0) ? eA_None : eA_Default;
		}
	//case SC_MONITORPOWER:
	//	return eA_None;
	default:
		return eA_Default;
	}
}

#else

CBasicEventListener::EAction CBasicEventListener::OnMouseActivate(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnEnterSizeMove(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnExitSizeMove(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnEnterMenuLoop(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnExitMenuLoop(HWND hWnd)
{
	return eA_Default;
}


CBasicEventListener::EAction CBasicEventListener::OnHotKey(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSycChar(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnIMEChar(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnChar(HWND hWnd, WPARAM wParam)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSysKeyDown(HWND hWn, WPARAM wParam, LPARAM lParam)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSetCursor(HWND hWnd)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMouseMove(HWND hWnd, LPARAM lParam)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonDown(HWND hWnd, LPARAM lParam)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMouseWheel(HWND hWnd, LPARAM lParam, short WheelDelta)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonUp(HWND hWnd, LPARAM lParam)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnLeftButtonDoubleClick(HWND hWnd, LPARAM lParam)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnMove(HWND hWnd, LPARAM lParam)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSize(HWND hWnd, LPARAM lParam)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnActivate(HWND hWnd, WPARAM wParam)
{
	return eA_None;
}

CBasicEventListener::EAction CBasicEventListener::OnSetFocus(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnKillFocus(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnWindowPositionChanged(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnWindowStyleChanged(HWND hWnd)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnInputLanguageChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	return eA_Default;
}

CBasicEventListener::EAction CBasicEventListener::OnSysCommand(HWND hWnd, WPARAM wParam)
{
	return eA_Default;
}

#endif
