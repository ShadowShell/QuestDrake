#include "StdAfx.h"
#include "FlowBaseNode.h"

#include <IEntitySystem.h>
#include <IAISystem.h>
#include <IAgent.h>
#include "IAIObject.h"

class CFlowNode_Dynamics : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowNode_Dynamics( SActivationInfo * pActInfo ) 
	{
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<bool>("active", true, _HELP("Update data on/off")),
			{0}
		};

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>("v", _HELP("Velocity of entity")),
			OutputPortConfig<Vec3>("a", _HELP("Acceleration of entity")),
			OutputPortConfig<Vec3>("w", _HELP("Angular velocity of entity")),
			OutputPortConfig<Vec3>("wa", _HELP("Angular acceleration of entity")),
			OutputPortConfig<float>("m", _HELP("Mass of entity")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Dynamic physical state of an entity");
		config.SetCategory(EFLN_APPROVED); // POLICY CHANGE: Maybe an Enable/Disable Port
	}


	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			break;
		case eFE_Update:
			{
				if(!GetPortBool(pActInfo, 0))
					return;

				IEntity * pEntity = pActInfo->pEntity;
				if (pEntity)
				{
					IPhysicalEntity * pPhysEntity = pEntity->GetPhysics();
					if (pPhysEntity)
					{
						pe_status_dynamics dyn;
						pPhysEntity->GetStatus( &dyn );
						ActivateOutput(pActInfo, 0, dyn.v);
						ActivateOutput(pActInfo, 1, dyn.a);
						ActivateOutput(pActInfo, 2, dyn.w);
						ActivateOutput(pActInfo, 3, dyn.wa);
						ActivateOutput(pActInfo, 4, dyn.mass);
					}
				}
			}
		}
	}
};

class CFlowNode_PhysicsSleepQuery : public CFlowBaseNode<eNCT_Instanced>
{
	enum EInPorts
	{
		IN_CONDITION = 0,
		IN_RESET
	};
	enum EOutPorts
	{
		OUT_SLEEPING = 0,
		OUT_ONSLEEP,
		OUT_ONAWAKE
	};

	bool m_Activated;

public:
	CFlowNode_PhysicsSleepQuery( SActivationInfo * pActInfo ) {}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<bool>( "condition", _HELP("Setting this input to TRUE sends the sleeping condition of the entity to the [sleeping] port, and triggers [sleep] and [awake]") ),
			InputPortConfig<bool>( "reset", _HELP("Triggering this input to TRUE resets the node") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<bool>( "sleeping", _HELP("Set to TRUE when the physics of the entity are sleeping (passes the value through)") ),
			OutputPortConfig<SFlowSystemVoid>( "sleep", _HELP("Triggers to TRUE when the physics of the entity switch to sleep") ),
			OutputPortConfig<SFlowSystemVoid>( "awake", _HELP("Triggers to TRUE when the physics of the entity switch to awake") ),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Node that returns the sleeping state of the physics of a given entity.");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
			m_Activated = false;
			break;

		case eFE_Update:
			{
				bool bReset = GetPortBool(pActInfo, IN_RESET);
				bool bCondition = GetPortBool(pActInfo, IN_CONDITION);

				if(bReset)
				{
					ActivateOutput(pActInfo, OUT_SLEEPING, !bCondition);
				} else
				{
					if(bCondition != m_Activated)
					{
						IEntity * pEntity = pActInfo->pEntity;

						if (pEntity)
						{
							IPhysicalEntity * pPhysEntity = pEntity->GetPhysics();

							if (pPhysEntity)
							{
								pe_status_awake psa;

								bool isSleeping = pPhysEntity->GetStatus( &psa ) ? false : true;

								ActivateOutput(pActInfo, OUT_SLEEPING, isSleeping);

								if(isSleeping)
									ActivateOutput(pActInfo, OUT_ONSLEEP, true);
								else
									ActivateOutput(pActInfo, OUT_ONAWAKE, true);
							}
						}

						m_Activated = bCondition;
					}
				}
			}
		}
	}
};

class CFlowNode_ActionImpulse : public CFlowBaseNode<eNCT_Singleton>
{
	enum ECoordSys
	{
		CS_PARENT = 0,
		CS_WORLD,
		CS_LOCAL
	};

