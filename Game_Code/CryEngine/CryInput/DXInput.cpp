/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
-------------------------------------------------------------------------
History:
- Jan 31,2001:	Created by Marco Corbetta
- Dec 01,2005:	Major rewrite by Marco Koegler
- Dec 05,2005:	Rename CInput to CDXInput ... now platform-specific
- Dec 18,2005:	Many refinements and abstraction

TODO:
- GetOSKeyName functionality needs to be restored
- investigate XInput slowdown
- get initial keystate when gaining focus
- Events for (dis)connect
- DirectX support (joystick)
- "Interpretation" Layer
- action-combinations ("xi_a + xi_b")

*************************************************************************/
#include "StdAfx.h"
#include "DXInput.h"

#ifdef USE_DXINPUT

#include <ILog.h>
#include <ISystem.h>
#include <IHardwareMouse.h>
#include <IRenderer.h>
#include "Keyboard.h"
#include "Mouse.h"
#include "XInputDevice.h"
#include "OrbisPadWin.h"
#include "ICmdLine.h"

// Adding in here for now, as DX Input is Windows specific, as this device.
// This is not a direct input device, though.
#include "HeadmountedDevice/HeadMountedDevice.h"

CDXInput* CDXInput::This = 0;

CDXInput::CDXInput(ISystem *pSystem, HWND hwnd) : CBaseInput()
{ 
	m_hwnd	=	hwnd;
	m_prevWndProc = 0;
	This		= this;
};

CDXInput::~CDXInput()
{
	This = NULL;
	/*if (!gEnv->IsEditor())
	{
		::SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_prevWndProc);
	}*/
}

bool CDXInput::Init()
{
//	gEnv->pLog->Log("DXInput::Init()\n");
	CBaseInput::Init();

	/*if (!gEnv->IsEditor())
	{
		m_prevWndProc = (WNDPROC)::GetWindowLongPtr(m_hwnd, GWLP_WNDPROC);
		::SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)CDXInput::InputWndProc);
	}*/

	gEnv->pLog->Log("Initializing DirectInput\n");

	HRESULT hr = DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_pDI, 0); 

	if (FAILED(hr)) 
	{
		gEnv->pLog->Log("Cannot initialize DirectInput\n");		
		return false;
	}			

	// add keyboard and mouse
	if (!AddInputDevice(new CKeyboard(*this))) return false;

	if (GetISystem()->GetICmdLine()->FindArg(eCLAT_Pre, "nomouse") == NULL)
	{
		if (!AddInputDevice(new CMouse(*this))) return false;
	}

	// Adding in here for now, as DX Input is Windows specific, as this device.
	// This is not a direct input device, though.
	if (!AddInputDevice(new CHeadMountedDevice(*this))) 
	{
		return false;
	}

	// add xinput controllers devices
	for (int i= 0; i < 4; ++i)
	{
		if (!AddInputDevice(new CXInputDevice(*this, i))) return false;
	}

	// add Orbis pad
#ifdef USE_ORBISPAD_WIN
	{
		IInputDevice *pOrbisPad = CreateOrbisPadWin(GetISystem(), *this, 0);
		if (pOrbisPad)
		{
			if (!AddInputDevice(pOrbisPad)) return false;
		}
	}
#endif // #if CRY_USE_ORBISPADWIN

	ClearKeyState();
	return true;	
}

void CDXInput::Update(bool bFocus)
{
	CBaseInput::Update(bFocus);
}


void CDXInput::ShutDown()
{
	gEnv->pLog->Log("DXInput Shutdown\n");

	CBaseInput::ShutDown();

	m_pDI->Release();
	m_pDI= 0;

	delete this;
}

void CDXInput::ClearKeyState()
{
	CBaseInput::ClearKeyState();
	unsigned char sKState[256] = {0};
	wchar_t buff[8];

	// reset DeadKey buffer in windows ... for reference why this is done by calling ToUnicode()
	// see http://blogs.msdn.com/michkap/archive/2005/01/19/355870.aspx
	::ToUnicode('A', DIK_A, sKState, buff, 8, 0);
}

void CDXInput::SetExclusiveMode( EInputDeviceType deviceType, bool exclusive, void *pUser )
{
	if (pUser && (m_hwnd != (HWND)pUser))
	{
		// unhook
		if (m_prevWndProc && deviceType == eIDT_Keyboard)
		{
			//::SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_prevWndProc);
			//m_prevWndProc = 0;
		}

		m_hwnd = (HWND)pUser;

		if (deviceType == eIDT_Keyboard)
		{
			// hook ourself into the message loop
			//m_prevWndProc = (WNDPROC)::GetWindowLongPtr(m_hwnd, GWLP_WNDPROC);
			//::SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)CDXInput::InputWndProc);
		}
	}

	CBaseInput::SetExclusiveMode(deviceType, exclusive, pUser);
}

// reroute to instance function
LRESULT CALLBACK CDXInput::InputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return This->OnInputWndProc(hWnd, message, wParam, lParam);
}

LRESULT CDXInput::OnInputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INPUT)
	{
		// process raw input
		UINT dwSize = 0;

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
/*
		if (dwSize > m_rawBufferSize)
		{
			delete[] m_rawBuffer;
			m_rawBuffer = new BYTE[dwSize];
			m_rawBufferSize = dwSize;
		}

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, m_rawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize )
		{
			gEnv->pLog->LogError("GetRawInputData doesn't return correct size !\n");
		}
		else
		{
			RAWINPUT* raw = (RAWINPUT*)m_rawBuffer;

			if (raw->header.dwType == RIM_TYPEKEYBOARD) 
			{
				if (m_rawKeyboard)
					m_rawKeyboard->Process(raw->data.keyboard);
			}
			else if (raw->header.dwType == RIM_TYPEMOUSE) 
			{
				if (m_rawMouse)
					m_rawMouse->Process(raw->data.mouse);
			} 
		}
*/
	}
	else if (message == WM_CHAR)
	{
		SInputEvent event;
		event.modifiers = GetModifiers();
		event.deviceType = eIDT_Keyboard;
		event.state = eIS_UI;
		event.value = 1.0f;
		event.pSymbol = 0;//m_rawKeyboard->GetSymbol((lParam>>16)&0xff);
		if (event.pSymbol)
			event.keyId = event.pSymbol->keyId;

		event.inputChar = (wchar_t)wParam;
		PostInputEvent(event);
	}

	return ::CallWindowProc(m_prevWndProc, hWnd, message, wParam, lParam);
}


#endif //USE_DXINPUT
