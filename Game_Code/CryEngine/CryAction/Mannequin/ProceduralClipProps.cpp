////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ICryMannequin.h"
#include <CryExtension/Impl/ClassWeaver.h>

class CProceduralClipAttachProp : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipAttachProp, "AttachProp", 0xB2B852D9E6754388, 0x877D5A8E91481A60)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		if(m_charInstance)
		{
			if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(params.dataCRC.crc))
			{
				m_attachmentCRC = params.dataCRC.crc;
				IAttachmentObject* pNewAttachment = NULL;

				const char* fileExt = PathUtil::GetExt(params.dataString.c_str());

				bool isCGF = (0 == stricmp(fileExt,"cgf"));

				if (isCGF)
				{
					if(m_pAttachedStatObj = gEnv->p3DEngine->LoadStatObj(params.dataString.c_str(), NULL, NULL, false))
					{
						CCGFAttachment* pCGFAttachment = new CCGFAttachment();
						pCGFAttachment->pObj = m_pAttachedStatObj;
					
						pNewAttachment = pCGFAttachment;
					}
				}
				else if(m_pAttachedCharInst = gEnv->pCharacterManager->CreateInstance(params.dataString.c_str()))
				{
					CSKELAttachment *pChrAttachment = new CSKELAttachment();
					pChrAttachment->m_pCharInstance = m_pAttachedCharInst;

					pNewAttachment = pChrAttachment;
				}

				if(pNewAttachment)
				{
					pAttachment->AddBinding(pNewAttachment);
				}
			}
		}
	}

virtual void OnExit(float blendTime) 
{
	if(m_pAttachedCharInst || m_pAttachedStatObj)
	{
		if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(m_attachmentCRC))
		{
			if(IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject()) 
			{
				//ensure the correct model is still bound to the attachment before clearing
				if((m_pAttachedCharInst && pAttachmentObject->GetICharacterInstance() == m_pAttachedCharInst)
						|| ( m_pAttachedStatObj && pAttachmentObject->GetIStatObj() == m_pAttachedStatObj))
				{
					pAttachment->ClearBinding();
				}
			}
		}
	}

	m_pAttachedStatObj = NULL;
	m_pAttachedCharInst = NULL;
}

virtual void Update(float timePassed) {}

private:
	uint32 m_attachmentCRC; //TODO: This should be available via m_pParams pointer but currently is cleared by the time OnExit is called. Remove once fixed by Tom
	_smart_ptr<IStatObj> m_pAttachedStatObj;
	_smart_ptr<ICharacterInstance> m_pAttachedCharInst;
};

CProceduralClipAttachProp::CProceduralClipAttachProp() : m_pAttachedStatObj(NULL), m_pAttachedCharInst(NULL), m_attachmentCRC(0)
{
}

CProceduralClipAttachProp::~CProceduralClipAttachProp()
{
}

CRYREGISTER_CLASS(CProceduralClipAttachProp)

class CProceduralClipAttachEntity : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipAttachEntity, "AttachEntity", 0xB2C952D9E6754388, 0x877E6A8E91481A60)

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		if(m_charInstance)
		{
			if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(params.dataCRC.crc))
			{
				m_attachedEntityId = 0;
				m_attachmentCRC = params.dataCRC.crc;
				IAttachmentObject* pNewAttachment = NULL;

				EntityId attachEntityId = 0; 
				
				GetParam(params.dataString.c_str(), attachEntityId);

				if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(attachEntityId))
				{
					CEntityAttachment* pEntityAttachment = new CEntityAttachment();
					pEntityAttachment->SetEntityId(attachEntityId);

					pAttachment->AddBinding(pEntityAttachment);
					m_attachedEntityId = attachEntityId;

					if(IPhysicalEntity* pPhysics = pEntity->GetPhysics())
					{
						pe_action_add_constraint constraint;
						constraint.flags = constraint_inactive|constraint_ignore_buddy;
						constraint.pBuddy = m_scope->GetEntity().GetPhysics();
						constraint.pt[0].Set(0,0,0);
						m_attachedConstraintId = pPhysics->Action(&constraint);

						pe_params_part colltype;
						colltype.flagsOR = geom_no_coll_response;
						pPhysics->SetParams(&colltype);
					}
				}
			}
		}
	}

	virtual void OnExit(float blendTime) 
	{
		if(m_attachedEntityId)
		{
			if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(m_attachmentCRC))
			{
				if(IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject()) 
				{
					//ensure the correct entity is still bound to the attachment before clearing
					if(pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Entity)
					{
						if(static_cast<CEntityAttachment*>(pAttachmentObject)->GetEntityId() == m_attachedEntityId)
						{
							pAttachment->ClearBinding();

							if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_attachedEntityId))
							{
								if(IPhysicalEntity* pPhysics = pEntity->GetPhysics())
								{
									pe_action_update_constraint constraint;
									constraint.bRemove = true;
									constraint.idConstraint = m_attachedConstraintId;
									pPhysics->Action(&constraint);

									pe_params_part colltype;
									colltype.flagsAND = ~geom_no_coll_response;
									pPhysics->SetParams(&colltype);

									pe_action_awake action;
									action.bAwake = true;
									action.minAwakeTime = 0.5f;
									pPhysics->Action(&action);
								}
							}
						}
					}
				}
			}
		}

		m_attachedEntityId = 0;
		m_attachedConstraintId = 0;
	}

	virtual void Update(float timePassed) {}

