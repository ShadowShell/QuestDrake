//==================================================================================================
// Name: GameEffectSoftCodeLibrary
// Desc: Game Effect Soft Code Library
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include <TypeLibrary.h>
#include "Effects/GameEffects/GameEffect.h"
#include "Effects/GameEffectsSystem.h"

#ifdef SOFTCODE
// Pull in the system and platform util functions
#include <platform_impl.h>
#endif

IMPLEMENT_TYPELIB(IGameEffect,GAME_FX_LIBRARY_NAME); // Implementation of Soft Coding library