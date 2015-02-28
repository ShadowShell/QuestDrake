////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ActionController.h"
#include "ActionScope.h"

#include "AnimationDatabase.h"

#include <CryExtension/CryCreateClassInstance.h>

#include "IEntityPoolManager.h"

const uint32 MAX_ALLOWED_QUEUE_SIZE = 10;


CActionController::TActionControllerList CActionController::s_actionControllers;
CActionController::TActionList CActionController::s_actionList;
CActionController::TActionList CActionController::s_tickedActions;

bool CActionController::s_registeredCVars = false;
uint32 CActionController::s_blendChannelCRCs[MANN_NUMBER_BLEND_CHANNELS];

#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
int CActionController::s_mnFatalErrorOnInvalidCharInst = 0;
int CActionController::s_mnFatalErrorOnInvalidEntity = 0;
#endif

#ifndef _RELEASE

CActionController *CActionController::s_debugAC = NULL;
ICVar* CActionController::s_cvarMnDebug = NULL;
int CActionController::s_mnDebugFragments = 0;

void ChangeDebug(ICVar* pICVar)
{
	if (!pICVar)
		return;

	const char* pVal = pICVar->GetString();
	CActionController::ChangeDebug(pVal);
}

void DumpSequence(IConsoleCmdArgs *pArgs)
{
	const int argCount = pArgs->GetArgCount();
	if (argCount >= 2)
	{
		const char* pVal = pArgs->GetArg(1);
		if(pVal)
		{
			float dumpTime = (argCount >= 3) ? (float)atof(pArgs->GetArg(2)) : -1.0f;

			CActionController::DumpSequence(pVal, dumpTime);
		}
	}
}

struct SACEntityNameAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const
	{ 
		return CActionController::GetGlobalActionControllers().size();
	};

	virtual const char* GetValue( int nIndex ) const 
	{
		return CActionController::GetGlobalActionControllers()[nIndex]->GetSafeEntityName();
	};
};

static SACEntityNameAutoComplete s_ACEntityNameAutoComplete;

#endif //!_RELEASE

void CActionController::RegisterCVars()
{
	if (!s_registeredCVars)
	{
		IConsole *pConsole = gEnv->pConsole;
		assert(pConsole);
		if (!pConsole)
			return;

#ifndef _RELEASE
		s_cvarMnDebug = REGISTER_STRING_CB("mn_debug", "", VF_CHEAT, "Entity to display debug information for action controller for", ::ChangeDebug);
		REGISTER_COMMAND("mn_dump", ::DumpSequence, VF_CHEAT, "Dumps specified entity's CryMannequin history in the path specified by mn_sequence_path");

		if(gEnv->pConsole != NULL)
		{
			gEnv->pConsole->RegisterAutoComplete("mn_debug", &s_ACEntityNameAutoComplete);
			gEnv->pConsole->RegisterAutoComplete("mn_dump", &s_ACEntityNameAutoComplete);
		}
		REGISTER_CVAR3("mn_debugfragments", s_mnDebugFragments, 0, VF_CHEAT, "Log out fragment queues");
#endif //!_RELEASE

#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
		REGISTER_CVAR3("mn_fatalerroroninvalidentity", s_mnFatalErrorOnInvalidEntity, 1, VF_CHEAT, "Throw a fatal error when an invalid entity is detected");
		REGISTER_CVAR3("mn_fatalerroroninvalidcharinst", s_mnFatalErrorOnInvalidCharInst, 1, VF_CHEAT, "Throw a fatal error when an invalid character instance is detected");
#endif //!CRYMANNEQUIN_WARN_ABOUT_VALIDITY()

		char channelName[10];
		strcpy(channelName, "channel0");
		for (uint32 i=0; i<MANN_NUMBER_BLEND_CHANNELS; i++)
		{
			channelName[7] = '0'+i;
			s_blendChannelCRCs[i] = MannGenCRC(channelName);
		}

		s_registeredCVars = true;
	}
}

void CActionController::UnregisterCVars()
{
	if (s_registeredCVars)
	{
		IConsole *pConsole = gEnv->pConsole;
		if (pConsole)
		{
#ifndef _RELEASE
			pConsole->UnRegisterAutoComplete("mn_debug");
			pConsole->UnRegisterAutoComplete("mn_dump" );
			pConsole->UnregisterVariable("mn_debug");
			pConsole->RemoveCommand("mn_dump");
			pConsole->UnregisterVariable("mn_debugfragments");
#endif //!_RELEASE

#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
			pConsole->UnregisterVariable("mn_fatalerroroninvalidentity");
			pConsole->UnregisterVariable("mn_fatalerroroninvalidcharinst");
#endif // !CRYMANNEQUIN_WARN_ABOUT_VALIDITY()

			s_registeredCVars = false;
		}
	}
}

void CActionController::Register(CActionController *ac)
{
	RegisterCVars();

	s_actionControllers.push_back(ac);

#ifndef _RELEASE
	::ChangeDebug(s_cvarMnDebug);
#endif //_RELEASE
}

void CActionController::Unregister(CActionController *ac)
{
	TActionControllerList::iterator iter = std::find(s_actionControllers.begin(), s_actionControllers.end(), ac);
	if (iter != s_actionControllers.end())
	{
		s_actionControllers.erase(iter);

		if (s_actionControllers.empty())
		{
			stl::free_container(s_actionControllers);
			stl::free_container(s_actionList);
			stl::free_container(s_tickedActions);
			UnregisterCVars();
		}
	}

#ifndef _RELEASE
	if (ac == s_debugAC)
	{
		s_debugAC = NULL;
	}
#endif //_RELEASE
}

void CActionController::OnShutdown()
{
	const uint32 numControllers = s_actionControllers.size();

	if (numControllers > 0)
	{
		for (TActionControllerList::iterator iter=s_actionControllers.begin(); iter != s_actionControllers.end(); ++iter)
		{
			CActionController *pAC = *iter;
			CryLogAlways("ActionController not released - Owner Controller Def: %s", pAC->GetContext().controllerDef.m_filename.GetString());
		}
		CryFatalError("ActionControllers (%d) not released at shutdown", numControllers);
	}
}

IActionController* CActionController::FindActionController(const IEntity& entity)
{
	const EntityId id = entity.GetId();
	for (TActionControllerList::iterator iter=s_actionControllers.begin(); iter != s_actionControllers.end(); ++iter)
	{
		CActionController *pAC = *iter;
		if (pAC->GetEntityId() == id)
			return pAC;
	}
	return NULL;
}


#ifndef _RELEASE
void CActionController::ChangeDebug(const char *entName)
{
	CActionController *debugAC = NULL;
	for (TActionControllerList::iterator iter=s_actionControllers.begin(); iter != s_actionControllers.end(); ++iter)
	{
		CActionController *ac = *iter;
		if (stricmp(ac->GetSafeEntityName(), entName) == 0)
		{
			debugAC = ac;
			break;
		}
	}

	if (debugAC != s_debugAC)
	{
		if (s_debugAC)
		{
			s_debugAC->SetFlag(AC_DebugDraw, false);
		}
		if (debugAC)
		{
			debugAC->SetFlag(AC_DebugDraw, true);
		}
		s_debugAC = debugAC;
	}
}

void BuildFilename(stack_string &filename, const char *entityName)
{
	char dateTime[128];
	time_t ltime;
	time(&ltime);
	tm *pTm = localtime(&ltime);
	strftime(dateTime, 128, "_%d_%b_%Y_%H_%M_%S.xml", pTm);

	ICVar* pSequencePathCVar = gEnv->pConsole->GetCVar("mn_sequence_path");
	if (pSequencePathCVar)
	{
		filename.append(pSequencePathCVar->GetString());
	}

	filename.append(entityName);
	filename.append(dateTime);
}

void CActionController::DumpSequence(const char *entName, float dumpTime)
{
	CActionController *debugAC = NULL;
	for (TActionControllerList::iterator iter=s_actionControllers.begin(); iter != s_actionControllers.end(); ++iter)
	{
		CActionController *ac = *iter;
		if (stricmp(ac->GetSafeEntityName(), entName) == 0)
		{
			debugAC = ac;
			break;
		}
	}

	if (!debugAC)
	{
		debugAC = s_debugAC;
	}

	if (debugAC)
	{
		stack_string filename;
		BuildFilename(filename, debugAC->GetSafeEntityName());
		debugAC->DumpHistory(filename.c_str(), dumpTime);
	}
}
#endif //!_RELEASE

CActionController::CActionController(IEntity* pEntity, SAnimationContext &context)
:
	m_entityId(pEntity ? pEntity->GetId() : 0),
	m_cachedEntity(pEntity),
	m_context(context),
	m_activeScopes(ACTION_SCOPES_NONE),
	m_flags(0),
	m_timeScale(1.f),
	m_scopeFlushMask(ACTION_SCOPES_NONE),
	m_lastTagStateRecorded(TAG_STATE_EMPTY)
#ifndef _RELEASE
	,m_historySlot(0)
#endif //_RELEASE
{
	const uint32 numScopeContexts = context.controllerDef.m_scopeContexts.GetNum();
	m_scopeContexts = new SScopeContext[numScopeContexts];
	for (uint32 i=0; i<numScopeContexts; i++)
	{
		m_scopeContexts[i].Reset(i);
	}

	const uint32 numScopes = context.controllerDef.m_scopeIDs.GetNum();
	m_scopes.resize(numScopes);
	for (uint32 i=0; i<numScopes; i++)
	{
		const SScopeDef &scopeDef = context.controllerDef.m_scopeDef[i];
		const SScopeContextDef &scopeContextDef = context.controllerDef.m_scopeContextDef[scopeDef.context];
		const TagState additionalTags = context.controllerDef.m_tags.GetUnion(scopeDef.additionalTags, scopeContextDef.additionalTags);
		m_scopes[i] = new CActionScope(context.controllerDef.m_scopeIDs.GetTagName(i), i, *this, context, m_scopeContexts[scopeDef.context], scopeDef.layer, scopeDef.numLayers, additionalTags);
	}

	Register(this);
}

CActionController::~CActionController()
{
}

void CActionController::Release() 
{ 
	if (!m_owningControllers.empty())
	{
		const uint32 numOwningControllers = m_owningControllers.size();
		for (uint32 i=0; i<numOwningControllers; i++)
		{
			m_owningControllers[i]->FlushSlaveController(this);
		}
		m_owningControllers.clear();
	}

	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CActionScope *scope = m_scopes[i];
		if (scope->m_pAction && (&scope->m_pAction->GetRootScope() == scope) && scope->m_pAction->IsStarted())
		{
			scope->m_pAction->Exit();
		}
	}

	ReleaseScopes();
	ReleaseScopeContexts();

	uint32 numProcContexts = m_procContexts.size();
	for (uint32 i=0; i<numProcContexts; i++)
	{
		m_procContexts[i].pContext.reset();
	}

	Unregister(this);

	delete this; 
}

