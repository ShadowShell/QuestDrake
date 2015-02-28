/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Central storage and processing of all gamecode client side 
					   monitoring

-------------------------------------------------------------------------
History:
- 09:11:2011: Created by Stewart Needham

*************************************************************************/
#ifndef ___CLIENT_CHEAT_MONITOR_H___
#define ___CLIENT_CHEAT_MONITOR_H___

#include "AntiCheat/AnticheatDefines.h"
#include "Utility/MaskedVar.h"


#if CLIENT_CHEAT_MONITOR_ENABLED
#include <IGameFramework.h>


class CClientCheatMonitor : public IGameFrameworkListener
{
public:
	CClientCheatMonitor();
	~CClientCheatMonitor();
		
					void	Init(IGameFramework *pFramework);
					 
	// IGameFrameworkListener
	virtual void	OnPostUpdate(float fDeltaTime);
	virtual void	OnSaveGame(ISaveGame* pSaveGame);
	virtual void	OnLoadGame(ILoadGame* pLoadGame);
	virtual void	OnLevelEnd(const char* nextLevel);
	virtual void	OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	void UploadTelemetry();
	
private:
	ILINE void * GetRealMonitorPointer(void * pPtr) { return (void*)((size_t)pPtr - m_MonitorAccessOffset); }

	size_t				m_MonitorAccessOffset;
	IGameFramework* m_pFramework;
};
#else
class CClientCheatMonitor
{
public:
	ILINE void UploadTelemetry() {}
};
#endif //CLIENT_CHEAT_MONITOR_ENABLED
#endif // ___CLIENT_CHEAT_MONITOR_H___
