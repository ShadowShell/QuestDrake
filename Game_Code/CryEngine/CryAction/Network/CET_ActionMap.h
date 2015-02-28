// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CET_ACTIONMAP_H__
#define __CET_ACTIONMAP_H__

#pragma once

void AddDisableActionMap( IContextEstablisher * pEst, EContextViewState state );
void AddInitActionMap_ClientActor( IContextEstablisher * pEst, EContextViewState state );
void AddInitActionMap_OriginalSpectator( IContextEstablisher * pEst, EContextViewState state );
void AddDisableKeyboardMouse( IContextEstablisher * pEst, EContextViewState state );

#endif
