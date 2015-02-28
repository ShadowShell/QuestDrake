////////////////////////////////////////////////////////////////////////////
//
//  CryENGINE Header File.
//  Copyright (C), Crytek Studios, 2001-2013.
// -------------------------------------------------------------------------
//  File name:   CScriptBind_MusicLogic.h
//  Version:     v1.00
//  Created:     28.08.2006 by Tomas Neumann
//  Description: Script Binding for MusicLogic
// -------------------------------------------------------------------------
//  History:
//           11.04.2013 by Thomas Wollenzin
//            -  minor cleanup and moved from MusicSystem
//
////////////////////////////////////////////////////////////////////////////
#ifndef __SCRIPTBIND_MUSICLOGIC_H__
#define __SCRIPTBIND_MUSICLOGIC_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>

struct IMusicLogic;


class CScriptBind_MusicLogic : public CScriptableBase
{
public:

	CScriptBind_MusicLogic(IScriptSystem* const pScriptSystem, ISystem* const pSystem);
	virtual ~CScriptBind_MusicLogic();

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

protected:

	//! <code>MusicLogic.SetEvent( eMusicEvent )</code>
	//!		<param name="eMusicEvent">identifier of a music event.</param>
	//! <description>
	//!		Sets an event in the music logic. Available music event ids are:
	//!		<pre>eMUSICLOGICEVENT_VEHICLE_ENTER
	//!		eMUSICLOGICEVENT_VEHICLE_LEAVE
	//!		eMUSICLOGICEVENT_WEAPON_MOUNT
	//!		eMUSICLOGICEVENT_WEAPON_UNMOUNT
	//!		eMUSICLOGICEVENT_ENEMY_SPOTTED
	//!		eMUSICLOGICEVENT_ENEMY_KILLED
	//!		eMUSICLOGICEVENT_ENEMY_HEADSHOT
	//!		eMUSICLOGICEVENT_ENEMY_OVERRUN
	//!		eMUSICLOGICEVENT_PLAYER_WOUNDED
	//!		eMUSICLOGICEVENT_MAX</pre>
	//! </description>
	int SetEvent(IFunctionHandler* pH, int eMusicEvent);

	//! <code>MusicLogic.StartLogic()</code>
	//! <description>Starts the music logic.</description>
	int StartLogic(IFunctionHandler* pH);

	//! <code>MusicLogic.StopLogic()</code>
	//! <description>Stops the music logic.</description>
	int StopLogic(IFunctionHandler* pH);

	//! <code>MusicLogic.SendEvent( eventName )</code>
	//! <description>Send an event to the music logic.</description>
	int SendEvent(IFunctionHandler* pH, const char *pEventName);

private:

	void RegisterGlobals();
	void RegisterMethods();

	IScriptSystem* m_pSS;
	IMusicLogic*   m_pMusicLogic;
};

#endif //__SCRIPTBIND_GAME_H__
