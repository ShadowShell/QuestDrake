/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
History:
- 14:12:2011   Created by Jean Geffroy
*************************************************************************/

#include "StdAfx.h"

#include "ICryMannequin.h"
#include "ICryMannequinEditor.h"
#include <CryExtension/Impl/ClassWeaver.h>

#include "ParticleParams.h"

struct SPlayParticleEffectParams : SProceduralParams
{
	Vec3 posOffset;
	Ang3 rotOffset;
	float cloneAttachment;// Clone an attachment from the specified bone (so as to leave any existing attachment intact)
};

class CProceduralClipParticleEffect : public TProceduralClip<SPlayParticleEffectParams>
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IProceduralClip)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CProceduralClipParticleEffect, "ParticleEffect", 0xb3bb85112d6d4bbb, 0xad5dc5048a050127)

	virtual void OnEnter(float blendTime, float duration, const SPlayParticleEffectParams &params)
	{
		if ( gEnv->IsEditor() && gEnv->pGame->GetIGameFramework()->GetMannequinInterface().IsSilentPlaybackMode() ) 
			return;
		
		IEntity *pEntity = &m_scope->GetEntity();
		const char *szEffectName = params.dataString.c_str();
		if (IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(szEffectName, "Particle.SpawnEffect"))
		{
			IParticleEmitter *pEmitter = NULL;
			IAttachment* pAttachment = NULL;

			if (ICharacterInstance *pCharInst = m_scope->GetCharInst())
			{
				if (IAttachmentManager *pAttachmentManager = pCharInst->GetIAttachmentManager())
				{
					pAttachment = pAttachmentManager->GetInterfaceByNameCRC(params.dataCRC.crc);

					const char* szNewAttachmentName = NULL;
					int32 attachmentJointId = -1;
					const bool cloneExisting = params.cloneAttachment > 0.0f;
					if (cloneExisting && pAttachment && pAttachment->GetType() == CA_BONE)
					{
						// Clone an already existing attachment interface
						attachmentJointId = pAttachment->GetJointID();
						szNewAttachmentName = pAttachment->GetName();
					}
					else if (!pAttachment)
					{
						// No existing attachment interface: try to create a new, dedicated one if the given data string is a valid joint name
						attachmentJointId = pCharInst->GetIDefaultSkeleton().GetJointIDByCRC32(params.dataCRC.crc);
						szNewAttachmentName = params.dataCRC.GetString();
					}

					if (szNewAttachmentName && attachmentJointId >= 0)
					{
						// Create new attachment interface with a unique name
						static uint16 s_clonedAttachmentCount = 0; 
						++s_clonedAttachmentCount;
						CryFixedStringT<64> attachmentCloneName;
						attachmentCloneName.Format("%s%s%u", szNewAttachmentName, "FXClone", s_clonedAttachmentCount); 

						const char* pBoneName = pCharInst->GetIDefaultSkeleton().GetJointNameByID(attachmentJointId);
						const IAttachment* const pOriginalAttachment = pAttachment;
						pAttachment = pAttachmentManager->CreateAttachment(attachmentCloneName.c_str(), CA_BONE, pBoneName);
						m_ownedAttachmentNameCRC = pAttachment->GetNameCRC();

						if (!pOriginalAttachment)
						{
							// Attachment newly created from a joint: clear the relative transform
							pAttachment->AlignJointAttachment();
						}
						else
						{
							// Cloning an attachment: copy original transforms
							CRY_ASSERT(cloneExisting);
							pAttachment->SetAttAbsoluteDefault(pOriginalAttachment->GetAttAbsoluteDefault());
							pAttachment->SetAttRelativeDefault(pOriginalAttachment->GetAttRelativeDefault());
						}
					}

					if (pAttachment && (pAttachment->GetType() == CA_BONE))
					{
						const Matrix34 offsetMatrix(Vec3(1.0f), Quat::CreateRotationXYZ(params.rotOffset), params.posOffset);
						CEffectAttachment *pEffectAttachment = new CEffectAttachment(pEffect, offsetMatrix.GetTranslation(), offsetMatrix.GetColumn1(), 1.0f);
						pAttachment->AddBinding(pEffectAttachment);
						pAttachment->UpdateAttModelRelative();
						pEffectAttachment->ProcessAttachment(pAttachment);
						pEmitter = pEffectAttachment->GetEmitter();
					}
				}
			}

			if (!pEmitter && !pAttachment && pEntity)
			{
				if (pEffect->GetParticleParams().eAttachType != GeomType_None)
				{
					int slot = pEntity->LoadParticleEmitter(-1, pEffect);
					SEntitySlotInfo slotInfo;
					if (pEntity->GetSlotInfo(slot, slotInfo))
					{
						pEmitter = slotInfo.pParticleEmitter;
					}
				}
				else
				{
					const Matrix34 &transform = pEntity->GetWorldTM();
					const Matrix34 localOffset(Vec3(1.0f), Quat(params.rotOffset), params.posOffset);
					pEmitter = pEffect->Spawn(true, transform * localOffset);
				}
			}

			if (pEmitter)
			{
				pEmitter->AddRef();
				m_pEmitter = pEmitter;

				IMannequin &mannequinInterface = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
				uint32 numListeners = mannequinInterface.GetNumMannequinGameListeners();
				for (uint32 itListeners = 0; itListeners < numListeners; ++itListeners)
				{
					mannequinInterface.GetMannequinGameListener(itListeners)->OnSpawnParticleEmitter(pEmitter, m_scope->GetActionController());
				}
			}
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CProceduralClipParticleEffect: could not load requested effect %s", szEffectName);
		}
	}

	virtual void OnExit(float blendTime)
	{
		if (m_pEmitter)
		{
			if (ICharacterInstance *pCharInst = m_scope->GetCharInst())
			{
				if (IAttachmentManager *pAttachmentManager = pCharInst->GetIAttachmentManager())
				{
					if (IAttachment *pAttachment = pAttachmentManager->GetInterfaceByNameCRC((m_ownedAttachmentNameCRC ? m_ownedAttachmentNameCRC : GetParams().dataCRC.crc)))
					{
						IAttachmentObject *pAttachmentObject = pAttachment->GetIAttachmentObject();
						if (pAttachmentObject && (pAttachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_Effect))
						{
							CEffectAttachment *pEffectAttachment = (CEffectAttachment *)pAttachmentObject;
							if (pEffectAttachment->GetEmitter() == m_pEmitter)
							{
								pAttachment->ClearBinding();
							}
						}
					}
				}
			}

			SAFE_RELEASE(m_pEmitter);
		}

		if(m_ownedAttachmentNameCRC)
		{
			if (ICharacterInstance *pCharInst = m_scope->GetCharInst())
			{
				if (IAttachmentManager *pAttachmentManager = pCharInst->GetIAttachmentManager())
				{
					pAttachmentManager->RemoveAttachmentByNameCRC(m_ownedAttachmentNameCRC);
					m_ownedAttachmentNameCRC = 0; 
				}
			}
		}
	}

	virtual void Update(float timePassed)
	{
	}

private:
	IParticleEmitter *m_pEmitter;
	uint32 m_ownedAttachmentNameCRC;
};

CProceduralClipParticleEffect::CProceduralClipParticleEffect()
	:
	m_pEmitter(NULL),
	m_ownedAttachmentNameCRC(0)
{
}

CProceduralClipParticleEffect::~CProceduralClipParticleEffect()
{
}

CRYREGISTER_CLASS(CProceduralClipParticleEffect)


