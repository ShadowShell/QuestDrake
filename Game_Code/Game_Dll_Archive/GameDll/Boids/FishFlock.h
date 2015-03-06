////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   fishflock.h
//  Version:     v1.00
//  Created:     8/2010 by Luciano Morpurgo (refactored from flock.h)
//  Compilers:   Visual C++ 7.0
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __fishflock_h__
#define __fishflock_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <IScriptSystem.h>
#include <IAISystem.h>
#include "Flock.h"

//////////////////////////////////////////////////////////////////////////
class CFishFlock : public CFlock
{
public:
	CFishFlock( IEntity *pEntity ) : CFlock( pEntity,EFLOCK_FISH ) { m_boidEntityName = "FishBoid"; m_boidDefaultAnimName = "swim_loop"; };
	virtual void CreateBoids( SBoidsCreateContext &ctx );
};

#endif // __fishflock_h__