	enum EInPorts
	{
		IN_ACTIVATE = 0,
		IN_IMPULSE,
		IN_ANGIMPULSE,
		IN_POINT,
		IN_PARTINDEX,
		IN_COORDSYS,
	};



public:
	CFlowNode_ActionImpulse( SActivationInfo * pActInfo ) {}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void( "activate", _HELP("Trigger the impulse") ),
			InputPortConfig<Vec3>( "impulse", Vec3(0,0,0), _HELP("Impulse vector") ),
			InputPortConfig<Vec3>( "angImpulse", Vec3(0,0,0), _HELP("The angular impulse") ),
			InputPortConfig<Vec3>( "Point", Vec3(0,0,0), _HELP("Point of application (optional)") ),
			InputPortConfig<int>( "partIndex", 0, _HELP("Index of the part that will receive the impulse (optional, 1-based, 0=unspecified)") ),
			InputPortConfig<int> ( "CoordSys", 1, _HELP("Defines which coordinate system is used for the inputs values"), _HELP("CoordSys"), _UICONFIG("enum_int:Parent=0,World=1,Local=2") ),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY|EFLN_AISEQUENCE_SUPPORTED;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.sDescription = _HELP("Applies an impulse on an entity");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (!pActInfo->pEntity)
			return;

		if (event == eFE_Activate && IsPortActive(pActInfo, IN_ACTIVATE))
		{
			pe_action_impulse action;

			int ipart = GetPortInt( pActInfo, IN_PARTINDEX );
			if (ipart>0)
				action.ipart = ipart-1;

			IEntity* pEntity = pActInfo->pEntity;
			ECoordSys coordSys = (ECoordSys)GetPortInt( pActInfo, IN_COORDSYS );

			if (coordSys==CS_PARENT && !pEntity->GetParent())
				coordSys = CS_WORLD;

			// When a "zero point" is set in the node, the value is left undefined and physics assume it is the CM of the object.
			// but when the entity has a parent (is linked), then we have to use a real world coordinate for the point, because we have to apply the impulse to the highest entity 
			// on the hierarchy and physics will use the position of that entity instead of the position of the entity assigned to the node
			bool bHaveToUseTransformedZeroPoint = false;
			Vec3 transformedZeroPoint;
			Matrix34 transMat;

		  switch (coordSys)
			{
				case CS_WORLD:
				default:
				{
					transMat.SetIdentity();
					bHaveToUseTransformedZeroPoint = pEntity->GetParent()!=NULL;
					transformedZeroPoint = pEntity->GetWorldPos();
					break;
				}

				case CS_PARENT:
				{
					transMat = pEntity->GetParent()->GetWorldTM();
					bHaveToUseTransformedZeroPoint = pEntity->GetParent()->GetParent()!=NULL;
					transformedZeroPoint = pEntity->GetParent()->GetWorldPos();
					break;
				}

				case CS_LOCAL:
				{
					transMat = pEntity->GetWorldTM();
					bHaveToUseTransformedZeroPoint = pEntity->GetParent()!=NULL;
					transformedZeroPoint = pEntity->GetWorldPos();
					break;
				}
			}

			action.impulse = GetPortVec3( pActInfo, IN_IMPULSE );
			action.impulse = transMat.TransformVector( action.impulse );

			Vec3 angImpulse = GetPortVec3( pActInfo, IN_ANGIMPULSE );
			if (!angImpulse.IsZero())
				action.angImpulse = transMat.TransformVector( angImpulse );

			Vec3 pointApplication = GetPortVec3( pActInfo, IN_POINT );
			if (!pointApplication.IsZero())
				action.point = transMat.TransformPoint( pointApplication );
			else
			{
				if (bHaveToUseTransformedZeroPoint)
					action.point = transformedZeroPoint;
			}


			// the impulse has to be applied to the highest entity in the hierarchy. This comes from how physics manage linked entities.
			IEntity* pEntityImpulse = pEntity;
			while (pEntityImpulse->GetParent())
			{
				pEntityImpulse = pEntityImpulse->GetParent();
			}

			IPhysicalEntity * pPhysEntity = pEntityImpulse->GetPhysics();
			if (pPhysEntity)
				pPhysEntity->Action( &action );
		}
	}
};

