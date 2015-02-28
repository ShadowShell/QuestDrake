// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#include "IMFXEffect.h"
#include "IMaterialEffects.h"
#pragma once

struct SMFXParticleEntry 
{
  string name;
  string userdata;
  float scale; // base scale
  float maxdist; // max distance for spawning this effect
  float minscale; // min scale (distance == 0)
  float maxscale; // max scale (distance == maxscaledist)
  float maxscaledist; 
  bool  attachToTarget;

  SMFXParticleEntry()
	  : scale(1.0f)
	  , maxdist(0.0f)
	  , minscale(0.0f)
	  , maxscale(0.0f)
	  , maxscaledist(0.0f)
	  , attachToTarget(false)
  {
   
  }  

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(name);
		pSizer->AddObject(userdata);
	}
};

typedef std::vector<SMFXParticleEntry> SMFXParticleEntries;

struct SMFXParticleParams 
{  
  SMFXParticleEntries m_entries;
	MFXParticleDirection directionType;
};

class CMFXParticleEffect :
	public IMFXEffect
{
	typedef CryFixedStringT<32> TAttachmentName;

public:
	virtual void Execute(SMFXRunTimeEffectParams& params);
	virtual void ReadXMLNode(XmlNodeRef& node);
	virtual IMFXEffectPtr Clone();
	virtual void GetResources(SMFXResourceList& rlist);
	virtual void PreLoadAssets();
	CMFXParticleEffect();
	virtual ~CMFXParticleEffect();
	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_particleParams.m_entries);
		for (size_t i=0; i<m_particleParams.m_entries.size(); i++)
		{
			pSizer->AddObject(m_particleParams.m_entries[i].name);
			pSizer->AddObject(m_particleParams.m_entries[i].userdata);
		}
		IMFXEffect::GetMemoryUsage(pSizer);
	}
protected:

	bool AttachToTarget(const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale);
	bool AttachToCharacter(IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, const Vec3& dir, float scale);
	bool AttachToEntity(IEntity& targetEntity, const SMFXParticleEntry& particleParams, const SMFXRunTimeEffectParams& params, IParticleEffect* pParticleEffect, const Vec3& dir, float scale);

	void GetNextCharacterAttachmentName(TAttachmentName& attachmentName);

	SMFXParticleParams m_particleParams;
};