bool CActionController::CanInstall(const IAction &action, TagID subContext, const ActionScopes &scopeMask, float timeStep, float &timeTillInstall) const
{
	timeTillInstall = 0.0f;

	//--- Ensure we test against all effected scopes
	ActionScopes expandedScopeMask = ExpandOverlappingScopes(scopeMask);
	expandedScopeMask &= m_activeScopes;

	//--- Calc frag tag state
	SFragTagState fragTagState(m_context.state.GetMask(), action.GetFragTagState());
	if (subContext != TAG_ID_INVALID)
	{
		const SSubContext subContextDef = m_context.controllerDef.m_subContext[subContext];
		fragTagState.globalTags = m_context.controllerDef.m_tags.GetUnion(fragTagState.globalTags, subContextDef.additionalTags);
	}

	const uint32 numScopes = m_scopes.size();
	for (uint32 i=0, scopeFlag=1; i<numScopes; i++, scopeFlag <<= 1)
	{
		if (scopeFlag & expandedScopeMask)
		{
			CActionScope *scope = m_scopes[i];
			{
				SScopeContext &scopeContext = m_scopeContexts[scope->GetContextID()];
				CActionController *pSlaveAC = scopeContext.enslavedController;

				if (!pSlaveAC)
				{
					float timeRemaining;
					SAnimBlend animBlend;
					EPriorityComparison priorityComp = Higher;
					const bool isRequeue = (&action == scope->m_pAction);
					const IActionPtr pCompareAction = scope->m_pAction ? scope->m_pAction : scope->m_pExitingAction;
					if (pCompareAction)
					{
						if (isRequeue && (&action.GetRootScope() != scope))
						{
							//--- If this is a requeued action, only use the primary scope to time transitions
							priorityComp = Higher;
						}
						else if (&pCompareAction->GetRootScope() != scope)
						{
							//--- Only test the action on its root scope, as that is timing the action
							continue;
						}
						else
						{
							priorityComp = action.DoComparePriority(*pCompareAction);
						}

						expandedScopeMask &= ~pCompareAction->GetInstalledScopeMask();
					}

					if (!scope->CanInstall(priorityComp, action.GetFragmentID(), fragTagState, isRequeue, timeRemaining))
					{
						return false;
					}

					timeTillInstall = max(timeTillInstall, timeRemaining);
				}
				else
				{
					//--- Ensure that we time our transitions based on the target too

					uint32 fragCRC = m_context.controllerDef.m_fragmentIDs.GetTagCRC(action.GetFragmentID());
					FragmentID tgtFragID = pSlaveAC->GetContext().controllerDef.m_fragmentIDs.Find(fragCRC);
					SFragTagState childFragTagState;
					childFragTagState.globalTags = pSlaveAC->GetContext().state.GetMask();
					childFragTagState.fragmentTags = action.GetFragTagState();
					ActionScopes childScopeMask = pSlaveAC->GetContext().controllerDef.GetScopeMask(tgtFragID, childFragTagState);
					float tgtTimeTillInstall;
					if (!pSlaveAC->CanInstall(action, TAG_ID_INVALID, childScopeMask, timeStep, tgtTimeTillInstall))
					{
						return false;
					}

					timeTillInstall = max(timeTillInstall, tgtTimeTillInstall);
				}
			}
		}
	}

	return (timeStep >= timeTillInstall);
}


bool CActionController::CanInstall(const IAction &action, const ActionScopes &scopeMask, float timeStep, float &timeTillInstall) const
{
	return CanInstall(action, action.GetSubContext(), scopeMask, timeStep, timeTillInstall);
}

bool CActionController::CanInstallNow(int priority, ActionScopes scopeMask) const
{
	//--- Something queued of higher priority?
	for (uint32 i=0; i<m_queuedActions.size(); i++)
	{
		const IAction* const pAction = m_queuedActions[i].get();
		if (((scopeMask & pAction->GetInstalledScopeMask()) != 0)
			&& (priority <= pAction->GetPriority()))
		{
			return false;
		}
	}

	//--- Check for straight install/displacing current actions or blending after
	for (uint32 i=0, scopeFlag = 1; i<m_scopes.size(); i++, scopeFlag <<= 1)
	{
		if (scopeMask & scopeFlag)
		{
			const IAction* const pAction = m_scopes[i]->m_pAction.get();
			if (pAction && (priority <= pAction->GetPriority()))
			{
				return false;
			}
		}
	}

	return true;
}

