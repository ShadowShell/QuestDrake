// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __CET_CLASSREGISTRY_H__
#define __CET_CLASSREGISTRY_H__

#pragma once

#include "ClassRegistryReplicator.h"

void AddRegisterAllClasses( IContextEstablisher * pEst, EContextViewState state, CClassRegistryReplicator * pRep );
void AddSendClassRegistration( IContextEstablisher * pEst, EContextViewState state, CClassRegistryReplicator * pRep, std::vector<SSendableHandle>** ppWaitFor );
void AddSendClassHashRegistration( IContextEstablisher * pEst, EContextViewState state, CClassRegistryReplicator * pRep, std::vector<SSendableHandle>** ppWaitFor );

#endif
