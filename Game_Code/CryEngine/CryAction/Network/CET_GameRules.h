// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CET_GAMERULES_H__
#define __CET_GAMERULES_H__

#pragma once

#include "ClassRegistryReplicator.h"

void AddGameRulesCreation( IContextEstablisher * pEst, EContextViewState state );
void AddGameRulesReset( IContextEstablisher * pEst, EContextViewState state );
void AddSendGameType( IContextEstablisher * pEst, EContextViewState state, CClassRegistryReplicator * pRep, std::vector<SSendableHandle>* pWaitFor );
void AddSendResetMap( IContextEstablisher * pEst, EContextViewState state );
void AddOnClientConnect( IContextEstablisher * pEst, EContextViewState state, bool isReset );
void AddOnClientEnteredGame( IContextEstablisher * pEst, EContextViewState state, bool isReset );
void AddInitImmersiveness( IContextEstablisher * pEst, EContextViewState state );
void AddClearOnHold( IContextEstablisher * pEst, EContextViewState state );
void AddPauseGame( IContextEstablisher * pEst, EContextViewState state, bool pause, bool forcePause );
void AddWaitForPrecachingToFinish( IContextEstablisher * pEst, EContextViewState state, bool* pGameStart );
void AddInitialSaveGame( IContextEstablisher * pEst, EContextViewState state );

#endif