bool CActionController::IsDifferent(const FragmentID fragID, const TagState &fragmentTags, const ActionScopes &scopeMask) const
{
	const uint32 numScopes  = GetTotalScopes();
	const ActionScopes mask = GetActiveScopeMask() & scopeMask;

	uint32 installedContexts = 0;
	for (uint32 i=0; i<numScopes; i++)
	{
		if ((1<<i) & mask)
		{
			const CActionScope *scope = m_scopes[i];
			if (scope && scope->NeedsInstall(installedContexts))
			{
				installedContexts |= scope->GetContextMask();
				if (scope->IsDifferent(fragID, fragmentTags))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CActionController::RequestInstall(const IAction &action, const ActionScopes &scopeMask)
{
	//--- Ensure we test against all effected scopes
	ActionScopes expandedScopeMask = ExpandOverlappingScopes(scopeMask);
	expandedScopeMask &= m_activeScopes;

	uint32 contextIDs = 0;
	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		if ((1<<i) & expandedScopeMask)
		{
			CActionScope *scope = m_scopes[i];
			if (scope->NeedsInstall(contextIDs))
			{
				contextIDs |= scope->GetContextMask();
				IAction* const pScopeAction = scope->m_pAction.get();
				if (pScopeAction && (&pScopeAction->GetRootScope() == scope))
				{
					EPriorityComparison priorityComp = action.DoComparePriority(*pScopeAction);
					pScopeAction->OnRequestBlendOut(priorityComp);
				}
			}
		}
	}
}

void CActionController::InsertEndingAction(const IActionPtr& pAction)
{
	const uint32 numActions = m_endedActions.size();
	for (uint32 i=0; i<numActions; i++)
	{
		if (m_endedActions[i] == pAction)
		{
			return;
		}
	}
	m_endedActions.push_back(pAction);
}

void CActionController::Install(IAction &action, float timeRemaining)
{
	const FragmentID fragmentID = action.GetFragmentID();
	const bool isReinstall = (action.GetStatus() == IAction::Installed);
	const TagID subContext = action.GetSubContext();
	uint32 optionIdx = action.GetOptionIdx();

	if (optionIdx == OPTION_IDX_RANDOM)
	{
		optionIdx = m_context.randGenerator.GenerateUint32();
	}

	action.BeginInstalling();
	if (!isReinstall)
	{
		action.Install();
	}

	//--- Setup scope mask
	ActionScopes scopeMask = action.GetForcedScopeMask() | QueryScopeMask(action.GetFragmentID(), action.GetFragTagState(), action.GetSubContext());
	ActionScopes filteredScope = scopeMask & m_activeScopes;
	SFragTagState tagState = SFragTagState(m_context.state.GetMask(), action.GetFragTagState());

	uint32 overlappedScopes = ExpandOverlappingScopes(filteredScope);
	m_scopeFlushMask |= overlappedScopes;

	RecordTagState();
	Record(SMannHistoryItem(action.GetForcedScopeMask(), fragmentID, tagState.fragmentTags, optionIdx, true));

	if (subContext != TAG_ID_INVALID)
	{
		const SSubContext subContextDef = m_context.controllerDef.m_subContext[subContext];
		tagState.globalTags = m_context.controllerDef.m_tags.GetUnion(tagState.globalTags, m_context.subStates[subContext].GetMask());
		tagState.globalTags = m_context.controllerDef.m_tags.GetUnion(tagState.globalTags, subContextDef.additionalTags);
	}

	action.m_installedScopeMask = filteredScope;
	m_scopeFlushMask &= ~filteredScope;

	//--- Now install action into scopes & animations on all other contexts
	uint32 rootScope = SCOPE_ID_INVALID;
	uint32 installedContexts = 0;
	bool isOneShot = (fragmentID != FRAGMENT_ID_INVALID);
	for (uint32 i=0, scopeFlag=1; i<m_scopes.size(); i++, scopeFlag <<= 1)
	{
		CActionScope *scope = m_scopes[i];
		IActionPtr pExitingAction = scope->m_pAction;
		bool exitCurrentAction = false;

		if (scopeFlag & filteredScope)
		{
			bool higherPriority = true;
			if (pExitingAction)
			{
				higherPriority = (action.DoComparePriority(*pExitingAction) == Higher);
			}

			scope->Install(action); // do this before QueueFragment so QueueFragment can call OnAnimInstalled on the action

			//--- Flush any previously existing exiting action, as this has already been overridden
			if (scope->m_pExitingAction)
			{
				InsertEndingAction(scope->m_pExitingAction);
				scope->m_pExitingAction->m_flags &= ~IAction::TransitioningOut;
				scope->m_pExitingAction.reset();
			}

			SScopeContext &scopeContext = m_scopeContexts[scope->GetContextID()];
			CActionController *pSlaveAC = scopeContext.enslavedController;

			const bool rootContextInstallation = scope->NeedsInstall(installedContexts);
			const bool waitingOnRootScope = (rootScope == SCOPE_ID_INVALID);

			if (pSlaveAC != NULL)
			{
				//--- Ensure that our slave tags are up to date
				SynchTagsToSlave(scopeContext, true);

				uint32 fragCRC = m_context.controllerDef.m_fragmentIDs.GetTagCRC(fragmentID);
				FragmentID slaveFragID = pSlaveAC->GetContext().controllerDef.m_fragmentIDs.Find(fragCRC);

				if (slaveFragID != FRAGMENT_ID_INVALID)
				{
					IActionPtr pDummyAction = action.CreateSlaveAction(slaveFragID, tagState.fragmentTags, pSlaveAC->GetContext());
					pDummyAction->m_mannequinParams = action.m_mannequinParams;
					pDummyAction->Initialise(pSlaveAC->m_context);
					pDummyAction->m_optionIdx = optionIdx;
					pSlaveAC->Install(*pDummyAction, timeRemaining);

					if (pDummyAction->GetInstalledScopeMask() != ACTION_SCOPES_NONE)
					{
						pSlaveAC->m_triggeredActions.push_back(std::make_pair(pDummyAction, false));
						action.m_slaveActions.push_back(pDummyAction);

						pSlaveAC->ResolveActionStates();

						//--- Copy the timing settings back from the target
						CActionScope *pTargetScope = (CActionScope *)&pDummyAction->GetRootScope();
						scope->ClearSequencers();
						scope->m_lastFragmentID						= fragmentID;
						scope->m_fragmentDuration					= pTargetScope->m_fragmentDuration;
						scope->m_transitionDuration				= pTargetScope->m_transitionDuration;
						scope->m_transitionOutroDuration	= pTargetScope->m_transitionOutroDuration;
						scope->m_fragmentTime							= pTargetScope->m_fragmentTime;
						scope->m_sequenceFlags						= pTargetScope->m_sequenceFlags;
						scope->QueueAnimFromSequence(0, 0, false);
					}
				}
			}
			else if (!isReinstall || scope->m_isOneShot || scope->IsDifferent(action.m_fragmentID, action.m_fragTags, action.m_subContext))
			{
				if (scope->QueueFragment(fragmentID, tagState, optionIdx, timeRemaining, action.GetUserToken(), waitingOnRootScope, higherPriority, rootContextInstallation))
				{
					if (scope->HasOutroTransition() && pExitingAction)
					{
						pExitingAction->TransitionOutStarted();
					}
				}
			}

			if (waitingOnRootScope && scope->HasFragment())
			{
				float rootStartTime = scope->GetFragmentStartTime();
				timeRemaining = rootStartTime;
				rootScope = i;

				isOneShot = scope->m_isOneShot;
			}

			installedContexts |= scope->GetContextMask();


			exitCurrentAction =  (pExitingAction && (pExitingAction->m_rootScope == scope));
		}
		else
		{
			exitCurrentAction =  (pExitingAction && ((scopeFlag & overlappedScopes) != 0));
		}

		if (exitCurrentAction && (pExitingAction != &action))
		{
			InsertEndingAction(pExitingAction);
			pExitingAction->m_eStatus = IAction::Exiting;
		}
	}

	if (isOneShot)
	{
		action.m_flags |= IAction::FragmentIsOneShot;
	}
	else
	{
		action.m_flags &= ~IAction::FragmentIsOneShot;
	}

	if (rootScope == SCOPE_ID_INVALID)
	{
		rootScope = GetLeastSignificantBit(filteredScope);
	}
	action.m_rootScope = m_scopes[rootScope];
}

bool CActionController::TryInstalling(IAction &action, float timePassed)
{
	const FragmentID fragmentID = action.GetFragmentID();

	const TagID subContext = action.GetSubContext();
	ActionScopes scopeMask = action.GetForcedScopeMask() | QueryScopeMask(fragmentID, action.GetFragTagState(), subContext);
	ActionScopes filteredScope = scopeMask & m_activeScopes;

	//--- Request Install
	RequestInstall(action, filteredScope);

	//--- Can I install now?
	float timeRemaining;
	if (CanInstall(action, filteredScope, timePassed, timeRemaining))
	{
		Install(action, timeRemaining);

		return true;
	}

	return false;
}

bool CActionController::QueryDuration(IAction &action, float &fragmentDuration, float &transitionDuration) const
{
	const TagID subContext = action.GetSubContext();
	ActionScopes scopeMask = action.GetForcedScopeMask() | QueryScopeMask(action.GetFragmentID(), action.GetFragTagState(), subContext);
	scopeMask = scopeMask & m_activeScopes;
	uint32 optionIdx = action.GetOptionIdx();
	if (optionIdx == OPTION_IDX_RANDOM)
	{
		optionIdx = m_context.randGenerator.GenerateUint32();
		action.SetOptionIdx(optionIdx);
	}

	SFragTagState tagState = SFragTagState(m_context.state.GetMask(), action.GetFragTagState());
	if (subContext != TAG_ID_INVALID)
	{
		const SSubContext subContextDef = m_context.controllerDef.m_subContext[subContext];
		tagState.globalTags = m_context.controllerDef.m_tags.GetUnion(tagState.globalTags, m_context.subStates[subContext].GetMask());
		tagState.globalTags = m_context.controllerDef.m_tags.GetUnion(tagState.globalTags, subContextDef.additionalTags);
	}

	for (uint32 i=0, scopeFlag=1; i<m_scopes.size(); i++, scopeFlag <<= 1)
	{
		if (scopeFlag & scopeMask)
		{
			CActionScope *pScope = m_scopes[i];

			bool higherPriority = true;
			if (pScope->m_pAction)
			{
				higherPriority = (action.DoComparePriority(*pScope->m_pAction) == Higher);
			}

			SBlendQuery query;
			pScope->FillBlendQuery(query, action.GetFragmentID(), tagState, higherPriority, NULL);

			const SScopeContext &scopeContext = m_scopeContexts[pScope->GetContextID()];
			SFragmentData fragData;
			IAnimationSet* pAnimSet = scopeContext.charInst ? scopeContext.charInst->GetIAnimationSet() : NULL;
			bool hasFound = (scopeContext.database->Query(fragData, query, optionIdx, pAnimSet) & eSF_Fragment) != 0;

			if (hasFound)
			{
				fragmentDuration = transitionDuration = 0.0f;
				for (uint32 p=0; p<SFragmentData::PART_TOTAL; p++)
				{
					if (fragData.transitionType[p] == eCT_Normal)
					{
						fragmentDuration += fragData.duration[p];
					}
					else
					{
						transitionDuration += fragData.duration[p];
					}
				}

				return true;
			}
		}
	}

	return false;
}

void CActionController::ReleaseScopes()
{
	uint32 scopeCount = m_scopes.size();
	for (uint32 i=0; i<scopeCount; i++)
	{
		CActionScope *scope = m_scopes[i];
		if (scope->m_pAction && (&scope->m_pAction->GetRootScope() == scope) && scope->m_pAction->IsStarted())
		{
			scope->m_pAction->Exit();
		}
	}

	for (uint32 i=0; i<scopeCount; ++i)
	{
		CActionScope* const pScope = m_scopes[i];
		pScope->Flush(FM_Normal);
	}

	for (uint32 i=0; i<scopeCount; ++i)
	{
		CActionScope* const pScope = m_scopes[i];
		assert(pScope);
		PREFAST_ASSUME(pScope);

		IAction* const pScopeAction = pScope->m_pAction.get();
		if (pScopeAction && pScopeAction->m_rootScope == pScope)
		{
			pScopeAction->m_eStatus = IAction::None;
			pScopeAction->m_flags &= ~IAction::Requeued;

			pScopeAction->m_rootScope = NULL;
		}
		pScope->m_pAction = NULL;
	}

	for (uint32 i=0; i<scopeCount; ++i)
	{
		CActionScope *scope = m_scopes[i];
		delete scope;
	}

	for (uint32 i=0; i<m_queuedActions.size(); ++i)
	{
		m_queuedActions[i]->m_eStatus = IAction::None;
	}

	m_scopes.clear();
	m_queuedActions.clear();
	m_activeScopes = 0;
}

void CActionController::ReleaseScopeContexts()
{
	//--- Remove ourselves from any enslaved characters
	const uint32 numContexts = m_context.controllerDef.m_scopeContexts.GetNum();
	for (uint32 i=0; i<numContexts; i++)
	{
		SScopeContext &scopeContext  = m_scopeContexts[i];
		if (scopeContext.enslavedController)
		{
			SynchTagsToSlave(scopeContext, false);
			scopeContext.enslavedController->UnregisterOwner(*this);
		}
	}
	delete [] m_scopeContexts;
	m_scopeContexts = NULL;
}

void CActionController::FlushScope(uint32 scopeID, ActionScopes scopeFlag, EFlushMethod flushMethod)
{
	CActionScope *scope = m_scopes[scopeID];

	if (scope->m_pAction && (&scope->m_pAction->GetRootScope() == scope))
	{
		EndActionsOnScope(scopeFlag, NULL, true, flushMethod);
	}
	scope->m_pAction = NULL;

	scope->Flush(flushMethod);
}

void CActionController::SetScopeContext(uint32 scopeContextID, IEntity &entity, ICharacterInstance *pCharacter, const IAnimationDatabase *animDatabase)
{
	if (scopeContextID >= (uint32)m_context.controllerDef.m_scopeContexts.GetNum())
	{
		CryFatalError("[SetScopeContext] Invalid scope context id %d used for %s", scopeContextID, m_cachedEntity->GetName());
	}

	SScopeContext &scopeContext = m_scopeContexts[scopeContextID];

	if ((pCharacter != scopeContext.charInst) || (animDatabase != scopeContext.database))
	{
		if (scopeContext.enslavedController)
		{
			SynchTagsToSlave(scopeContext, false);
			scopeContext.enslavedController->UnregisterOwner(*this);
		}
		
		if(scopeContext.charInst)
		{
			scopeContext.charInst->SetPlaybackScale(1.f);
		}

		// CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_INFO, "***SetScopeContext %2d on entity %s (id=%d; %p) to entity=%s (id=%8d; %p), char=%x", scopeContextID, GetSafeEntityName(), m_entityId, m_cachedEntity, entity.GetName(), entity.GetId(), &entity, pCharacter);

		// Flush all scopes for the specified scopeContext
		ActionScopes scopesUsingContext = FlushScopesByScopeContext(scopeContextID);

		// Fill context data
		scopeContext.charInst = pCharacter;
		scopeContext.database = (CAnimationDatabase*)animDatabase;
		scopeContext.entityId = entity.GetId();
		scopeContext.cachedEntity = &entity;
		scopeContext.enslavedController = NULL;
		
		if(pCharacter)
		{
			pCharacter->SetPlaybackScale(GetTimeScale());
		}

		// If the new context is valid, activate its scopes
		bool scopeContextIsValid = UpdateScopeContextValidity(scopeContextID);
		CRY_ASSERT(scopeContextIsValid);
		if (scopeContextIsValid && animDatabase)
			m_activeScopes |= scopesUsingContext;
		else
			m_activeScopes &= ~scopesUsingContext;
	}
}

void CActionController::ClearScopeContext(uint32 scopeContextID, bool flushAnimations)
{
	if (scopeContextID >= (uint32)m_context.controllerDef.m_scopeContexts.GetNum())
	{
		CryFatalError("[ClearScopeContext] Invalid scope context id %d used for %s", scopeContextID, m_cachedEntity->GetName());
	}

	//-- Flush all scopes that use this scopecontext
	ActionScopes scopesUsingContext = FlushScopesByScopeContext(scopeContextID, flushAnimations ? FM_Normal : FM_NormalLeaveAnimations);

	SScopeContext &scopeContext = m_scopeContexts[scopeContextID];

	if (scopeContext.enslavedController)
	{
		SynchTagsToSlave(scopeContext, false);
		scopeContext.enslavedController->UnregisterOwner(*this);
	}

	if(scopeContext.charInst)
	{
		scopeContext.charInst->SetPlaybackScale(1.f);
	}

	//--- Clear context data
	scopeContext.charInst = NULL;
	scopeContext.database = NULL;
	scopeContext.entityId   = 0;
	scopeContext.cachedEntity = NULL;
	scopeContext.enslavedController = NULL;

	//--- Disable scopes that use this context
	m_activeScopes &= ~scopesUsingContext;
}
	
void CActionController::GetScopeContext(uint32 scopeContextID, SScopeContextQuery &query) const
{
	const uint32 numContexts = m_context.controllerDef.m_scopeContexts.GetNum();

	CRY_ASSERT_MESSAGE((scopeContextID < numContexts), "Invalid scope context id");

	if (scopeContextID < numContexts)
	{
		query.entityId  = m_scopeContexts[scopeContextID].entityId;
		query.pCharInst = m_scopeContexts[scopeContextID].charInst;
		query.pDatabase = m_scopeContexts[scopeContextID].database;
	}
	else
	{
		query.entityId  = 0;
		query.pCharInst = NULL;
		query.pDatabase = NULL;
	}
}

IScope *CActionController::GetScope(uint32 scopeID)
{
	CRY_ASSERT_MESSAGE((scopeID < m_scopes.size()), "Invalid scope id");

	if (scopeID < m_scopes.size())
	{
		return m_scopes[scopeID];
	}

	return NULL;
}

const IScope *CActionController::GetScope(uint32 scopeID) const
{
	CRY_ASSERT_MESSAGE((scopeID < m_scopes.size()), "Invalid scope id");

	if (scopeID < m_scopes.size())
	{
		return m_scopes[scopeID];
	}

	return NULL;
}

#ifdef _DEBUG
void CActionController::ValidateActions()
{
	const uint32 numScopes = m_scopes.size();
	for (uint32 i=0, scopeFlag=1; i<numScopes; i++, scopeFlag <<= 1)
	{
		CActionScope *pScope = m_scopes[i];

		if (pScope->m_pAction)
		{
			CRY_ASSERT(pScope->m_pAction->m_installedScopeMask & scopeFlag);
		}
	}
}
#endif //_DEBUG

void CActionController::ResolveActionStates()
{
	const uint32 numScopes = m_scopes.size();
	m_scopeFlushMask &= m_activeScopes;
	for (uint32 i=0, scopeFlag=1; i<numScopes; i++, scopeFlag <<= 1)
	{
		if (scopeFlag & m_scopeFlushMask)
		{
			CActionScope &scope = *m_scopes[i];
			scope.BlendOutFragments();
			scope.m_pAction = NULL;
		}
	}
	m_scopeFlushMask = 0;

	//--- Now delete dead actions
	for (uint32 i=0; i<m_endedActions.size(); i++)
	{
		IAction *action = m_endedActions[i];

		if (action->IsTransitioningOut())
		{
			CActionScope *pActionScope = (CActionScope *)&action->GetRootScope();
			pActionScope->m_pExitingAction = action;
		}
		else
		{
			EndAction(action);
		}
	}
	m_endedActions.clear();

#ifdef _DEBUG
	ValidateActions();
#endif //_DEBUG

	//--- Now install actions
	for (uint32 i=0; i<m_triggeredActions.size(); i++)
	{
		IActionPtr action = m_triggeredActions[i].first;
		const bool isReinstall = m_triggeredActions[i].second;

		for (TActionList::iterator iter = m_queuedActions.begin(); iter != m_queuedActions.end(); ++iter)
		{
			if (*iter == action)
			{
				m_queuedActions.erase(iter);
				break;
			}
		}

		if (action->GetStatus() == IAction::Installed)
		{
			CActionScope &rootScope = (CActionScope&) action->GetRootScope();
			if (isReinstall)
			{
				//--- Reinstallation? -> remove queue's additional reference and wipe flag
				action->m_flags &= ~(IAction::Requeued);
			}
			else if (!rootScope.HasOutroTransition())
			{
				action->Enter();
			}
		}

		action->EndInstalling();
	}
	m_triggeredActions.clear();
}

bool CActionController::ResolveActionInstallations(float timePassed)
{
	bool changed = false;

	ActionScopes scopeMaskAcc = 0;

	const uint32 numScopes = m_scopes.size();
	for (uint32 i=0; i<numScopes; i++)
	{
		IActionPtr pScopeAction = m_scopes[i]->GetPlayingAction();
		if (pScopeAction && (pScopeAction->m_rootScope == m_scopes[i]))
		{
			pScopeAction->OnResolveActionInstallations();
		}
	}
	
	const uint32 numActions = m_queuedActions.size();

	CRY_ASSERT(s_actionList.empty());
	s_actionList = m_queuedActions;
	for (uint32 i=0; i<numActions; i++)
	{
		IAction &action = *s_actionList[i];

		ActionScopes scopeMask = action.GetForcedScopeMask() | QueryScopeMask(action.GetFragmentID(), action.GetFragTagState(), action.GetSubContext());
		ActionScopes scopeMaskFiltered = scopeMask & m_activeScopes;
		uint32 rootScope = GetLeastSignificantBit(scopeMaskFiltered);
		action.m_rootScope = m_scopes[rootScope];
		const bool isRequeue = ((action.m_flags & IAction::Requeued) != 0);

		if (!isRequeue && !stl::find(s_tickedActions, &action))
		{
			action.UpdatePending(timePassed);
			s_tickedActions.push_back(&action);
		}

		IAction::EStatus status = action.GetStatus();

		if ((status == IAction::Finished)	|| (action.m_flags & IAction::Stopping) || ((status != IAction::Installed) && isRequeue))
		{
			//--- Remove from queue & clear requeued flag
			for (TActionList::iterator iter = m_queuedActions.begin(); iter != m_queuedActions.end(); ++iter)
			{
				if (*iter == &action)
				{
					m_queuedActions.erase(iter);
					break;
				}
			}
			action.m_flags &= ~IAction::Requeued;
		}
		else
		{
			if (scopeMaskFiltered)
			{
				const bool canTrigger = (scopeMaskAcc & scopeMaskFiltered) == 0;
				const bool isReinstall = (action.GetStatus() == IAction::Installed);

				scopeMaskAcc |= scopeMaskFiltered;

				if (canTrigger && TryInstalling(action, timePassed))
				{
					m_triggeredActions.push_back(std::make_pair(&action, isReinstall));
					changed = true;
				}
			}
		}
	}
	s_actionList.clear();

	ResolveActionStates();

	if (BlendOffActions(timePassed))
	{
		changed = true;
	}

	return changed;
}

bool CActionController::BlendOffActions(float timePassed)
{
	bool hasEndedAction = false;

	//--- Check for any already finished actions and flush them here
	const uint32 numScopes = m_scopes.size();
	for (uint32 i=0; i<numScopes; i++)
	{
		CActionScope *pRootScope = m_scopes[i];
		IActionPtr pExitingAction = pRootScope->m_pAction;

		if (pExitingAction 
			&& (&pExitingAction->GetRootScope() == pRootScope) 
			&& (pRootScope->GetFragmentTime() > 0.0f) 
			&& ((pExitingAction->m_flags & IAction::TransitionPending) == 0))	// No blend off if there is a fragment we are waiting to blend to already
		{
			float timeLeft = 0.0f;
			EPriorityComparison priority = (pExitingAction->GetStatus() == IAction::Finished) ? Higher : Lower;
			if (pRootScope->CanInstall(priority, FRAGMENT_ID_INVALID, SFragTagState(), false, timeLeft) && (timePassed >= timeLeft))
			{
				const ActionScopes installedScopes = pExitingAction->GetInstalledScopeMask() & m_activeScopes;
				for (uint32 s=0, scopeFlag=1; s<numScopes; s++, scopeFlag<<=1)
				{
					if ((scopeFlag & installedScopes) != 0)
					{
						CActionScope &scope = *m_scopes[s];

						scope.QueueFragment(FRAGMENT_ID_INVALID, SFragTagState(m_context.state.GetMask()), OPTION_IDX_RANDOM, 0.0f, 0, priority == Higher);
						scope.m_pAction = NULL;
					}
				}

				if (pRootScope->HasOutroTransition())
				{
					pRootScope->m_pExitingAction = pExitingAction;
					pExitingAction->TransitionOutStarted();
				}
				else
				{
					EndAction(pExitingAction);
				}

				hasEndedAction = true;
			}
		}
	}

#ifdef _DEBUG
	ValidateActions();
#endif //_DEBUG

	return hasEndedAction;
}

void CActionController::PruneQueue()
{
	int numTooManyActions = m_queuedActions.size() - MAX_ALLOWED_QUEUE_SIZE;
	while (numTooManyActions > 0)
	{
		bool removedAction = false;
		// Remove from the back of the list
		for (int i = (int)m_queuedActions.size() - 1; i >= 0; --i)
		{
			IActionPtr pOtherAction = m_queuedActions[i];
			const bool isInterruptable = ((pOtherAction->GetFlags() & IAction::Interruptable) != 0);
			if (!isInterruptable)
			{
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Error, attempting to queue too many actions on entity '%s'. Ditching other action '%s' (%p) (at queue index %d)", GetSafeEntityName(), pOtherAction->GetName(), pOtherAction.get(), i);
				m_queuedActions.erase(m_queuedActions.begin() + i);
				pOtherAction->Fail(AF_QueueFull);
				numTooManyActions--;
				removedAction = true;
				break;
			}
		}
		// Can't remove anything - so just bail out
		if (removedAction == false)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Error, attempting to queue too many actions on entity '%s'. Can't remove anything", GetSafeEntityName());
			break;
		}
	}
}

void CActionController::Update(float timePassed)
{
	if ((m_flags & AC_PausedUpdate) == 0)
	{
		timePassed *= GetTimeScale();

		SetFlag(AC_IsInUpdate, true);
		UpdateValidity();

		RecordTagState();

		const uint32 numScopes = m_scopes.size();
		for (uint32 i=0; i<numScopes; i++)
		{
			CActionScope &scope = *m_scopes[i];
			IActionPtr pScopeAction = scope.GetPlayingAction();
			if (pScopeAction && (pScopeAction->m_rootScope == &scope))
			{
				//--- Insert user data into the parameter system
				ICharacterInstance *pInst = scope.GetCharInst();
				if (pInst)
				{
					for (uint32 ch=0; ch<MANN_NUMBER_BLEND_CHANNELS; ch++)
					{
						pScopeAction->SetParam(s_blendChannelCRCs[ch], pInst->GetISkeletonAnim()->GetUserData(ch));
					}
				}

				pScopeAction->Update(timePassed * pScopeAction->GetSpeedBias());
				//--- Reset the pending transition flag
				//--- If the transition still exists it'll be reset in the blend code
				pScopeAction->m_flags &= ~IAction::TransitionPending;
			}
		}

		CRY_ASSERT(s_tickedActions.empty());
		uint32 numIts;
		const uint32 MAX_ITERATIONS = 5;
		for (numIts = 0; numIts < MAX_ITERATIONS; numIts++)
		{
			if (!ResolveActionInstallations(timePassed))
				break;
		}
		s_tickedActions.clear();

		if (numIts == MAX_ITERATIONS)
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Error, cannot resolve action changes in %d iterations for '%s'", MAX_ITERATIONS, GetSafeEntityName());
		}

		PruneQueue();

		//--- Update scope sequencers
		for (uint32 i=0; i<m_scopes.size(); i++)
		{
			if ((1<<i) & m_activeScopes)
			{
				CActionScope *scope = m_scopes[i];
				scope->Update(timePassed);
			}
		}

		uint32 numProcContexts = m_procContexts.size();
		for (uint32 i=0; i<numProcContexts; i++)
		{
			m_procContexts[i].pContext->Update(timePassed);
		}
		SetFlag(AC_IsInUpdate, false);
	}

#ifndef _RELEASE
	if (m_flags & AC_DumpState)
	{
		stack_string filename;
		BuildFilename(filename, GetSafeEntityName());
		DumpHistory(filename.c_str(), -1.0f);

		m_flags &= ~AC_DumpState;
	}

	if (m_flags & AC_DebugDraw)
	{
		DebugDraw();
	}
#endif //!_RELEASE
}

void CActionController::SetSlaveController(IActionController *target, uint32 targetContext, bool enslave, const IAnimationDatabase* piOptionTargetDatabase)
{
	const bool doFullEnslavement = !piOptionTargetDatabase;

	UpdateValidity();

	CActionController *targetController = (CActionController *)target;
	SScopeContext &scopeContext = m_scopeContexts[targetContext];


	if (enslave)
	{
		targetController->UpdateValidity();
		CRY_ASSERT(targetController->GetEntityId());
		
		if (scopeContext.enslavedController)
		{
			SynchTagsToSlave(scopeContext, false);
			scopeContext.enslavedController->UnregisterOwner(*this);
			scopeContext.enslavedController = NULL;
		}
		
		if (targetController->GetEntityId())
		{
			IEntity &targetEnt = targetController->GetEntity();

			if (doFullEnslavement)
			{
				SetScopeContext(targetContext, targetEnt, NULL, NULL);

				targetController->RegisterOwner(*this);

				//--- Enable all associated scopes
				const int numScopes = m_scopes.size();
				for (size_t i = 0, scopeFlag = 1; i < numScopes; i++, scopeFlag <<= 1)
				{
					CActionScope *scope = m_scopes[i];
					if (scope->GetContextID() == targetContext)
					{
						m_activeScopes |= scopeFlag;
					}
				}

				scopeContext.enslavedController = targetController;

				const SControllerDef &controllerDef = GetContext().controllerDef;
				const CTagDefinition &tagDef			= controllerDef.m_tags;
				const CTagDefinition &tagDefSlave = targetController->GetContext().controllerDef.m_tags;
				const SScopeContextDef &scopeContextDef = controllerDef.m_scopeContextDef[targetContext];
				scopeContext.sharedTags = tagDef.GetIntersection(tagDef.GetSharedTags(tagDefSlave), scopeContextDef.sharedTags);
				scopeContext.sharedTags = tagDef.GetUnion(scopeContext.sharedTags, scopeContextDef.additionalTags);
				scopeContext.setTags		= TAG_STATE_EMPTY;

				SynchTagsToSlave(scopeContext, true);
			}
			else
			{
				//--- Hookup all the scopes
				ActionScopes targetScopeMask = targetController->GetActiveScopeMask();
				uint32 rootScope = GetLeastSignificantBit(targetScopeMask);
				CActionScope *targetScope = targetController->m_scopes[rootScope];
				SetScopeContext(targetContext, targetEnt, targetScope->GetCharInst(), piOptionTargetDatabase ? piOptionTargetDatabase : &targetScope->GetDatabase());

				ActionScopes clearMask = targetController->m_activeScopes & ~(1<<rootScope);

				for (uint32 i=0, scopeMask=1; i<targetController->m_scopes.size(); i++, scopeMask <<= 1)
				{
					if (scopeMask & clearMask)
					{
						CActionScope *scope = targetController->m_scopes[i];
						scope->BlendOutFragments();
						scope->UpdateSequencers(0.0f);
					}
				}
				targetController->EndActionsOnScope(targetScopeMask, NULL);
			}
		}
	}
	else
	{
		ClearScopeContext(targetContext);
	}

	if (!doFullEnslavement)
	{
		targetController->SetFlag(AC_PausedUpdate, enslave);
	}
}

void CActionController::FlushSlaveController(IActionController *target)
{
	const uint32 numScopeContexts = m_context.controllerDef.m_scopeContexts.GetNum();
	for (uint32 i=0; i<numScopeContexts; i++)
	{
		SScopeContext &scopeContext = m_scopeContexts[i];
		if (scopeContext.enslavedController == target)
		{
			scopeContext.charInst = NULL;
			scopeContext.database = NULL;
			scopeContext.entityId   = 0;
			scopeContext.cachedEntity = NULL;
			scopeContext.enslavedController = NULL;

			//--- Disable scopes that use this context
			const uint32 numScopes = m_scopes.size();
			for (size_t s = 0, scopeFlag = 1; s < numScopes; s++, scopeFlag <<= 1)
			{
				CActionScope *scope = m_scopes[s];
				if (scope->GetContextID() == i)
				{
					m_activeScopes &= ~scopeFlag;
					if (scope->m_pAction)
					{
						scope->m_pAction->m_installedScopeMask &= ~scopeFlag;
						scope->m_pAction = NULL;
					}
				}
			}
		}
	}
}

void CActionController::SynchTagsToSlave(SScopeContext &scopeContext, bool enable)
{
	CActionController *targetController = scopeContext.enslavedController;

	if (targetController)
	{
		const CTagDefinition &tagDefSlave = targetController->GetContext().controllerDef.m_tags;
		CTagState &targetTagState = targetController->GetContext().state;

		//--- Clear previous setting
		for (uint32 i=0; i<tagDefSlave.GetNum(); i++)
		{
			if (tagDefSlave.IsSet(scopeContext.setTags, i))
			{
				targetTagState.Set(i, false);
			}
		}

		scopeContext.setTags = TAG_STATE_EMPTY;

		if (enable)
		{
			const CTagDefinition &tagDef			= GetContext().controllerDef.m_tags;
			TagState sourceTagState = GetContext().state.GetMask();
			sourceTagState = tagDef.GetUnion(sourceTagState, GetContext().controllerDef.m_scopeContextDef[scopeContext.id].additionalTags);

			for (uint32 i=0; i<tagDef.GetNum(); i++)
			{
				TagID groupID = tagDef.GetGroupID(i);
				const bool isKnown = ((groupID != TAG_ID_INVALID) && tagDef.IsGroupSet(scopeContext.sharedTags, groupID))
					|| tagDef.IsSet(scopeContext.sharedTags, i);
				if ( isKnown && (tagDef.IsSet(sourceTagState, i)))
				{
					int tagCRC = tagDef.GetTagCRC(i);
					TagID tagID = tagDefSlave.Find(tagCRC);
					targetTagState.Set(tagID, true);
					tagDefSlave.Set(scopeContext.setTags, tagID, true);
				}
			}
		}
	}
}

void CActionController::Reset()
{
	Flush();

	const uint32 numScopeContexts = (uint32)m_context.controllerDef.m_scopeContexts.GetNum();
	for (uint32 itContext = 0; itContext < numScopeContexts; ++itContext)
	{
		ClearScopeContext(itContext, false);
	}
}

void CActionController::Flush()
{
	if (m_flags & AC_IsInUpdate)
	{
		CryFatalError("Flushing the action controller in the middle of an update, this is bad!");
	}
	UpdateValidity();

	const uint32 numScopes = m_scopes.size();
	for (uint32 i=0, scopeFlag=1; i<numScopes; i++, scopeFlag <<= 1)
	{
		CActionScope &scope = *m_scopes[i];
		FlushScope(i, scopeFlag);
	}

	const uint32 numQueuedActions = m_queuedActions.size();
	for (uint32 i=0; i<numQueuedActions; i++)
	{
		IActionPtr pAction = m_queuedActions[i];
		CRY_ASSERT(pAction != NULL);

		if (pAction)
		{
			pAction->m_eStatus = IAction::None;
		}
	}
	m_queuedActions.clear();

	FlushProceduralContexts();
}

void CActionController::FlushProceduralContexts()
{
	uint32 numProcContexts = m_procContexts.size();
	for (uint32 i=0; i<numProcContexts; i++)
	{
		m_procContexts[i].pContext.reset();
	}
	m_procContexts.clear();
}

void CActionController::RegisterListener(IMannequinListener *listener)
{
	m_listeners.push_back(listener);
}

void CActionController::UnregisterListener(IMannequinListener *listener)
{
	const uint32 numListeners = m_listeners.size();
	for (uint32 i=0; i<numListeners; i++)
	{
		if (m_listeners[i] == listener)
		{
			m_listeners.erase(m_listeners.begin()+i);
			return;
		}
	}

	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Attempting to remove an unknown listener!");
}

void CActionController::EndAction(const IActionPtr& pAction, EFlushMethod flushMethod)
{
	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CRY_ASSERT(m_scopes[i]->m_pAction != pAction);
	}

	switch(flushMethod)
	{
	case FM_Normal:
	case FM_NormalLeaveAnimations:
		if (pAction->Interrupt())
		{
			pAction->Initialise(m_context);

			if (pAction->m_flags & IAction::Requeued)
			{
				pAction->m_flags &= ~IAction::Requeued;
			}
			else
			{
				PushOntoQueue(pAction);
			}
		}
		else
		{
			if (pAction->m_flags & IAction::Requeued)
			{
				for (TActionList::iterator iter = m_queuedActions.begin(); iter != m_queuedActions.end(); ++iter)
				{
					if (*iter == pAction)
					{
						m_queuedActions.erase(iter);
						break;
					}
				}
			}
		}
		break;
	case FM_Failure:
		pAction->Fail(AF_InvalidContext);
		break;
	default:
		CRY_ASSERT(false);
	}
}

