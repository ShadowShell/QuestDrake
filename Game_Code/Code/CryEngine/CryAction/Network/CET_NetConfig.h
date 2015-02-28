// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CET_NETCONFIG_H__
#define __CET_NETCONFIG_H__

#pragma once

void AddEstablishedContext( IContextEstablisher * pEst, EContextViewState state, int token );
void AddDeclareWitness( IContextEstablisher * pEst, EContextViewState state );
void AddPopulateRebroadcaster(IContextEstablisher* pEst, EContextViewState state);
void AddDelegateAuthorityToClientActor( IContextEstablisher * pEst, EContextViewState state );
void AddClearPlayerIds( IContextEstablisher * pEst, EContextViewState state );

#endif
