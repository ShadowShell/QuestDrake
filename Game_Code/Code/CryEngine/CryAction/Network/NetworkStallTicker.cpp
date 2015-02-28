// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "GameContext.h"
#include "NetworkStallTicker.h"


//--------------------------------------------------------------------------
// special ticker thread to run during load and unload of levels
#ifdef USE_NETWORK_STALL_TICKER_THREAD

#define WARN_ABOUT_LONG_STALLS_IN_TICKER		(0)	// Log warnings about long stalls in the loading ticker (which would likely cause disconnects)

void CNetworkStallTickerThread::Run()
{
	bool gotLockLastTime=true;

#if WARN_ABOUT_LONG_STALLS_IN_TICKER
	CTimeValue started=gEnv->pTimer->GetAsyncTime();
	CTimeValue ended;
#endif
	SetName("NetworkStallTicker");

	while (m_threadRunning)
	{
		if (gEnv->pNetwork)
		{
			gEnv->pNetwork->SyncWithGame(eNGS_SleepNetwork);
		}

		if (gotLockLastTime)
		{
			CrySleep(33);
		}
		else
		{
			CrySleep(1);
		}

		{
			if (gEnv->pNetwork)
			{
				SCOPED_TICKER_TRY_LOCK;
				if (SCOPED_TICKER_HAS_LOCK)
				{
#ifdef PS3
					if (gEnv->pSystem->IsQuitting() == false)
#endif // #ifdef PS3
					{
						gEnv->pNetwork->SyncWithGame(eNGS_MinimalUpdateForLoading);
						gotLockLastTime=true;
					}
				}
				else
				{
					gotLockLastTime=false;
				}
				gEnv->pNetwork->SyncWithGame(eNGS_WakeNetwork);
			}

#ifdef PS3
			//As Ian has made the callbacks threadsafe; it's fine to call the checking util in this function
			extern void CryEngine_cellSysutilCheckCallback();
			CryEngine_cellSysutilCheckCallback();
#endif // #ifdef PS3

#if WARN_ABOUT_LONG_STALLS_IN_TICKER
			ended = gEnv->pTimer->GetAsyncTime();
			if (ended.GetDifferenceInSeconds(started)>1.f)
			{
				CryLogAlways("THREADEDLOADING:: No update for %f",ended.GetDifferenceInSeconds(started));
			}
			started=ended;
#endif
		}
	}

	Stop();
}

#endif // #ifdef USE_NETWORK_STALL_TICKER_THREAD
//--------------------------------------------------------------------------