void CActionController::StartAction(const IActionPtr& pAction)
{
	//if ((pAction->m_flags && IAction::Requeued) != 0)
	//{
	//	//--- Reinstallation? -> remove queue's additional reference and wipe flag
	//	pAction->m_flags &= ~(IAction::Requeued);
	//	pAction->Release();
	//}
	//else
	{
		pAction->Enter();
	}
}

ActionScopes CActionController::ExpandOverlappingScopes(ActionScopes scopeMask) const
{
	ActionScopes expandedScopeMask = scopeMask;
	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CActionScope &scope = *m_scopes[i];
		const IAction* const pScopeAction = scope.m_pAction.get();
		if (pScopeAction && (pScopeAction->m_eStatus != IAction::Exiting) && ((pScopeAction->GetInstalledScopeMask() & scopeMask) != 0))
		{
			expandedScopeMask |= pScopeAction->GetInstalledScopeMask();
		}
	}

	return expandedScopeMask;
}

ActionScopes CActionController::EndActionsOnScope(ActionScopes scopeMask, const IActionPtr &pPendingAction, bool blendOut, EFlushMethod flushMethod)
{
	TActionList deleteActionList;
	//--- Expand the scope mask to all overlapped scopes
	ActionScopes expandedScopeMask = ExpandOverlappingScopes(scopeMask);

	//--- Clean up scopes
	for (uint32 i=0, scopeFlag=1; i<m_scopes.size(); i++, scopeFlag<<=1)
	{
		CActionScope &scope = *m_scopes[i];
		if (scope.m_pAction && (scope.m_pAction->GetInstalledScopeMask() & expandedScopeMask))
		{
			if ((scope.m_pAction->m_rootScope == &scope) && (pPendingAction != scope.m_pAction))
			{
				deleteActionList.push_back(scope.m_pAction);
			}

			scope.m_pAction = NULL;

			if (blendOut && (scope.m_scopeContext.database != NULL))
			{
				scope.BlendOutFragments();
			}
		}
	}

	//--- Clean up actions
	for (uint32 i=0; i<deleteActionList.size(); i++)
	{
		EndAction(deleteActionList[i]);
	}

	return expandedScopeMask;
}


