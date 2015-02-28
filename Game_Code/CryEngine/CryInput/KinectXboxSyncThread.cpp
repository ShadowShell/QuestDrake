// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"

#if !defined(RELEASE) && defined(WIN32) && !defined(WIN64) && defined(USE_KINECT)

#include "KinectXboxSyncThread.h"
#include <IConsole.h>
#include <MSR_NuiApi.h>

// Link to Windows Sockets
#pragma comment(lib, "Ws2_32.lib")

// Include and link the delay-loaded xbdm.dll
#include <Xbdm.h>

//////////////////////////////////////////////////////////////////////////

CKinectXboxSyncThread::CKinectXboxSyncThread() : 
	m_XboxListenSocket(INVALID_SOCKET),
	m_bRequestShutdown(false),
	m_bUseCompression(false),
	m_bUseSeatedST(false),
	m_bSeatedSTFlagDirty(false)
{	
}

CKinectXboxSyncThread::~CKinectXboxSyncThread()
{
	Shutdown();
}

//////////////////////////////////////////////////////////////////////////

bool CKinectXboxSyncThread::Initialize()
{
	ILog* iLog = gEnv->pLog;
	m_XboxListenSocket	=	SOCKET_ERROR;

	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: Failed to initialize Winsock\n");
		return false;
	}

	int Err = SOCKET_ERROR;

	// if IP is provided in CVar then use that one directly, else try to retrieve using XBDM
	string XboxIP = g_pInputCVars->i_kinectXboxConnectIP->GetString();
	if (XboxIP.length() == 0)
		XboxIP = GetXboxIP();

	// if we still don't have a valid IP then don't bother connecting
	if (XboxIP.length() == 0)
	{
		iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: can't get valid XBox IP address (either have a default Xbox IP in the neighbourhood or provide one with i_kinectXboxConnectIP)\n");
		return false;
	}

	m_XboxListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(m_XboxListenSocket == INVALID_SOCKET)
	{
		iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: can't create client socket: error %i\n",m_XboxListenSocket);
		return false;
	}

	int arg = 1;
	setsockopt(m_XboxListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&arg, sizeof arg);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_pInputCVars->i_kinectXboxConnectPort); 
	addr.sin_addr.s_addr = inet_addr(XboxIP.c_str());

	Err = connect(m_XboxListenSocket, (struct sockaddr *)&addr, sizeof addr);

	if(Err<0)
	{
		int WSAError = WSAGetLastError();
		iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: could not connect to %s (error %i, sys_net_errno=%i)\n", XboxIP.c_str(), Err, WSAError);

		// if buffer is full try sleeping a bit before retrying
		// (if you keep getting this issue then try using same shutdown mechanism as server is doing (see server code))
		// (for more info on windows side check : http://www.proxyplus.cz/faq/articles/EN/art10002.htm)
		if (WSAError == WSAENOBUFS)
		{
			Sleep(5000);
		}

		//socketclose(s);
		//return (size_t)-1;
		struct timeval tv;
		struct fd_set emptySet;
		FD_ZERO(&emptySet);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		closesocket(m_XboxListenSocket);
		m_XboxListenSocket = INVALID_SOCKET;
		return false;
	}

	if (m_XboxListenSocket == INVALID_SOCKET)
	{
		iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: invalid socket after trying to connect: error %i, sys_net_errno=%i\n", Err, WSAGetLastError() );
		return false;
	}

	{
		char szIPAddress[1024];
		if(gethostname(szIPAddress, sizeof(szIPAddress)) == SOCKET_ERROR)
		{
			iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: invalid return value on gethostname: sys_net_errno=%i\n", WSAGetLastError() );
			return false;
		}

		struct hostent *host = gethostbyname(szIPAddress);
		if(host == NULL)
		{
			iLog->LogError("ERROR: CKinectXboxSyncThread::Initialize: invalid return value on gethostbyname: sys_net_errno=%i\n", WSAGetLastError() );
			return false;
		}

		//Obtain the computer's IP
		unsigned char b1, b2, b3, b4;
		b1 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1;
		b2 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2;
		b3 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3;
		b4 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4;

		sprintf_s(szIPAddress, 128, "%d.%d.%d.%d", b1, b2 , b3, b4);

		// tell the app about our IP address
		char acSendData[128];
		sprintf_s(acSendData, 128, "IP:%s", szIPAddress);
		::send(m_XboxListenSocket, acSendData, 128, 0);
	}

	{
		// tell the app that we are listening
		char acData[128];
		sprintf_s(acData, 128, "Status:2");
		::send(m_XboxListenSocket, acData, 128, 0);
	}

	return true;
}

