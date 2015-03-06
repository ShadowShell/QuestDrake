/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 

-------------------------------------------------------------------------
History:
- 13:11:2009	16:25 : Created by David Ramos
*************************************************************************/
#ifndef __SCRIPT_BIND_HIT_DEATH_REACTIONS_H
#define __SCRIPT_BIND_HIT_DEATH_REACTIONS_H

#include "HitDeathReactionsDefs.h"

class CPlayer;
struct HitInfo;

class CScriptBind_HitDeathReactions : public CScriptableBase
{
public:
	CScriptBind_HitDeathReactions(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_HitDeathReactions();

	// Notifies a hit event to the hit death reactions system
	// Params:
	// scriptHitInfo = script table with the hit info
	//
	// Return:
	// TRUE if the hit is processed successfully, FALSE otherwise
	int										OnHit(IFunctionHandler *pH, SmartScriptTable scriptHitInfo);


	// Executes a hit reaction using the default C++ execution code
	// Params:
	// reactionParams = script table with the reaction parameters
	int										ExecuteHitReaction (IFunctionHandler *pH, SmartScriptTable reactionParams);

	// Executes a death reaction using the default C++ execution code
	// Params:
	// reactionParams = script table with the reaction parameters
	int										ExecuteDeathReaction (IFunctionHandler *pH, SmartScriptTable reactionParams);

	// Ends the current reaction
	int										EndCurrentReaction (IFunctionHandler *pH);

	// Run the default C++ validation code and returns its result
	// Params:
	//	validationParams = script table with the validation parameters
	//	scriptHitInfo = script table with the hit info
	// Return:
	//	TRUE is the validation was successful, FALSE otherwise
	int										IsValidReaction (IFunctionHandler *pH, SmartScriptTable validationParams, SmartScriptTable scriptHitInfo);

	// Starts an animation through the HitDeathReactions. Pauses the animation graph while playing it
	// and resumes automatically when the animation ends
	// Params:
	// (sAnimName, bool bLoop = false, float fBlendTime = 0.2f, int iSlot = 0, int iLayer = 0, float fAniSpeed = 1.0f)
	int										StartReactionAnim(IFunctionHandler *pH);

	// Ends the current reaction anim, if any
	int										EndReactionAnim(IFunctionHandler *pH);

	// Starts an interactive action. 
	// Params:
	// szActionName = name of the interactive action
	int										StartInteractiveAction(IFunctionHandler *pH, const char* szActionName);

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

private:
	CPlayer*							GetAssociatedActor(IFunctionHandler *pH) const;
	CHitDeathReactionsPtr GetHitDeathReactions(IFunctionHandler *pH) const;

	SmartScriptTable	m_pParams;

	ISystem*					m_pSystem;
	IGameFramework*		m_pGameFW;
};

#endif // __SCRIPT_BIND_HIT_DEATH_REACTIONS_H