void CActionController::PushOntoQueue(const IActionPtr& action)
{
	const int priority = action->GetPriority();
	const bool requeued = ((action->GetFlags() & IAction::Requeued) != 0);

	for (TActionList::iterator iter = m_queuedActions.begin(); iter != m_queuedActions.end(); ++iter)
	{
		const int otherPriority = (*iter)->GetPriority();

		bool insertHere = false;
		if (otherPriority < priority)
		{
			insertHere = true;
		}
		else if (otherPriority == priority)
		{
			const bool otherRequeued = (((*iter)->GetFlags() & IAction::Requeued) != 0);

			if (requeued && !otherRequeued)
			{
				insertHere = true;
			}
		}

		if (insertHere)
		{
			m_queuedActions.insert(iter, action);
			return;
		}
	}
	m_queuedActions.push_back(action);
}

ActionScopes CActionController::QueryScopeMask(FragmentID fragID, const TagState &fragTags, const TagID subContext) const
{
	ActionScopes scopeMask = 0;

	SFragTagState fragTagState(m_context.state.GetMask(), fragTags);

	if (fragID != FRAGMENT_ID_INVALID)
	{
		uint32 numScopes = m_scopes.size();
		scopeMask = m_context.controllerDef.GetScopeMask(fragID, fragTagState);
	}

	if (subContext != TAG_ID_INVALID)
	{
		scopeMask |= m_context.controllerDef.m_subContext[subContext].scopeMask;
	}

	return scopeMask;
}