void CKinectXboxSyncThread::Shutdown()
{
	// request thread shutdown and wait until done running
	m_bRequestShutdown = true;
	while (IsRunning())
		Sleep(5);
}

//////////////////////////////////////////////////////////////////////////

string CKinectXboxSyncThread::GetXboxIP()
{
	// The xbdm.dll contains the functionality that DebugConsole needs. However
	// this DLL is not in the system path, and shouldn't be. To avoid this we
	// get DebugConsole to add %XEDK%\bin\win32 to its own copy of the path.
	// Then it loads xbdm.dll. This works because the project specifies that
	// xbdm.dll should be delay loaded, so the Win32 loader doesn't load it
	// when the process loads. As long as we load xbdm.dll before we use it
	// this all works magically.
	// If DebugConsole tries to use xbdm.dll and it cannot be loaded then
	// an exception is thrown. Loading xbdm.dll now is the best way to ensure
	// that it will be available when it is needed.
	// Alternate strategies would include copying xbdm.dll to the tool's directory,
	// or using the COM interface.
	const char* errorMessage = NULL;
	char* path;
	_dupenv_s( &path, NULL, "path" );

	char* xedkDir;
	_dupenv_s( &xedkDir, NULL, "xedk" );

	if( !xedkDir )
		xedkDir = "";
	// Build up a new path using std::string to handle memory management.
	string newpath = "path=" + string( path ) + ";" + string( xedkDir ) + "\\bin\\win32";
	// Set the path to the update version.
	_putenv( newpath.c_str() );
	HMODULE hXBDM = LoadLibrary( TEXT( "xbdm.dll" ) );
	if( !hXBDM )
	{
		if( xedkDir[0] )
			errorMessage = "Couldn't load xbdm.dll";
		else
			errorMessage = "Couldn't load xbdm.dll\nXEDK environment variable not set.";
	}

	// If anything goes wrong while trying to load xbdm.dll, bail out with
	// an error message.
	if( errorMessage )
	{
		gEnv->pLog->LogError( errorMessage );
		return "";
	}

	typedef HRESULT (_stdcall *TDmSetConnectionTimeout)(DWORD,DWORD);
	typedef HRESULT (_stdcall *TDmGetNameOfXbox)(LPSTR,LPDWORD,BOOL);
	typedef HRESULT (_stdcall *TDmGetAltAddress)(LPDWORD);

	TDmSetConnectionTimeout pDmSetConnectionTimeout = reinterpret_cast<TDmSetConnectionTimeout>(GetProcAddress(hXBDM, "DmSetConnectionTimeout"));
	TDmGetNameOfXbox pDmGetNameOfXbox								= reinterpret_cast<TDmGetNameOfXbox>(GetProcAddress(hXBDM, "DmGetNameOfXbox"));
	TDmGetAltAddress pDmGetAltAddress								= reinterpret_cast<TDmGetAltAddress>(GetProcAddress(hXBDM, "DmGetAltAddress"));

	// put only half a second of connection timeout
	HRESULT eResult = pDmSetConnectionTimeout(500, 500);
	if (eResult != XBDM_NOERR)
		return "";

	char acName[512];
	DWORD dwSize;
	eResult = pDmGetNameOfXbox(acName, &dwSize, true);
	if (eResult != XBDM_NOERR)
		return "";

	DWORD dwAltAddress;
	eResult = pDmGetAltAddress(&dwAltAddress);
	if (eResult != XBDM_NOERR)
		return "";

	// endian swap the ip address to make it readable on PC
	SwapEndian(dwAltAddress, true);

	// transform into string IP address
	struct in_addr addr;
	addr.s_addr = (long)dwAltAddress;
	string XboxIP = inet_ntoa(addr);

	return XboxIP;
}

//////////////////////////////////////////////////////////////////////////

void CKinectXboxSyncThread::GetSkeletonFrame(SKinSkeletonFrame& skeletonFrame)
{
	CryAutoCriticalSection lock(m_XboxListenCS);

	// copy the latest skeleton data received from the network to the game skeleton
	skeletonFrame.dwFlags						= m_XboxListenSkeletonFrame.dwFlags;
	skeletonFrame.dwFrameNumber			= m_XboxListenSkeletonFrame.dwFrameNumber;
	skeletonFrame.liTimeStamp				= m_XboxListenSkeletonFrame.liTimeStamp;
	skeletonFrame.vFloorClipPlane		= m_XboxListenSkeletonFrame.vFloorClipPlane;
	skeletonFrame.vNormalToGravity	= m_XboxListenSkeletonFrame.vNormalToGravity;
	for( DWORD dwSkeletonIndex = 0; dwSkeletonIndex < NUI_SKELETON_COUNT; ++ dwSkeletonIndex )
	{
		skeletonFrame.skeletonRawData[dwSkeletonIndex] = m_XboxListenSkeletonFrame.skeletonRawData[dwSkeletonIndex];
	}
}