class CFlowNode_Raycast : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowNode_Raycast( SActivationInfo * pActInfo ) {}

	enum EInPorts
	{
		GO = 0,
		DIR,
		MAXLENGTH,
		POS,
		TRANSFORM_DIRECTION,
	};
	enum EOutPorts
	{
		NOHIT = 0,
		HIT,
		DIROUT,
		DISTANCE,
		HITPOINT,
		NORMAL,
		SURFTYPE,
		HIT_ENTITY,
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<SFlowSystemVoid>( "go", SFlowSystemVoid(), _HELP("Perform Raycast") ),
			InputPortConfig<Vec3>( "direction", Vec3(0,1,0), _HELP("Direction of Raycast") ),
			InputPortConfig<float>( "maxLength", 10.0f, _HELP("Maximum length of Raycast") ),
			InputPortConfig<Vec3>( "position", Vec3(0,0,0), _HELP("Ray start position, relative to entity") ),
			InputPortConfig<bool>( "transformDir", true, _HELP("Direction is transformed by entity orientation.") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<SFlowSystemVoid>( "nohit", _HELP("Triggered if NO object was hit by raycast") ),
			OutputPortConfig<SFlowSystemVoid>( "hit", _HELP("Triggered if an object was hit by raycast") ),
			OutputPortConfig<Vec3>( "direction", _HELP("Actual direction of the cast ray (transformed by entity rotation")),
			OutputPortConfig<float>( "distance", _HELP("Distance to object hit") ),
			OutputPortConfig<Vec3>( "hitpoint", _HELP("Position the ray hit") ),
			OutputPortConfig<Vec3>( "normal", _HELP("Normal of the surface at the hitpoint") ),
			OutputPortConfig<int>( "surfacetype", _HELP("Surface type index at the hit point") ),
			OutputPortConfig<EntityId> ( "entity", _HELP("Entity which was hit")), 
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate && IsPortActive(pActInfo, GO))
		{
			IEntity * pEntity = pActInfo->pEntity;
			if (pEntity)
			{
				ray_hit hit;
				IPhysicalEntity *pSkip = pEntity->GetPhysics();
				Vec3 direction = GetPortVec3(pActInfo, DIR).GetNormalized();
				if(GetPortBool(pActInfo, TRANSFORM_DIRECTION))
					direction = pEntity->GetWorldTM().TransformVector( GetPortVec3(pActInfo, DIR).GetNormalized() );
				IPhysicalWorld * pWorld = gEnv->pPhysicalWorld;
				int numHits = pWorld->RayWorldIntersection( 
					pEntity->GetPos() + GetPortVec3(pActInfo, POS),
					direction * GetPortFloat(pActInfo, MAXLENGTH),
					ent_all,
					rwi_stop_at_pierceable|rwi_colltype_any,
					&hit, 1, 
					&pSkip, 1 );
					
				if (numHits)
				{
					pEntity = (IEntity*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);    
					ActivateOutput( pActInfo, HIT,(bool)true );
					ActivateOutput( pActInfo, DIROUT, direction );
					ActivateOutput( pActInfo, DISTANCE, hit.dist );
					ActivateOutput( pActInfo, HITPOINT, hit.pt );
					ActivateOutput( pActInfo, NORMAL, hit.n );
					ActivateOutput( pActInfo, SURFTYPE, (int)hit.surface_idx );
					ActivateOutput( pActInfo, HIT_ENTITY, pEntity ? pEntity->GetId() : 0);
				}
				else
					ActivateOutput( pActInfo, NOHIT, false);
			}
		}
	}
};