void CActionController::Queue(const IActionPtr &action, float time)
{
	action->m_queueTime = time;
	action->Initialise(m_context);

	PushOntoQueue(action);
}


void CActionController::Requeue(const IActionPtr& pAction)
{
	CRY_ASSERT(pAction->GetStatus() == IAction::Installed);

	PushOntoQueue(pAction);
}

void CActionController::OnEvent(const SGameObjectEvent& event)
{
	UpdateValidity();

	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CActionScope *scope = m_scopes[i];
		IAction* const pAction = scope->m_pAction.get();
		if (pAction && (&pAction->GetRootScope() == scope))
		{
			pAction->OnEvent(event);
		}
	}
}

void CActionController::OnAnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event)
{
	UpdateValidity();

	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CActionScope *scope = m_scopes[i];
		IAction * const pAction = scope->m_pAction.get();
		if (pAction && (&pAction->GetRootScope() == scope) && (pAction->GetStatus() == IAction::Installed))
		{
			pAction->OnAnimationEvent(pCharacter, event);
		}
	}
}

void CActionController::Record(const SMannHistoryItem &item)
{
	const uint32 numListeners = m_listeners.size();
#ifndef _RELEASE
	const float curTime = gEnv->pTimer->GetCurrTime();

	m_history[m_historySlot] = item;
	m_history[m_historySlot].time = curTime;
	m_historySlot = (m_historySlot+1)%TOTAL_HISTORY_SLOTS;
#endif //_RELEASE

	for (uint32 i=0; i<numListeners; i++)
	{
		m_listeners[i]->OnEvent(item, *this);
	}
}

void CActionController::RecordTagState()
{
	const TagState newTagState = m_context.state.GetMask();
	if (m_lastTagStateRecorded != newTagState)
	{
		SMannHistoryItem tagItem(newTagState);
		Record(tagItem);
		m_lastTagStateRecorded = newTagState;
	}
}

#ifndef _RELEASE

void CActionController::DumpHistory(const char *filename, float timeDelta) const
{
	const uint32 BUFFER_SIZE = 512;
	char szStringBuffer[BUFFER_SIZE];

	XmlNodeRef root = GetISystem()->CreateXmlNode("History");

	float endTime		= gEnv->pTimer->GetCurrTime();
	float startTime = (timeDelta > 0.0f) ? endTime - timeDelta : 0.0f;
	root->setAttr("StartTime", startTime);
	root->setAttr("EndTime", endTime);

	for (uint32 i=0; i<TOTAL_HISTORY_SLOTS; i++)
	{
		uint32 slotID = (i+m_historySlot+1)%TOTAL_HISTORY_SLOTS;
		const SMannHistoryItem &item = m_history[slotID];

		if (item.time >= startTime)
		{
			//--- Save slot out
			XmlNodeRef event = GetISystem()->CreateXmlNode("Item");
			event->setAttr("Time", item.time);
			switch (item.type)
			{
			case SMannHistoryItem::Fragment:
				if (item.fragment != FRAGMENT_ID_INVALID)
				{
					event->setAttr("FragmentID", m_context.controllerDef.m_fragmentIDs.GetTagName(item.fragment));
					const CTagDefinition *tagDef = m_context.controllerDef.GetFragmentTagDef(item.fragment);
					if ((item.tagState != TAG_STATE_EMPTY) && tagDef)
					{
						tagDef->FlagsToTagList(item.tagState, szStringBuffer, BUFFER_SIZE);
						event->setAttr("TagState", szStringBuffer);
					}
				}
				m_context.controllerDef.m_scopeIDs.IntegerFlagsToTagList(item.scopeMask, szStringBuffer, BUFFER_SIZE);
				event->setAttr("ScopeMask", szStringBuffer);
				event->setAttr("OptionIdx", item.optionIdx);
				event->setAttr("Trump", item.trumpsPrevious);
				break;
			case SMannHistoryItem::Tag:
				m_context.controllerDef.m_tags.FlagsToTagList(item.tagState, szStringBuffer, BUFFER_SIZE);
				event->setAttr("TagState", szStringBuffer);
				break;
			case SMannHistoryItem::None:
				continue;
				break;
			}

			root->addChild(event);
		}
	}

	const bool success = root->saveToFile(filename);
	if (!success)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Error saving CryMannequin history to '%s'", filename);
	}
}

void CActionController::DebugDrawLocation(const QuatT& location, ColorB colorPos, ColorB colorX, ColorB colorY, ColorB colorZ) const
{
	IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();

	const float thickness = 7.0f;
	const Vec3 pushUp(0.0f, 0.03f, 0.0f);

	pAuxGeom->DrawLine(location.t + pushUp, colorX, location.t + pushUp + location.q.GetColumn0(), colorX, thickness);
	pAuxGeom->DrawLine(location.t + pushUp, colorY, location.t + pushUp + location.q.GetColumn1(), colorY, thickness);
	pAuxGeom->DrawLine(location.t + pushUp, colorZ, location.t + pushUp + location.q.GetColumn2(), colorZ, thickness);

	const float radius = 0.06f;
	pAuxGeom->DrawSphere(location.t + pushUp, radius, colorPos);
}

float g_mannequinYPosEnd = 0;

