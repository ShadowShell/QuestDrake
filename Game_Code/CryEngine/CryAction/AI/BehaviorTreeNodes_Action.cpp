// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "BehaviorTreeNodes_Action.h"
#include "BehaviorTree/IBehaviorTree.h"
#include "BehaviorTree/Action.h"
#include "BehaviorTree/Decorator.h"
#include "IActorSystem.h"
#include "Mannequin/AnimActionTriState.h"
#include "XMLAttrReader.h"

namespace BehaviorTree
{	
	// Play an animation fragment directly through Mannequin (start a fragment 
	// for a specific FragmentID), and wait until it is done.
	class AnimateFragment : public Action
	{
	public:
		// TODO: Move mannequin priorities into CryAction or get this from a central location
		static const int NORMAL_ACTION_PRIORITY = 3; // currently equal to PP_Action, just above movement, but underneath urgent actions, hit death, etc.
		static const int URGENT_ACTION_PRIORITY = 4; // currently equal to PP_Urgent_Action, just above normal actions, but underneath hit death, etc.

		struct RuntimeData
		{
			// The action animation action that is running or NULL if none.
			typedef _smart_ptr<CAnimActionTriState> CAnimActionTriStatePtr;
			CAnimActionTriStatePtr action;

			// True if the action was queued; false if an error occured.
			bool actionQueuedFlag;

			RuntimeData()
				: actionQueuedFlag(false)
			{
			}

			~RuntimeData()
			{
				ReleaseAction();
			}

			void ReleaseAction()
			{
				if (this->action.get() != NULL)
				{
					this->action->ForceFinish();
					this->action.reset();
				}
			}
		};
		
		AnimateFragment() :
			m_fragNameCrc32(0)
#ifdef STORE_INFORMATION_FOR_BEHAVIOR_TREE_VISUALIZER
			, m_fragName()
#endif			
		{
		}

		virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) OVERRIDE
		{
			const string fragName = xml->getAttr("name");
			IF_UNLIKELY (fragName.empty())
			{
				ErrorReporter(*this, context).LogError("Missing attribute 'name'.");
				return LoadFailure;
			}
#ifdef STORE_INFORMATION_FOR_BEHAVIOR_TREE_VISUALIZER
			m_fragName = fragName;
#endif
			m_fragNameCrc32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(fragName);

			return LoadSuccess;
		}


#ifdef STORE_INFORMATION_FOR_BEHAVIOR_TREE_VISUALIZER
		virtual void GetDebugTextForVisualizer(const UpdateContext& updateContext, stack_string& debugText) const OVERRIDE
		{
			debugText = m_fragName;
		}
#endif


	protected:

		virtual void OnInitialize(const UpdateContext& context) OVERRIDE
		{
			QueueAction(context);
		}


		virtual void OnTerminate(const UpdateContext& context) OVERRIDE
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
			runtimeData.ReleaseAction();
		}


		virtual Status Update(const UpdateContext& context) OVERRIDE
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
			
			IF_UNLIKELY (!runtimeData.actionQueuedFlag)
			{
				return Failure;
			}

			assert(runtimeData.action.get() != NULL);
			const IAction::EStatus currentActionStatus = runtimeData.action->GetStatus();
			IF_UNLIKELY ( (currentActionStatus == IAction::None) || (currentActionStatus == IAction::Finished) )
			{
				return Success;
			}

			return Running;
		}


	private:

		void QueueAction(const UpdateContext& context)
		{
			RuntimeData& runtimeData = GetRuntimeData<RuntimeData>(context);
			
			IActor* actor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(context.entityId);
			
			IF_UNLIKELY (actor == NULL)
			{
				assert(false);
				return;
			}

			IAnimatedCharacter* animChar = actor->GetAnimatedCharacter();
			IF_UNLIKELY (animChar == NULL)
			{
				assert(false);
				return;
			}

			IF_LIKELY (runtimeData.action.get() == NULL)
			{
				IActionController* pIActionController = animChar->GetActionController();
				const FragmentID fragID = pIActionController->GetFragID(m_fragNameCrc32);
				IF_LIKELY (fragID == FRAGMENT_ID_INVALID)
				{
#ifdef STORE_INFORMATION_FOR_BEHAVIOR_TREE_VISUALIZER
					ErrorReporter(*this, context).LogError("Invalid fragment name '%s'", m_fragName);
#else
					ErrorReporter(*this, context).LogError("Invalid fragment name!");
#endif
					return;
				}
				runtimeData.action = new CAnimActionTriState(NORMAL_ACTION_PRIORITY, fragID, *animChar, true /*oneshot*/);
				pIActionController->Queue(runtimeData.action);
			}

			runtimeData.actionQueuedFlag = true;
		}





	private:

		// The CRC value for the fragment name.
		uint32 m_fragNameCrc32;

#ifdef STORE_INFORMATION_FOR_BEHAVIOR_TREE_VISUALIZER
		// The actual fragment name.
		string m_fragName;
#endif
	};

 	GenerateBehaviorTreeNodeCreator(AnimateFragment);
}



void RegisterActionBehaviorTreeNodes(BehaviorTree::INodeFactory& factory)
{
	using namespace BehaviorTree;

 	ExposeBehaviorTreeNodeToFactory(factory, AnimateFragment);
}