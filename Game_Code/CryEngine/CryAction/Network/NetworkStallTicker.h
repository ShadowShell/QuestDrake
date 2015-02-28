// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __NETWORK_STALL_TICKER_H__
#define __NETWORK_STALL_TICKER_H__

#pragma once


//--------------------------------------------------------------------------
// a special ticker thread to run during load and unload of levels

#ifdef USE_NETWORK_STALL_TICKER_THREAD


#include <ISystem.h>
#include <ICmdLine.h>

#include "IGameFramework.h"


class CNetworkStallTickerThread : public CrySimpleThread<>	//in multiplayer mode
{
public:
	CNetworkStallTickerThread()
	{
		m_threadRunning = true;
	}

	virtual void Run();

	void FlagStop()
	{
		m_threadRunning = false;
	}

	void Cancel()
	{
	}

private:
	bool m_threadRunning;
};


#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD
//--------------------------------------------------------------------------

#endif