void CActionController::DebugDraw() const
{
	g_mannequinYPosEnd = 0;

	static float XPOS_SCOPELIST = 50.0f;
	static float XPOS_SCOPEACTIONLIST = 250.0f;
	static float XPOS_QUEUE = 900.0f;
	static float XINC_PER_LETTER = 6.0f;
	static float YPOS = 20.0f;
	static float YINC = 16.0f;
	static float YINC_SEQUENCE = 15.0f;
	static float FONT_SIZE = 1.65f;
	static float FONT_SIZE_ANIMLIST = 1.2f;
	static float FONT_COLOUR[4] = {1.0f,1.0f,1.0f,1.0f};
	static float FONT_COLOUR_INACTIVE[4] = {0.5f,0.5f,0.5f,1.0f};
	static float FONT_COLOUR_ANIM_PENDING[4]		= {1.0f,1.0f,0.5f,1.0f};
	static float FONT_COLOUR_ANIM_INSTALLED[4]	= {0.5f,1.0f,0.5f,1.0f};
	static float FONT_COLOUR_PROC_PENDING[4]		= {0.5f,1.0f,1.0f,1.0f};
	static float FONT_COLOUR_PROC_INSTALLED[4]	= {0.5f,0.5f,1.0f,1.0f};
	static float FONT_COLOUR_ACTION_QUEUE[4]		= {0.5f,1.0f,0.5f,1.0f};

	float ypos = YPOS;
	const int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE] = {0};
	char buffer2[BUFFER_SIZE] = {0};

	const SControllerDef &controllerDef = m_context.controllerDef;
	if (m_flags & AC_PausedUpdate)
	{
		gEnv->pRenderer->Draw2dLabel(XPOS_SCOPELIST, ypos, FONT_SIZE, FONT_COLOUR, false, "[PAUSED ACTION CONTROLLER]");
		ypos += YINC;
	}

	m_context.controllerDef.m_tags.FlagsToTagList(m_context.state.GetMask(), buffer, 1024);
	gEnv->pRenderer->Draw2dLabel(XPOS_SCOPELIST, ypos, FONT_SIZE, FONT_COLOUR, false, "TagState: %s", buffer);
	ypos += YINC;

	for (uint32 i=0; i<m_context.subStates.size(); i++)
	{
		m_context.controllerDef.m_tags.FlagsToTagList(m_context.subStates[i].GetMask(), buffer, 1024);
		gEnv->pRenderer->Draw2dLabel(XPOS_SCOPELIST, ypos, FONT_SIZE, FONT_COLOUR, false, "%s: %s", controllerDef.m_subContextIDs.GetTagName(i), buffer);
		ypos += YINC;
	}
	ypos += YINC;

	for (uint32 i=0; i<m_scopes.size(); i++)
	{
		CActionScope *scope = m_scopes[i];

		FragmentID fragID = scope->GetLastFragmentID();
		const char *fragName = (fragID != FRAGMENT_ID_INVALID) ? controllerDef.m_fragmentIDs.GetTagName(fragID) : "NoFragment";
		if (scope->HasFragment())
		{
			controllerDef.m_tags.FlagsToTagList(scope->m_lastFragSelection.tagState.globalTags, buffer, BUFFER_SIZE);

			const CTagDefinition *pFragTags = controllerDef.GetFragmentTagDef(fragID);

			if (pFragTags && (scope->m_lastFragSelection.tagState.fragmentTags != TAG_STATE_EMPTY))
			{
				pFragTags->FlagsToTagList(scope->m_lastFragSelection.tagState.fragmentTags, buffer2, BUFFER_SIZE);
				if (buffer2 && buffer2[0])
				{
					if (scope->m_lastFragSelection.tagState.globalTags != TAG_STATE_EMPTY)
						strcat_s(buffer,BUFFER_SIZE, "+");
					strcat_s(buffer, BUFFER_SIZE, "[");
					strcat_s(buffer, BUFFER_SIZE, buffer2);
					strcat_s(buffer, BUFFER_SIZE, "]");
				}
			}
		}
		else
		{
			buffer[0] = 0;
			strncat(buffer, "No Match: ", BUFFER_SIZE);
			
			if( scope->m_pAction )
			{
				// scope tags (aka additional tags)
				controllerDef.m_tags.FlagsToTagList(scope->m_additionalTags, buffer2, BUFFER_SIZE);
				strncat(buffer, buffer2, BUFFER_SIZE - strlen(buffer) - 1);

				// scope context tags (aka additional tags)
				controllerDef.m_tags.FlagsToTagList(controllerDef.m_scopeContextDef[scope->m_scopeContext.id].additionalTags, buffer2, BUFFER_SIZE);
				strcat_s(buffer, BUFFER_SIZE, buffer2);

				// frag tags (aka fragment-specific tags)
				FragmentID fragIDNotMatched = scope->m_pAction->GetFragmentID();
				if (fragIDNotMatched != FRAGMENT_ID_INVALID)
				{
					const CTagDefinition *pFragTags = controllerDef.GetFragmentTagDef(fragIDNotMatched);

					if (pFragTags && scope->m_pAction->GetFragTagState() != TAG_STATE_EMPTY)
					{
						pFragTags->FlagsToTagList(scope->m_pAction->GetFragTagState(), buffer2, BUFFER_SIZE);
						if (buffer2 && buffer2[0])
						{
							if (scope->m_additionalTags != TAG_STATE_EMPTY)
								strcat_s(buffer,BUFFER_SIZE, "+");
					        strcat_s(buffer, BUFFER_SIZE, "[");
					        strcat_s(buffer, BUFFER_SIZE, buffer2);
					        strcat_s(buffer, BUFFER_SIZE, "]");
						}
					}
				}
			}
			else
			{
				strcpy(buffer, "No Action");
			}
	}

		float *colour = (m_activeScopes & (1<<i)) ? FONT_COLOUR : FONT_COLOUR_INACTIVE;

		gEnv->pRenderer->Draw2dLabel(XPOS_SCOPELIST, ypos, FONT_SIZE, colour, false, "%s:", scope->m_name.c_str());
		if (scope->m_pExitingAction)
		{
			strncpy(buffer2, scope->m_pExitingAction->GetName(), BUFFER_SIZE);
			strcat_s(buffer2, BUFFER_SIZE, "->");
		}
		else
		{
			buffer2[0] = '\0';
		}
		strncat(buffer2, scope->m_pAction ? scope->m_pAction->GetName() : " --- ", BUFFER_SIZE - strlen(buffer2) - 1);
		gEnv->pRenderer->Draw2dLabel(XPOS_SCOPEACTIONLIST, ypos, FONT_SIZE, colour, false, "%s \t%s(%s)\tP: %d %d TR: %f", buffer2, fragName, buffer, scope->m_pAction ? scope->m_pAction->GetPriority() : 0, scope->m_pAction ? scope->m_pAction->m_refCount : 0, scope->CalculateFragmentTimeRemaining() );
		ypos += YINC;

		if (scope->m_scopeContext.charInst)
		{
			IAnimationSet *animSet = scope->m_scopeContext.charInst->GetIAnimationSet();
			for (uint32 l=0; l<scope->m_numLayers; l++)
			{
				const CActionScope::SSequencer &sequencer = scope->m_layerSequencers[l];

				if (sequencer.sequence.size() > 0)
				{
					float xpos = XPOS_SCOPELIST;
					for (uint32 k=0; k<sequencer.sequence.size(); k++)
					{
						const char *animName = sequencer.sequence[k].animation.animRef.GetString();
						int letterCount = animName ? strlen(animName) : 0;
						float *colourA = FONT_COLOUR;
						if (k == sequencer.pos)
						{
							colourA = FONT_COLOUR_ANIM_PENDING;
						}
						else if (k == sequencer.pos-1)
						{
							colourA = FONT_COLOUR_ANIM_INSTALLED;
						}
						gEnv->pRenderer->Draw2dLabel(xpos, ypos, FONT_SIZE_ANIMLIST, colourA, false, "%s", animName);
						xpos += XPOS_SCOPELIST + (letterCount*XINC_PER_LETTER);
					}
					ypos += YINC_SEQUENCE;
				}
			}
		}

		const uint32 numProcLayers = scope->m_procSequencers.size();
		for (uint32 l=0; l<numProcLayers; l++)
		{
			const CActionScope::SProcSequencer &procSeq =	scope->m_procSequencers[l];

			if (procSeq.sequence.size() > 0)
			{
				float xpos = XPOS_SCOPELIST;
				for (uint32 k=0; k<procSeq.sequence.size(); k++)
				{
					const SProcClip &procClip = procSeq.sequence[k];

					if (procClip.procedural.type != 0)
					{
						const char *animName = procClip.procedural.typeString.c_str();
						int letterCount = animName ? strlen(animName) : 0;
						float *colourA = FONT_COLOUR;
						if (k == procSeq.pos)
						{
							colourA = FONT_COLOUR_PROC_PENDING;
						}
						else if (k == procSeq.pos-1)
						{
							colourA = FONT_COLOUR_PROC_INSTALLED;
						}
						strncpy(buffer, animName, BUFFER_SIZE);
						const char *infoString = NULL;
						if (!procClip.procedural.animRef.IsEmpty())
						{
							infoString = procClip.procedural.animRef.GetString();
						}
						else if (!procClip.procedural.dataString.empty())
						{
							infoString = procClip.procedural.dataString.c_str();
						}
						if (infoString)
						{
							strcat_s(buffer, BUFFER_SIZE, "(");
							strcat_s(buffer, BUFFER_SIZE, infoString);
							strcat_s(buffer, BUFFER_SIZE , ")");
						}
						gEnv->pRenderer->Draw2dLabel(xpos, ypos, FONT_SIZE_ANIMLIST, colourA, false, "%s", buffer);
						xpos += XPOS_SCOPELIST + (letterCount*XINC_PER_LETTER);
					}
				}
				ypos += YINC_SEQUENCE;
			}
		}

		ypos += YINC*0.5f;

		if (scope->m_pAction)
		{
			QuatT targetPos(IDENTITY);

			const bool success = scope->m_pAction->GetParam("TargetPos", targetPos);
			if (success)
			{
				DebugDrawLocation(
					targetPos,
					RGBA8(0x80,0x80,0x80,0xff),
					RGBA8(0xb0,0x80,0x80,0xff),
					RGBA8(0x80,0xb0,0x80,0xff),
					RGBA8(0x80,0x80,0xb0,0xff));
			}
		}
	}

	g_mannequinYPosEnd = ypos;

	ypos = YPOS;
	gEnv->pRenderer->Draw2dLabel(XPOS_QUEUE, ypos, FONT_SIZE, FONT_COLOUR_ACTION_QUEUE, false, "Pending Action Queue");
	ypos += YINC * 2.0f;
	for (uint32 i=0; i<m_queuedActions.size(); i++)
	{
		const IAction &action = *m_queuedActions[i];
		bool isPending = action.GetStatus() == IAction::Pending;
		buffer[0] = '\0';
		bool first = true;
		for (uint32 k=0; k<m_scopes.size(); k++)
		{
			if ((1<<k) & action.GetForcedScopeMask())
			{
				if (first)
				{
					strcpy(buffer, m_scopes[k]->m_name);
				}
				else
				{
					strcat(buffer, "|");
					strcat(buffer, m_scopes[k]->m_name);
				}

				first = false;
			}
		}
		FragmentID fragID = action.GetFragmentID();
		const char *fragName = (fragID != FRAGMENT_ID_INVALID) ? m_context.controllerDef.m_fragmentIDs.GetTagName(fragID) : "NoFragment";
		gEnv->pRenderer->Draw2dLabel(XPOS_QUEUE, ypos, FONT_SIZE, isPending ? FONT_COLOUR_ACTION_QUEUE : FONT_COLOUR, false, "%s: %s P: %d - %s", action.GetName(), fragName, action.GetPriority(), buffer);

		ypos += YINC;
	}

	{
		QuatT targetPos(IDENTITY);

		const bool success = m_mannequinParams.GetParam("TargetPos", targetPos);
		if (success)
		{
			DebugDrawLocation(
				targetPos,
				RGBA8(0xa0,0xa0,0xa0,0xff),
				RGBA8(0xc0,0xa0,0xa0,0xff),
				RGBA8(0xa0,0xc0,0xa0,0xff),
				RGBA8(0xa0,0xa0,0xc0,0xff));
		}
	}

	if (m_cachedEntity)
	{
		AABB bbox;
		m_cachedEntity->GetWorldBounds(bbox);
		const Vec3 entityWorldPos = m_cachedEntity->GetWorldPos();
		const Vec3 debugPos = Vec3( entityWorldPos.x, entityWorldPos.y, bbox.max.z );
		const float radius = 0.1f;
		IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		pAuxGeom->DrawSphere(debugPos, radius, RGBA8(0xff,0x00,0x00,0xff));
	}
}

