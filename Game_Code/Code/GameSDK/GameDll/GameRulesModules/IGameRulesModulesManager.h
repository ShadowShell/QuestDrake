/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 

	-------------------------------------------------------------------------
	History:
	- 02:09:2009  : Created by Colin Gulliver

*************************************************************************/

#ifndef _IGameRulesModulesManager_h_
#define _IGameRulesModulesManager_h_

#if _MSC_VER > 1000
# pragma once
#endif

#include "IGameFramework.h"
#include "GameRulesModules/GameRulesModulesRegistration.h"

class IGameRulesModulesManager
{
public:
	virtual ~IGameRulesModulesManager() {}

// Register as a factory for each module type
#define GAMERULES_MODULE_LIST_FUNC(type, name, lowerCase, useInEditor) DECLARE_GAMEOBJECT_FACTORY(type)
	GAMERULES_MODULE_TYPES_LIST
#undef GAMERULES_MODULE_LIST_FUNC

	virtual void Init() = 0;

	virtual int GetRulesCount() = 0;

	virtual const char* GetRules(int index) = 0;
};

#endif // _IGameRulesModulesManager_h_
