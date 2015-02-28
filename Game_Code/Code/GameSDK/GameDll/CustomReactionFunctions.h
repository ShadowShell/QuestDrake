/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
Description: Encapsulates custom (C++ or LUA) function handling for the
Hit and Death reactions system
-------------------------------------------------------------------------
History:
- 18:10:2010	16:01 : Created by David Ramos
*************************************************************************/
#if _MSC_VER > 1000
	# pragma once
#endif

#ifndef __CUSTOM_REACTION_FUNCTIONS_H
#define __CUSTOM_REACTION_FUNCTIONS_H

#include "HitDeathReactionsDefs.h"

struct SReactionParams;
#if !defined(LINUX) && !defined(APPLE) && !defined(ORBIS)
struct SReactionParams::SValidationParams;
#endif
struct HitInfo;
class CPlayer;
class CActor;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CCustomReactionFunctions
{
	// Private types
	typedef Functor4wRet<CActor&, const SReactionParams::SValidationParams&, const HitInfo&, float, bool>	ValidationFunctor;
	typedef stl::hash_map<string, ValidationFunctor, stl::hash_stricmp<string> >													ValidationFncContainer;

	typedef Functor3<CActor&, const SReactionParams&, const HitInfo&>																			ExecutionFunctor;
	typedef stl::hash_map<string, ExecutionFunctor, stl::hash_stricmp<string> >														ExecutionFncContainer;

public:
	CCustomReactionFunctions();

	void	InitCustomReactionsData();

	bool	CallCustomValidationFunction(bool& bResult, ScriptTablePtr hitDeathReactionsTable, CActor& actor, const SReactionParams::SValidationParams& validationParams, const HitInfo& hitInfo, float fCausedDamage) const;
	bool	CallCustomExecutionFunction(ScriptTablePtr hitDeathReactionsTable, const string& function, CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo) const;

	bool	RegisterCustomValidationFunction(const string& sName, const ValidationFunctor& validationFunctor);
	bool	RegisterCustomExecutionFunction(const string& sName, const ExecutionFunctor& executionFunctor);

private:
	// Private methods
	void													RegisterCustomFunctions();
	CHitDeathReactionsPtr					GetActorHitDeathReactions(CActor& actor) const;

	// C++ custom execution functions
	void													FallAndPlay_Reaction(CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo);
	void													DeathImpulse_Reaction(CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo);
	void													DeathImpulse_PowerMelee(CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo);
	void													MeleeDeath_Reaction(CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo);
	void													ReactionDoNothing(CActor& actor, const SReactionParams& reactionParams, const HitInfo& hitInfo);

	// Private attributes
	ValidationFncContainer				m_validationFunctors;	
	ExecutionFncContainer					m_executionFunctors;

	// utility data used by the custom functions
	uint16												m_shotgunShellProjectile;
};

#endif // __CUSTOM_REACTION_FUNCTIONS_H
