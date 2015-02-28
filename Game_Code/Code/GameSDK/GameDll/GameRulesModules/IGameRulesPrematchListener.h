/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Interface for a class that receives events when pc prematch state changes, etc.
	-------------------------------------------------------------------------
	History:
	- 14:08:2012  : Created by Jonathan Bunner

*************************************************************************/

#ifndef _IGAME_RULES_PREMATCH_LISTENER_H_
#define _IGAME_RULES_PREMATCH_LISTENER_H_

#if _MSC_VER > 1000
# pragma once
#endif

#if USE_PC_PREMATCH
class IGameRulesPrematchListener
{
public:
	virtual ~IGameRulesPrematchListener() {}
	virtual void OnPrematchEnd() = 0;
};
#endif //#USE_PC_PREMATCH

#endif //_IGAME_RULES_PREMATCH_LISTENER_H_