//////////////////////////////////////////////////////////////////////////

void CKinectXboxSyncThread::Run()
{
	// Main thread loop
	while (!m_bRequestShutdown)
	{
		// blocking call
		std::vector<uint8> rSkeletonData;
		if (ReceiveDataFromSocket(rSkeletonData))
		{
			if (m_bUseCompression)
			{
				// Decompress incoming skeleton data
				std::vector<uint8> rCompressedData;
				rCompressedData.swap(rSkeletonData);
				uint32 nSrcUncompressedLen = *(uint32*)&rCompressedData[0];
				size_t nUncompressedLen = (size_t)nSrcUncompressedLen;
				rSkeletonData.resize(nUncompressedLen);
				if (nUncompressedLen < 1000000 && nUncompressedLen > 0)
				{
					if (gEnv->pSystem->DecompressDataBlock( &rCompressedData[4],rCompressedData.size()-4,&rSkeletonData[0],nUncompressedLen ))
					{
						CryAutoCriticalSection lock(m_XboxListenCS);
						m_XboxListenSkeletonFrame = *(SKinSkeletonFrame*)&rSkeletonData[0];					
					}
				}
			} else {
				CryAutoCriticalSection lock(m_XboxListenCS);
				m_XboxListenSkeletonFrame = *(SKinSkeletonFrame*)&rSkeletonData[0];
			}

			// this is not 100% thread safe, but does it need to be for now?
			if (m_bSeatedSTFlagDirty)
			{
				char acSeatedSTUpdate[128];
				sprintf_s(acSeatedSTUpdate, 128, "SeatedST:%d", m_bUseSeatedST);
				::send(m_XboxListenSocket, acSeatedSTUpdate, 128, 0);
				m_bSeatedSTFlagDirty = false;
			}

			char acStatus[128];
			sprintf_s(acStatus, 128, "Status:2");
			::send(m_XboxListenSocket, acStatus, 128, 0);
		} else {
			// some network related error ? 
			// perhaps try to reconnect ?
			int iError = 0;
		}
	}
}

void CKinectXboxSyncThread::Cancel()
{

}

//////////////////////////////////////////////////////////////////////////

void CKinectXboxSyncThread::SetSeatedST(bool bValue)
{
	m_bUseSeatedST = bValue;
	m_bSeatedSTFlagDirty = true;
}

//////////////////////////////////////////////////////////////////////////

#define MAX_TIME_TO_WAIT 100000

bool CKinectXboxSyncThread::ReceiveDataFromSocket(std::vector<uint8>& rData)
{
	uint32 nMsgLength = 0;
	uint32 nTotalRecived = 0;
	const size_t	BLOCKSIZE	=	8*1024;
	const size_t	SIZELIMIT	=	1024*1024;
	rData.resize(0);
	rData.reserve(64*1024);
	int CurrentPos	=	0;
	while(rData.size()<SIZELIMIT)
	{
		rData.resize(CurrentPos+BLOCKSIZE);

		int Recived = SOCKET_ERROR;
		int waitingtime = 0;
		while (Recived < 0)
		{
			Recived = recv(m_XboxListenSocket,reinterpret_cast<char*>(&rData[CurrentPos]),BLOCKSIZE, 0);
			if (Recived == SOCKET_ERROR)
			{
				int WSAError = WSAGetLastError();
				if (WSAError == WSAEWOULDBLOCK)
				{
					// are we out of time 
					if (waitingtime > MAX_TIME_TO_WAIT)
					{
						return false;
					}

					waitingtime += 5;

					// sleep a bit and try again 
					Sleep(5);
				} else {
					return false;
				}
			}
		}

		if (Recived >= 0)
			nTotalRecived += Recived;

		if (nTotalRecived >= 4)
			nMsgLength = *(uint32*)&rData[0] + 4;

		if(Recived == 0 || nTotalRecived == nMsgLength)
		{
			rData.resize(nTotalRecived);
			break;
		}
		CurrentPos	+=	Recived;
	}

	if(rData.size() == 0)
	{	
		return false;
	}

	if (rData.size() > 4)
	{
		memmove( &rData[0],&rData[4],rData.size()-4 );
		rData.resize(rData.size()-4);
	}

	return	rData.size()>=4;
}

#endif