private:
	uint32		m_attachmentCRC;
	EntityId	m_attachedEntityId;
	uint			m_attachedConstraintId;
};

CProceduralClipAttachEntity::CProceduralClipAttachEntity() : m_attachmentCRC(0), m_attachedEntityId(0), m_attachedConstraintId(0)
{
}

CProceduralClipAttachEntity::~CProceduralClipAttachEntity()
{
}

CRYREGISTER_CLASS(CProceduralClipAttachEntity)

struct SSwapAttachmentParams : public SProceduralParams
{
	float resetOnExit;
	float clearOnExit;
};

class CProceduralClipSwapAttachment : public TProceduralClip<SSwapAttachmentParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipSwapAttachment, "SwapAttachment", 0xB2B852DFE6754388, 0x87745A8E91481A60)

	virtual void OnEnter(float blendTime, float duration, const SSwapAttachmentParams &params)
	{
		m_newAttachmentCRC = 0;
		m_oldAttachmentCRC = 0;

		if(m_charInstance)
		{
			if(IAttachment* pNewAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(params.dataCRC.crc))
			{
				uint32 oldattachment = gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(params.dataString.c_str());
				if(IAttachment* pOldAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(oldattachment))
				{	
					if(pNewAttachment->GetIAttachmentObject() == NULL && pOldAttachment->GetIAttachmentObject() != NULL)
					{
						if(params.resetOnExit > 0.f)
						{
							m_newAttachmentCRC = params.dataCRC.crc;
							m_oldAttachmentCRC = oldattachment;
						}
						else if(params.clearOnExit > 0.f)
						{
							m_newAttachmentCRC = params.dataCRC.crc;
						}

						pOldAttachment->SwapBinding(pNewAttachment);
					}
				}
			}
		}
	}

	virtual void OnExit(float blendTime) 
	{
		if(m_newAttachmentCRC != 0)
		{
			if(IAttachment* pNewAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(m_newAttachmentCRC))
			{
				if(m_oldAttachmentCRC != 0)
				{
					if(IAttachment* pOldAttachment =  m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(m_oldAttachmentCRC))
					{
						if(pOldAttachment->GetIAttachmentObject() == NULL && pNewAttachment->GetIAttachmentObject() != NULL)
						{
							pNewAttachment->SwapBinding(pOldAttachment);
						}
					}
				}
				else
				{
					pNewAttachment->ClearBinding();
				}
			}
		}
	}

	virtual void Update(float timePassed) {}

private:
	uint32 m_newAttachmentCRC;
	uint32 m_oldAttachmentCRC;
};

CProceduralClipSwapAttachment::CProceduralClipSwapAttachment() : m_newAttachmentCRC(0), m_oldAttachmentCRC(0)
{
}

CProceduralClipSwapAttachment::~CProceduralClipSwapAttachment()
{
}

CRYREGISTER_CLASS(CProceduralClipSwapAttachment)

class CProceduralClipHideAttachment : public TProceduralClip<SProceduralParams>
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
		CRYINTERFACE_END()

		CRYGENERATE_CLASS(CProceduralClipHideAttachment, "HideAttachment", 0x1D68702FC5E64DF4, 0x92C8BD29869228BC)


	enum EHideFlags
	{
		eHideFlags_HiddenInMainPass = BIT(0),
		eHideFlags_HiddenInShadow = BIT(1),
		eHideFlags_HiddenInRecursion = BIT(2),
	};

	virtual void OnEnter(float blendTime, float duration, const SProceduralParams &params)
	{
		m_attachmentCRC = 0;
		m_hiddenFlags = 0; 

		if(m_charInstance)
		{
			if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(params.dataCRC.crc))
			{
				m_attachmentCRC = params.dataCRC.crc;

				if(pAttachment->IsAttachmentHidden())
				{
					m_hiddenFlags |= eHideFlags_HiddenInMainPass;
				}

				if(pAttachment->IsAttachmentHiddenInRecursion())
				{
					m_hiddenFlags |= eHideFlags_HiddenInRecursion;
				}

				if(pAttachment->IsAttachmentHiddenInShadow())
				{
					m_hiddenFlags |= eHideFlags_HiddenInShadow;
				}

				pAttachment->HideAttachment(1);
			}
		}
	}

	virtual void OnExit(float blendTime) 
	{
		if(m_attachmentCRC != 0)
		{
			if(IAttachment* pAttachment = m_charInstance->GetIAttachmentManager()->GetInterfaceByNameCRC(m_attachmentCRC))
			{
				pAttachment->HideAttachment((m_hiddenFlags & eHideFlags_HiddenInMainPass));
				pAttachment->HideInRecursion((m_hiddenFlags & eHideFlags_HiddenInRecursion));
				pAttachment->HideInShadow((m_hiddenFlags & eHideFlags_HiddenInShadow));
			}
		}
	}

	virtual void Update(float timePassed) {}

private:
	uint32 m_attachmentCRC;
	uint32 m_hiddenFlags;
};

CProceduralClipHideAttachment::CProceduralClipHideAttachment() : m_attachmentCRC(0), m_hiddenFlags(0)
{
}

CProceduralClipHideAttachment::~CProceduralClipHideAttachment()
{
}

CRYREGISTER_CLASS(CProceduralClipHideAttachment)