#endif //!_RELEASE


IProceduralContext *CActionController::FindOrCreateProceduralContext(const char *contextName)
{
	IProceduralContext *procContext = FindProceduralContext(contextName);
	if (procContext)
		return procContext;

	return CreateProceduralContext(contextName);
}


IProceduralContext *CActionController::CreateProceduralContext(const char *contextName)
{
	const bool hasValidRootEntity = UpdateRootEntityValidity();
	if (!hasValidRootEntity)
		return NULL;

	const uint32 contextNameCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(contextName);

	SProcContext newProcContext;
	newProcContext.nameCRC = contextNameCRC;
	CryCreateClassInstance<IProceduralContext>(contextName, newProcContext.pContext);
	m_procContexts.push_back(newProcContext);

	newProcContext.pContext->Initialise(*m_cachedEntity, *this);
	return newProcContext.pContext.get();
}


const IProceduralContext *CActionController::FindProceduralContext( const char *contextName ) const
{
	const uint32 contextNameCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(contextName);
	return FindProceduralContext(contextNameCRC);
}


IProceduralContext *CActionController::FindProceduralContext( const char *contextName )
{
	const uint32 contextNameCRC = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(contextName);
	return FindProceduralContext(contextNameCRC);
}


IProceduralContext *CActionController::FindProceduralContext(const uint32 contextNameCRC) const
{
	const uint32 numContexts = m_procContexts.size();
	for (uint32 i=0; i<numContexts; i++)
	{
		if (m_procContexts[i].nameCRC == contextNameCRC)
		{
			return m_procContexts[i].pContext.get();
		}
	}
	return NULL;
}


bool CActionController::IsActionPending(uint32 userToken) const
{
	for (uint32 i=0; i<m_queuedActions.size(); i++)
	{
		const IAction &action = *m_queuedActions[i];
		if ( (action.GetStatus() == IAction::Pending) && (userToken == action.GetUserToken()) )
			return true;
	}
	return false;
}


void CActionController::Pause()
{
	if (!GetFlag(AC_PausedUpdate))
	{
		SetFlag(AC_PausedUpdate, true);
		const size_t scopeCount = m_scopes.size();
		for (size_t i = 0; i < scopeCount; ++i)
		{
			CActionScope* scope = m_scopes[i];
			scope->Pause();
		}
	}
}


void CActionController::Resume(uint32 resumeFlags)
{
	if (GetFlag(AC_PausedUpdate))
	{
		SetFlag(AC_PausedUpdate, false);
		const bool restartAnimations = ((resumeFlags & ERF_RestartAnimations) != 0);
		if (restartAnimations)
		{
			const size_t scopeCount = m_scopes.size();
			for (size_t i = 0; i < scopeCount; ++i)
			{
				CActionScope* scope = m_scopes[i];
				const float blendTime = -1;
				scope->Resume(blendTime, resumeFlags);
			}
		}
	}
}


ActionScopes CActionController::FlushScopesByScopeContext( uint32 scopeContextID, EFlushMethod flushMethod )
{
	const size_t numScopes = m_scopes.size();
	ActionScopes scopesUsingContext = 0;
	for (size_t i = 0, scopeFlag = 1; i < numScopes; i++, scopeFlag <<= 1)
	{
		CActionScope *scope = m_scopes[i];
		if (scope->GetContextID() == scopeContextID)
		{
			FlushScope(i, scopeFlag, flushMethod);

			scopesUsingContext |= scopeFlag;
		}
	}

	return scopesUsingContext;
}

void CActionController::UpdateValidity()
{
	// Validate root entity
	const bool rootEntityIsValid = UpdateRootEntityValidity();

	// Validate all scopecontexts (and keep track of which ones are invalid)
	const uint32 numScopeContexts = m_context.controllerDef.m_scopeContexts.GetNum();
	uint32 scopeContextsToFlush = 0;
	for (uint32 scopeContextID=0; scopeContextID<numScopeContexts; scopeContextID++)
	{
		const bool scopeContextIsValid = UpdateScopeContextValidity(scopeContextID);
		if (!rootEntityIsValid || !scopeContextIsValid)
		{
			scopeContextsToFlush |= BIT(scopeContextID);
		}
	}

	// Flush the invalid scopecontexts
	if (scopeContextsToFlush)
	{
		for (uint32 scopeContextID=0; scopeContextID<numScopeContexts; scopeContextID++)
		{
			if (scopeContextsToFlush & BIT(scopeContextID))
			{
				ActionScopes flushedScopes = FlushScopesByScopeContext(scopeContextID, FM_Failure);
				m_activeScopes &= ~flushedScopes;
			}
		}
	}
}

bool CActionController::UpdateScopeContextValidity(uint32 scopeContextID)
{
	SScopeContext& scopeContext = m_scopeContexts[scopeContextID];

	const bool hasInvalidEntity = scopeContext.HasInvalidEntity();
	if (hasInvalidEntity)
	{
#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
		IEntity* expectedEntity = gEnv->pEntitySystem->GetEntity(scopeContext.entityId);
		if (s_mnFatalErrorOnInvalidEntity)
			CryFatalError("[CActionController::UpdateScopeContextValidity] Dangling Entity %p (expected %p for id=%d) in context '%s'", (void*)scopeContext.cachedEntity, (void*)expectedEntity, scopeContext.entityId, m_context.controllerDef.m_scopeContexts.GetTagName(scopeContextID));
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[CActionController::UpdateScopeContextValidity] Dangling Entity %p (expected %p for id=%d) in context '%s'", (void*)scopeContext.cachedEntity, (void*)expectedEntity, scopeContext.entityId, m_context.controllerDef.m_scopeContexts.GetTagName(scopeContextID));
#endif // !CRYMANNEQUIN_WARN_ABOUT_VALIDITY()

		scopeContext.charInst = NULL;
		scopeContext.entityId = 0;
		scopeContext.cachedEntity = NULL;
	}

	const bool hasInvalidCharInst = scopeContext.HasInvalidCharInst();
	if (hasInvalidCharInst)
	{
#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
		const char *entityName = (scopeContext.cachedEntity ? scopeContext.cachedEntity->GetName() : "<NULL>");
		if (s_mnFatalErrorOnInvalidCharInst)
			CryFatalError("[CActionController::UpdateScopeContextValidity] Dangling Char Inst in entity '%s' (id=%d) in context '%s'", entityName, scopeContext.entityId, m_context.controllerDef.m_scopeContexts.GetTagName(scopeContextID));
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[CActionController::UpdateScopeContextValidity] Dangling Char Inst in entity '%s' (id=%d) in context '%s'", entityName, scopeContext.entityId, m_context.controllerDef.m_scopeContexts.GetTagName(scopeContextID));
#endif // !CRYMANNEQUIN_WARN_ABOUT_VALIDITY()

		scopeContext.charInst = NULL;
	}

	return !(hasInvalidEntity || hasInvalidCharInst);
}

bool CActionController::UpdateRootEntityValidity()
{
	const IEntity *expectedEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
	const bool hasValidRootEntity = (expectedEntity == m_cachedEntity);
	if (!hasValidRootEntity)
	{
#if CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
		if (s_mnFatalErrorOnInvalidEntity)
			CryFatalError("[CActionController::UpdateRootEntityValidity] Dangling Entity %p (expected %p for id=%d) in actioncontroller for '%s'", (void*)m_cachedEntity, (void*)expectedEntity, m_entityId, m_context.controllerDef.m_filename.GetString());
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "[CActionController::UpdateRootEntityValidity] Dangling Entity %p (expected %p for id=%d) in actioncontroller for '%s'", (void*)m_cachedEntity, (void*)expectedEntity, m_entityId, m_context.controllerDef.m_filename.GetString());
#endif // !CRYMANNEQUIN_WARN_ABOUT_VALIDITY()
		m_entityId = 0;
		m_cachedEntity = NULL;
	}

	return hasValidRootEntity && (m_cachedEntity || m_entityId == 0);
}

QuatT CActionController::ExtractLocalAnimLocation(FragmentID fragID, TagState fragTags, uint32 scopeID, uint32 optionIdx, bool ignoreOutros)
{
	QuatT targetLocation(IDENTITY);

	if(IScope* pScope = GetScope(scopeID))
	{
		IActionPtr pCurrentAction = pScope->GetAction();
		ICharacterInstance* pCharInst = pScope->GetCharInst();

		const IAnimationDatabase& animDatabase = pScope->GetDatabase();
		const IAnimationSet* pAnimationSet = pCharInst ? pCharInst->GetIAnimationSet() : NULL;
		
		if(pAnimationSet)
		{
			SFragmentData fragData;
			SBlendQuery blendQuery;

			blendQuery.fragmentFrom = pCurrentAction ? pCurrentAction->GetFragmentID() : FRAGMENT_ID_INVALID;
			blendQuery.fragmentTo = fragID;

			blendQuery.tagStateFrom.globalTags = blendQuery.tagStateTo.globalTags = GetContext().state.GetMask();
			blendQuery.tagStateFrom.fragmentTags = pCurrentAction ? pCurrentAction->GetFragTagState() : TAG_STATE_EMPTY;
			blendQuery.tagStateTo.fragmentTags = fragTags;

			blendQuery.flags = SBlendQuery::toInstalled;
			if(!ignoreOutros)
			{
				blendQuery.flags |= SBlendQuery::fromInstalled;
			}

			animDatabase.Query(fragData, blendQuery, optionIdx, pAnimationSet);

			if(!fragData.animLayers.empty() && !fragData.animLayers[0].empty())
			{
				int animID = pAnimationSet->GetAnimIDByCRC(fragData.animLayers[0][0].animation.animRef.crc);
				if(animID >= 0)
				{
					pAnimationSet->GetAnimationDCCWorldSpaceLocation(animID, targetLocation);
				}
			}		
		}
	}

	return targetLocation;
}

void CActionController::SetTimeScale( float timeScale )
{
	const uint32 numContexts = m_context.controllerDef.m_scopeContexts.GetNum();
	for(uint32 i = 0; i < numContexts; ++i)
	{
		const SScopeContext& scopeContext = m_scopeContexts[i];
		if(scopeContext.charInst)
		{
			scopeContext.charInst->SetPlaybackScale(timeScale);
		}
	}
	m_timeScale = timeScale;
}