class CFlowNode_RaycastCamera : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowNode_RaycastCamera( SActivationInfo * pActInfo ) {}

	enum EInPorts
	{
		GO = 0,
		POS,
		MAXLENGTH,
	};
	enum EOutPorts
	{
		NOHIT = 0,
		HIT,
		DIROUT,
		DISTANCE,
		HITPOINT,
		NORMAL,
		SURFTYPE,
		HIT_ENTITY,
	};

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<SFlowSystemVoid>( "go", SFlowSystemVoid(), _HELP("Perform Raycast") ),
			InputPortConfig<Vec3>( "offset", Vec3(0,0,0), _HELP("Ray start position, relative to camera") ),
			InputPortConfig<float>( "maxLength", 10.0f, _HELP("Maximum length of Raycast") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<SFlowSystemVoid>( "nohit", _HELP("Triggered if NO object was hit by raycast") ),
			OutputPortConfig<SFlowSystemVoid>( "hit", _HELP("Triggered if an object was hit by raycast") ),
			OutputPortConfig<Vec3>( "direction", _HELP("Actual direction of the cast ray (transformed by entity rotation")),
			OutputPortConfig<float>( "distance", _HELP("Distance to object hit") ),
			OutputPortConfig<Vec3>( "hitpoint", _HELP("Position the ray hit") ),
			OutputPortConfig<Vec3>( "normal", _HELP("Normal of the surface at the hitpoint") ),
			OutputPortConfig<int>( "surfacetype", _HELP("Surface type index at the hit point") ),
			OutputPortConfig<EntityId> ( "entity", _HELP("Entity which was hit")), 
			{0}
		};
		config.sDescription = _HELP("Perform a raycast relative to the camera");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate && IsPortActive(pActInfo, GO))
		{
			IEntity * pEntity = pActInfo->pEntity;
			// if (pEntity)
			{
				ray_hit hit;
				CCamera& cam = GetISystem()->GetViewCamera();
				Vec3 pos = cam.GetPosition()+cam.GetViewdir();
				Vec3 direction = cam.GetViewdir();
				IPhysicalWorld * pWorld = gEnv->pPhysicalWorld;
//				IPhysicalEntity *pSkip = 0; // pEntity->GetPhysics();
				int numHits = pWorld->RayWorldIntersection( 
				pos + GetPortVec3(pActInfo, POS),
				direction * GetPortFloat(pActInfo, MAXLENGTH),
				ent_all,
				rwi_stop_at_pierceable|rwi_colltype_any,
				&hit, 1
				/* ,&pSkip, 1 */  );
				if (numHits)
				{
					pEntity = (IEntity*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);    
					ActivateOutput( pActInfo, HIT,(bool)true );
					ActivateOutput( pActInfo, DIROUT, direction );
					ActivateOutput( pActInfo, DISTANCE, hit.dist );
					ActivateOutput( pActInfo, HITPOINT, hit.pt );
					ActivateOutput( pActInfo, NORMAL, hit.n );
					ActivateOutput( pActInfo, SURFTYPE, (int)hit.surface_idx );
					ActivateOutput( pActInfo, HIT_ENTITY, pEntity ? pEntity->GetId() : 0);
				}
				else
					ActivateOutput( pActInfo, NOHIT, false );
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// Enable/Disable AI for an entity/
//////////////////////////////////////////////////////////////////////////
class CFlowNode_PhysicsEnable : public CFlowBaseNode<eNCT_Singleton>
{
public:
	enum EInputs {
		IN_ENABLE,
		IN_DISABLE,
		IN_ENABLE_AI,
		IN_DISABLE_AI,
	};
	CFlowNode_PhysicsEnable( SActivationInfo * pActInfo ) {};

	/*
	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return this;
	}
	*/

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void( "Enable",_HELP("Enable Physics for target entity") ),
			InputPortConfig_Void( "Disable",_HELP("Disable Physics for target entity") ),
			InputPortConfig_Void( "AI_Enable",_HELP("Enable AI for target entity") ),
			InputPortConfig_Void( "AI_Disable",_HELP("Disable AI for target entity") ),
			{0}
		};
		config.sDescription = _HELP( "Enables/Disables Physics" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = 0;
		config.SetCategory(EFLN_ADVANCED);
	}
	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Activate:
			{
				if (!pActInfo->pEntity)
					return;

				if(IsPortActive(pActInfo, IN_ENABLE))
				{
					IEntityPhysicalProxy *pPhysicalProxy = (IEntityPhysicalProxy*)pActInfo->pEntity->GetProxy(ENTITY_PROXY_PHYSICS);
					if (pPhysicalProxy)
						pPhysicalProxy->EnablePhysics(true);
				}
				if(IsPortActive(pActInfo, IN_DISABLE))
				{
					IEntityPhysicalProxy *pPhysicalProxy = (IEntityPhysicalProxy*)pActInfo->pEntity->GetProxy(ENTITY_PROXY_PHYSICS);
					if (pPhysicalProxy)
						pPhysicalProxy->EnablePhysics(false);
				}
	
				if(IsPortActive(pActInfo, IN_ENABLE_AI))
				{
					if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
						aiObject->Event(AIEVENT_ENABLE,0);
				}
				if(IsPortActive(pActInfo, IN_DISABLE_AI))
				{
					if (IAIObject* aiObject = pActInfo->pEntity->GetAI())
						aiObject->Event(AIEVENT_DISABLE,0);
				}
				break;
			}

		case eFE_Initialize:
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE( "Physics:Dynamics", CFlowNode_Dynamics );
REGISTER_FLOW_NODE( "Physics:ActionImpulse", CFlowNode_ActionImpulse );
REGISTER_FLOW_NODE( "Physics:RayCast", CFlowNode_Raycast );
REGISTER_FLOW_NODE( "Physics:RayCastCamera", CFlowNode_RaycastCamera );
REGISTER_FLOW_NODE( "Physics:PhysicsEnable", CFlowNode_PhysicsEnable );
REGISTER_FLOW_NODE( "Physics:PhysicsSleepQuery", CFlowNode_PhysicsSleepQuery );
