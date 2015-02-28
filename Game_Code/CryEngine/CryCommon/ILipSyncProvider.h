////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   ILipSyncProvider.h
//  Version:     v1.00
//  Created:     2014-08-29 by Christian Werle.
//  Description: Interface for the lip-sync provider that gets injected into the IEntitySoundProxy.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BoostHelpers.h"


enum ELipSyncMethod
{
	eLSM_None,
	eLSM_MatchAnimationToSoundName,
};

struct IEntityAudioProxy;

struct ILipSyncProvider
{
	virtual ~ILipSyncProvider() {}

	virtual void RequestLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;  // Use this to start loading as soon as the sound gets requested (this can be safely ignored)
	virtual void StartLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;
	virtual void PauseLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;
	virtual void UnpauseLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;
	virtual void StopLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;
	virtual void UpdateLipSync( IEntityAudioProxy* pProxy, const TAudioControlID nAudioTriggerId, const ELipSyncMethod lipSyncMethod ) = 0;
};

DECLARE_BOOST_POINTERS(ILipSyncProvider);

