////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   RenderProxy.h
//  Version:     v1.00
//  Created:     19/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __RenderProxy_h__
#define __RenderProxy_h__
#pragma once

#include "EntitySystem.h"
#include <IRenderer.h>
#include <IEntityRenderState.h>

// forward declarations.
class CEntityObject;
class CEntity;
struct SRendParams;
struct IShaderPublicParams;
class CEntityTimeoutList;
class CRenderProxy;
struct AnimEventInstance;

//////////////////////////////////////////////////////////////////////////
// Description:
//    CRenderProxy provides default implementation of IEntityRenderProxy interface.
//    This is a renderable object that is registered in 3D engine sector.
//    It can contain multiple sub object slots that can have their own relative transformation, and 
//    each slot can represent specific renderable interface (IStatObj,ICharacterInstance,etc..)
//    Slots can also form hierarchical transformation tree, every slot can reference parent slot for transformation.
///////////////////////////////////////////////////////////// /////////////
class CRenderProxy : public IRenderNode, public IEntityRenderProxy
{
public:

	enum EFlags // Must be limited to 32 flags.
	{
		FLAG_CUSTOM_POST_EFFECT = BIT(0), // Has custom post effect ID stored in custom data
		FLAG_BBOX_VALID_LOCAL  = BIT(1),
		FLAG_BBOX_FORCED       = BIT(2),
		FLAG_BBOX_INVALID      = BIT(3),
		FLAG_HIDDEN            = BIT(4), // If render proxy is hidden.
		//FLAG_HAS_ENV_LIGHTING  = 0x0020, // If render proxy have environment lighting.
		FLAG_UPDATE            = BIT(5), // If render proxy needs to be updated.
		FLAG_NOW_VISIBLE       = BIT(6), // If render proxy currently visible.
		FLAG_REGISTERED_IN_3DENGINE = BIT(7), // If render proxy have been registered in 3d engine.
		FLAG_POST_INIT         = BIT(8), // If render proxy have received Post init event.
		FLAG_HAS_LIGHTS        = BIT(9), // If render proxy has some lights.
		FLAG_GEOMETRY_MODIFIED = BIT(10), // Geometry for this render slot was modified at runtime.
		FLAG_HAS_CHILDRENDERNODES = BIT(11), // If render proxy contains child render nodes
		FLAG_HAS_PARTICLES     = BIT(12), // If render proxy contains particle emitters
		FLAG_SHADOW_DISSOLVE   = BIT(13), // If render proxy requires dissolving shadows
		FLAG_FADE_CLOAK_BY_DISTANCE   = BIT(14), // If render proxy requires fading cloak by distance
		FLAG_IGNORE_HUD_INTERFERENCE_FILTER  = BIT(15), // HUD render proxy ignores hud interference filter post effect settings
		FLAG_IGNORE_HEAT_VALUE = BIT(16), // Don't appear hot in nano vision
		FLAG_POST_3D_RENDER  = BIT(17), // Render proxy in post 3D pass
		FLAG_IGNORE_CLOAK_REFRACTION_COLOR  = BIT(18), // Will ignore cloak refraction color
		FLAG_HUD_REQUIRE_DEPTHTEST  = BIT(19), // If 3D HUD Object requires to be rendered at correct depth (i.e. behind weapon)
		FLAG_CLOAK_INTERFERENCE  = BIT(20), // When set the cloak will use the cloak interference parameters
		FLAG_CLOAK_HIGHLIGHTS  = BIT(21), // When set the cloak will use the cloak highlight parameters
		FLAG_HUD_DISABLEBLOOM  = BIT(22), // Allows 3d hud object to disable bloom (Required to avoid overglow and cutoff with alien hud ghosted planes)
		FLAG_ANIMATE_OFFSCREEN_SHADOW  = BIT(23), // Update the animation if object drawn in the shadow pass
		FLAG_DISABLE_MOTIONBLUR  = BIT(24), // Disable motion blur
		FLAG_EXECUTE_AS_JOB_FLAG = BIT(25),	// set if this CRenderProxy can be executed as a Job from the 3DEngine
		FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG = BIT(26), // set if the slots changed, to recheck if this renderproxy can execute as a job
	};

	CRenderProxy();
	~CRenderProxy();

	EERType GetRenderNodeType();
	virtual void GetMemoryUsage(ICrySizer *pSizer ) const;
	virtual bool CanExecuteRenderAsJob();

	// Must be called somewhen after constructor.
	void PostInit();

 	//////////////////////////////////////////////////////////////////////////
	// IEntityEvent implementation.
 	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize( const SComponentInitializer& init );
	virtual	void ProcessEvent( SEntityEvent &event );
 	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() { return ENTITY_PROXY_RENDER; }
	virtual void Release();
	virtual void Done() {};
	// Called when the subsystem initialize.
	virtual bool Init( IEntity *pEntity,SEntitySpawnParams &params ) { return true; }
	virtual void Reload( IEntity *pEntity,SEntitySpawnParams &params );
	virtual	void Update( SEntityUpdateContext &ctx );
	virtual void SerializeXML( XmlNodeRef &entityNode,bool bLoading );
	virtual void Serialize( TSerialize ser );
	virtual bool NeedSerialize();
	virtual bool GetSignature( TSerialize signature );
	virtual void SetViewDistRatio(int nViewDistRatio);
	virtual void SetLodRatio(int nLodRatio);
	//////////////////////////////////////////////////////////////////////////

  virtual void SetMinSpec(int nMinSpec);

	virtual bool PhysicalizeFoliage(bool bPhysicalize=true, int iSource=0, int nSlot=0);
	virtual IPhysicalEntity* GetBranchPhys(int idx, int nSlot=0) { IFoliage *pFoliage = GetFoliage(); return pFoliage ? pFoliage->GetBranchPhysics(idx) : 0; }
	virtual struct IFoliage* GetFoliage(int nSlot=0);

	//////////////////////////////////////////////////////////////////////////
	// IEntityRenderProxy interface.
	//////////////////////////////////////////////////////////////////////////
	// Description:
	//    Force local bounds.
	// Arguments:
	//    bounds - Bounding box in local space.
	//    bDoNotRecalculate - when set to true entity will never try to recalculate local bounding box set by this call.
	virtual void SetLocalBounds( const AABB &bounds,bool bDoNotRecalculate );
	virtual void GetWorldBounds( AABB &bounds );
	virtual void GetLocalBounds( AABB &bounds );
	virtual void InvalidateLocalBounds();

	//////////////////////////////////////////////////////////////////////////
	virtual IMaterial* GetRenderMaterial( int nSlot=-1 );
  // Set shader parameters for this instance
  //virtual void SetShaderParam(const char *pszName, UParamVal &pParam, int nType = 0) ;

	virtual struct IRenderNode* GetRenderNode() { return this; };
	virtual IShaderPublicParams* GetShaderPublicParams( bool bCreate=true );

	virtual void AddShaderParamCallback(IShaderParamCallbackPtr pCallback);
	virtual bool RemoveShaderParamCallback(IShaderParamCallbackPtr pCallback);
	virtual void CheckShaderParamCallbacks();
	virtual void ClearShaderParamCallbacks();

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityRenderProxy implementation.
	//////////////////////////////////////////////////////////////////////////
	bool IsSlotValid( int nIndex ) const { return nIndex >=0 && nIndex < (int)m_slots.size() && m_slots[nIndex] != NULL; };
	int  GetSlotCount() const;
	bool SetParentSlot( int nParentIndex,int nChildIndex );
	bool GetSlotInfo( int nIndex,SEntitySlotInfo &slotInfo ) const;
	void SetSlotMaterial( int nSlot,IMaterial *pMaterial );
	IMaterial* GetSlotMaterial( int nSlot );
	void SetSubObjHideMask( int nSlot,uint64 nSubObjHideMask );
	uint64 GetSubObjHideMask( int nSlot ) const;
	void ClearSlots() { FreeAllSlots(); };

	void SetSlotFlags( int nSlot,uint32 nFlags );
	uint32 GetSlotFlags( int nSlot );

	ICharacterInstance* GetCharacter( int nSlot );
	IStatObj* GetStatObj( int nSlot );
	IParticleEmitter* GetParticleEmitter( int nSlot );
#if defined(USE_GEOM_CACHES)
	IGeomCacheRenderNode* GetGeomCacheRenderNode( int nSlot );
#endif

	int SetSlotGeometry( int nSlot,IStatObj *pStatObj );
  void QueueSlotGeometryChange( int nSlot,IStatObj *pStatObj );
	int SetSlotCharacter( int nSlot, ICharacterInstance* pCharacter );
	int LoadGeometry( int nSlot,const char *sFilename,const char *sGeomName=NULL,int nLoadFlags=0 );
	int LoadCharacter( int nSlot,const char *sFilename,int nLoadFlags=0 );
	int	LoadParticleEmitter( int nSlot, IParticleEffect *pEffect, SpawnParams const* params=NULL, bool bPrime = false, bool bSerialize = false );
	int	SetParticleEmitter( int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false );
	int LoadLight( int nSlot,CDLight *pLight,bool isCheapLight,uint16 layerId );
#if !defined(RENDERNODES_LEAN_AND_MEAN)
	int LoadCloud( int nSlot,const char *sFilename );
	int SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties);
#endif
	int LoadFogVolume( int nSlot, const SFogVolumeProperties& properties );
	int LoadLightShape( int nSlot, const char *sFilename, const uint32 nLSFlags );
	int FadeGlobalDensity( int nSlot, float fadeTime, float newGlobalDensity );
#if defined(USE_GEOM_CACHES)
	int LoadGeomCache( int nSlot, const char *sFilename );
#endif

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
	int LoadPrismObject(int nSlot);
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

#if !defined(RENDERNODES_LEAN_AND_MEAN)
	int LoadVolumeObject(int nSlot, const char* sFilename);
	int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties);
#endif

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Slots.
	//////////////////////////////////////////////////////////////////////////
	ILINE CEntityObject* GetSlot( int nIndex ) const { return (nIndex >=0 && nIndex < (int)m_slots.size()) ? m_slots[nIndex] : NULL; }
	CEntityObject* GetParentSlot( int nIndex ) const;

	// Allocate a new object slot.
	CEntityObject* AllocSlot( int nIndex );
	// Frees existing object slot, also optimizes size of slot array.
	void FreeSlot( int nIndex );
	void FreeAllSlots();
	void ExpandCompoundSlot0();	// if there's a compound cgf in slot 0, expand it into subobjects (slots)
	
	// Returns world transformation matrix of the object slot.
	const Matrix34& GetSlotWorldTM( int nIndex ) const;
	// Returns local relative to host entity transformation matrix of the object slot.
	const Matrix34& GetSlotLocalTM( int nIndex, bool bRelativeToParent) const;
	// Set local transformation matrix of the object slot.
	void SetSlotLocalTM( int nIndex,const Matrix34 &localTM,int nWhyFlags=0 );
	// Set camera space position of the object slot
	void SetSlotCameraSpacePos( int nIndex, const Vec3 &cameraSpacePos );
	// Get camera space position of the object slot
	void GetSlotCameraSpacePos( int nSlot, Vec3 &cameraSpacePos ) const;

	// Invalidates local or world space bounding box.
	void InvalidateBounds( bool bLocal,bool bWorld );

	// Register render proxy in 3d engine.	
	void RegisterForRendering( bool bEnable );
	static void RegisterCharactersForRendering();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// IRenderNode interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void SetLayerId(uint16 nLayerId);
	virtual void SetMaterial( IMaterial *pMat );
	virtual IMaterial* GetMaterial(Vec3 * pHitPos = NULL);
	virtual IMaterial* GetMaterialOverride() { return m_pMaterial; }
	virtual const char * GetEntityClassName() const;
	virtual Vec3 GetPos(bool bWorldOnly = true) const;
	virtual const Ang3& GetAngles(int realA=0) const;
	virtual float GetScale() const;
	virtual const char *GetName() const;
	virtual void GetRenderBBox( Vec3 &mins,Vec3 &maxs );
	virtual void GetBBox( Vec3 &mins,Vec3 &maxs );
	virtual void FillBBox(AABB & aabb);
	virtual bool HasChanged() { return false; }
	virtual void Render(const struct SRendParams & EntDrawParams, const SRenderingPassInfo &passInfo);
	virtual struct IStatObj *GetEntityStatObj( unsigned int nPartId, unsigned int nSubPartId, Matrix34A * pMatrix = NULL, bool bReturnOnlyVisible = false);
	virtual void SetEntityStatObj( unsigned int nSlot, IStatObj * pStatObj, const Matrix34A * pMatrix = NULL ) {};
	virtual IMaterial * GetEntitySlotMaterial( unsigned int nPartId, bool bReturnOnlyVisible, bool * pbDrawNear );
	virtual struct ICharacterInstance* GetEntityCharacter( unsigned int nSlot, Matrix34A * pMatrix = NULL, bool bReturnOnlyVisible = false );
#if defined(USE_GEOM_CACHES)
	virtual IGeomCacheRenderNode* GetGeomCacheRenderNode( unsigned int nSlot, Matrix34A * pMatrix = NULL, bool bReturnOnlyVisible = false );
#endif
	virtual bool IsRenderProxyVisAreaVisible() const;
	virtual struct IPhysicalEntity* GetPhysics() const;
	virtual void SetPhysics( IPhysicalEntity* pPhys ) {};
	virtual void PreloadInstanceResources(Vec3 vPrevPortalPos, float fPrevPortalDistance, float fTime) {};
  
	void Render_JobEntry( const SRendParams inRenderParams, const SRenderingPassInfo passInfo );

	virtual float GetMaxViewDist();

  virtual void SetMaterialLayersMask( uint8 nMtlLayers );
  virtual uint8 GetMaterialLayersMask() const 
  { 
    return m_nMaterialLayers; 
  }

  virtual void SetMaterialLayersBlend( uint32 nMtlLayersBlend )
  {
    m_nMaterialLayersBlend = nMtlLayersBlend; 
  }

  virtual uint32 GetMaterialLayersBlend( ) const
  {
    return m_nMaterialLayersBlend;
  }

	virtual void SetCloakHighlightStrength( float highlightStrength )
	{
		m_fCustomData[0] = clamp_tpl(highlightStrength,0.0f,1.0f);

		if(highlightStrength > 0.0f)
		{
			AddFlags(FLAG_CLOAK_HIGHLIGHTS); 
		}
		else
		{
			ClearFlags(FLAG_CLOAK_HIGHLIGHTS);
		}
	}

	virtual void SetMotionBlur(bool enable)
	{
		if( enable )
		{
			AddFlags(FLAG_DISABLE_MOTIONBLUR); 
		}
		else
		{
			ClearFlags(FLAG_DISABLE_MOTIONBLUR);
		}
	}

	virtual void SetCloakInterferenceState( bool bHasCloakInterference )
	{
		if(bHasCloakInterference)
		{
			AddFlags(FLAG_CLOAK_INTERFERENCE); 
		}
		else
		{
			ClearFlags(FLAG_CLOAK_INTERFERENCE);
		}
	}

	virtual bool GetCloakInterferenceState( ) const
	{
		return (m_nFlags & FLAG_CLOAK_INTERFERENCE) ? true : false;
	}

	virtual void SetCloakColorChannel( uint8 nCloakColorChannel )
	{
		m_nCloakColorChannel = nCloakColorChannel;
	}

	virtual uint8 GetCloakColorChannel( ) const
	{
		return m_nCloakColorChannel;
	}

	virtual void SetCloakFadeByDistance( bool bCloakFadeByDistance )
	{
		if(bCloakFadeByDistance)
		{
			AddFlags(FLAG_FADE_CLOAK_BY_DISTANCE); 
		}
		else
		{
			ClearFlags(FLAG_FADE_CLOAK_BY_DISTANCE);
		}
	}

	virtual bool DoesCloakFadeByDistance( ) const
	{
		return (m_nFlags & FLAG_FADE_CLOAK_BY_DISTANCE) ? true : false;
	}

	virtual void SetCloakBlendTimeScale( float fCloakBlendTimeScale )
	{
		m_nCloakBlendTimeScale = (uint8) min(255.0f, max(1.0f, fCloakBlendTimeScale*64.0f)); 
	}

	virtual float GetCloakBlendTimeScale( ) const
	{
		return ((float)m_nCloakBlendTimeScale) * (1.0f / 64.0f) ;
	}

	virtual void SetIgnoreCloakRefractionColor(bool bIgnoreCloakRefractionColor)
	{
		if(bIgnoreCloakRefractionColor)
		{
			AddFlags(FLAG_IGNORE_CLOAK_REFRACTION_COLOR); 
		}
		else
		{
			ClearFlags(FLAG_IGNORE_CLOAK_REFRACTION_COLOR);
		}
	}

	virtual bool DoesIgnoreCloakRefractionColor() const
	{
		return (m_nFlags & FLAG_IGNORE_CLOAK_REFRACTION_COLOR) ? true : false;
	}

	virtual void SetCustomPostEffect( const char* pPostEffectName );

	virtual void SetAsPost3dRenderObject( bool bPost3dRenderObject, uint8 groupId, float groupScreenRect[4] );
	
	virtual void SetIgnoreHudInterferenceFilter(const bool bIgnoreFiler)
	{
		if(bIgnoreFiler)
		{
			AddFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER); 
		}
		else
		{
			ClearFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER);
		}
	}

	virtual void SetHUDRequireDepthTest(const bool bRequire)
	{
		if(bRequire)
		{
			AddFlags(FLAG_HUD_REQUIRE_DEPTHTEST); 
		}
		else
		{
			ClearFlags(FLAG_HUD_REQUIRE_DEPTHTEST);
		}
	}

	virtual void SetHUDDisableBloom(const bool bDisableBloom)
	{
		if(bDisableBloom)
		{
			AddFlags(FLAG_HUD_DISABLEBLOOM); 
		}
		else
		{
			ClearFlags(FLAG_HUD_DISABLEBLOOM);
		}
	}

	virtual void SetIgnoreHeatAmount( bool bIgnoreHeat )
	{
		if (bIgnoreHeat)
		{
			AddFlags(FLAG_IGNORE_HEAT_VALUE);
		}
		else
		{
			ClearFlags(FLAG_IGNORE_HEAT_VALUE);
		}
	}

  //! Allows to adjust vision params:
  virtual void SetVisionParams( float r, float g, float b, float a ) 
  { 
    m_nVisionParams =  (uint32) (int_round( r * 255.0f) << 24)| (int_round( g * 255.0f) << 16)| (int_round( b * 255.0f) << 8) | (int_round( a * 255.0f));
  }

  //! return vision params
  virtual uint32 GetVisionParams() const { return m_nVisionParams; }
  
	//! Allows to adjust hud silhouettes params:
	// . binocular view uses color and alpha for blending
	virtual void SetHUDSilhouettesParams( float r, float g, float b, float a ) 
	{ 
		m_nHUDSilhouettesParams =  (uint32) (int_round( r * 255.0f) << 24)| (int_round( g * 255.0f) << 16)| (int_round( b * 255.0f) << 8) | (int_round( a * 255.0f));
	}

	//! return vision params
	virtual uint32 GetHUDSilhouettesParams() const { return m_nHUDSilhouettesParams; }
  
  //! set shadow dissolve state
  virtual void SetShadowDissolve(bool enable)
  {
		if(enable)
		 AddFlags(FLAG_SHADOW_DISSOLVE); 
		else
		 ClearFlags(FLAG_SHADOW_DISSOLVE);
  }
  
  //! return shadow dissolve state
  virtual bool GetShadowDissolve() const { return CheckFlags(FLAG_SHADOW_DISSOLVE);};

	//! Allows to adjust effect layer params
	virtual void SetEffectLayerParams( const Vec4 &pParams ) 
	{ 
		m_pLayerEffectParams = (uint32) (int_round( pParams.x * 255.0f) << 24)| (int_round( pParams.y * 255.0f) << 16)| (int_round( pParams.z * 255.0f) << 8) | (int_round( pParams.w * 255.0f));
	}

	//! Allows to adjust effect layer params
	virtual void SetEffectLayerParams( uint32 nEncodedParams  ) 
	{ 
		m_pLayerEffectParams = nEncodedParams;
	}

	//! return effect layer params
	virtual const uint32 GetEffectLayerParams() const { return m_pLayerEffectParams; }

  virtual void SetOpacity(float fAmount) 
  {     
    m_nOpacity = int_round(clamp_tpl<float>(fAmount, 0.0f, 1.0f) * 255);
  }

  virtual float GetOpacity() const {  return (float) m_nOpacity / 255.0f; }

  virtual const AABB GetBBox() const { return m_WSBBox; }
  virtual void SetBBox( const AABB& WSBBox ) { m_WSBBox = WSBBox; }
  virtual void OffsetPosition(const Vec3& delta) {}

	virtual float	GetLastSeenTime() const { return m_fLastSeenTime; }
	//////////////////////////////////////////////////////////////////////////
	
	// Internal slot access function.
	ILINE CEntityObject* Slot( int nIndex ) const { assert(nIndex >=0 && nIndex < (int)m_slots.size()); return m_slots[nIndex]; }
	IStatObj *GetCompoundObj() const;

	//////////////////////////////////////////////////////////////////////////
	// Flags
	//////////////////////////////////////////////////////////////////////////
	ILINE void   SetFlags( uint32 flags ) { m_nFlags = flags; };
	ILINE uint32 GetFlags() { return m_nFlags; };
	ILINE void   AddFlags( uint32 flagsToAdd ) { SetFlags( m_nFlags | flagsToAdd ); };
	ILINE void   ClearFlags( uint32 flagsToClear ) { SetFlags( m_nFlags & ~flagsToClear ); };
	ILINE bool   CheckFlags( uint32 flagsToCheck ) const { return (m_nFlags&flagsToCheck) == flagsToCheck; };
	//////////////////////////////////////////////////////////////////////////

	// Change custom material.
	void SetCustomMaterial( IMaterial *pMaterial );
	// Get assigned custom material.
	IMaterial* GetCustomMaterial() { return m_pMaterial; }

	void	UpdateEntityFlags();
	void	SetLastSeenTime(float fNewTime) { m_fLastSeenTime=fNewTime; }

	void  DebugDraw( const SGeometryDebugDrawInfo &info );
	
	//////////////////////////////////////////////////////////////////////////
	// Custom new/delete.
	//////////////////////////////////////////////////////////////////////////
	void *operator new( size_t nSize );
	void operator delete( void *ptr );

	static void SetTimeoutList(CEntityTimeoutList* pList)	{s_pTimeoutList = pList;}

	static ILINE void SetViewDistMin(float fViewDistMin)                  { s_fViewDistMin = fViewDistMin; }
	static ILINE void SetViewDistRatio(float fViewDistRatio)              { s_fViewDistRatio = fViewDistRatio; }
	static ILINE void SetViewDistRatioCustom(float fViewDistRatioCustom)  { s_fViewDistRatioCustom = fViewDistRatioCustom; }
	static ILINE void SetViewDistRatioDetail(float fViewDistRatioDetail)  { s_fViewDistRatioDetail = fViewDistRatioDetail; }	

	static int AnimEventCallback(ICharacterInstance *pCharacter, void *userdata);

private:
	int GetSlotId( CEntityObject *pSlot ) const;
	void CalcLocalBounds();
	void OnEntityXForm( int nWhyFlags );
	void OnHide( bool bHide );
  void OnReset( );

	// Get existing slot or make a new slot if not exist.
	// Is nSlot is negative will allocate a new available slot and return it Index in nSlot parameter.
	CEntityObject* GetOrMakeSlot( int &nSlot );

	I3DEngine* GetI3DEngine() { return gEnv->p3DEngine; }
	bool CheckCharacterForMorphs(ICharacterInstance* pCharacter);
	void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event);
  void UpdateMaterialLayersRendParams( SRendParams &pRenderParams, const SRenderingPassInfo &passInfo );
	void UpdateEffectLayersParams( SRendParams &pRenderParams );
  bool IsMovableByGame() const;

private:
	friend class CEntityObject;

	static float gsWaterLevel;	//static cached water level, updated each ctor call
	
	static float s_fViewDistMin;
	static float s_fViewDistRatio;
	static float s_fViewDistRatioCustom;
	static float s_fViewDistRatioDetail;
	static std::vector<CRenderProxy*> s_arrCharactersToRegisterForRendering;

	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity *m_pEntity;

	// Object Slots.
	typedef std::vector<CEntityObject*> Slots;
	Slots m_slots;

	// Local bounding box of render proxy.
	AABB m_localBBox;

	//! Override material.
	_smart_ptr<IMaterial> m_pMaterial;

  // per instance shader public params
	_smart_ptr<IShaderPublicParams>	m_pShaderPublicParams;

	// per instance callback object
	typedef std::vector<IShaderParamCallbackPtr> TCallbackVector;
	TCallbackVector m_Callbacks;
  
	// Time passed since this entity was seen last time  (wrong: this is an absolute time, todo: fix float absolute time values)
	float		m_fLastSeenTime;

	static CEntityTimeoutList* s_pTimeoutList;
	EntityId m_entityId;

	uint32 m_nEntityFlags;

	// Render proxy flags.
	uint32 m_nFlags;

  // Vision modes stuff
  uint32 m_nVisionParams;
	// Hud silhouetes param
	uint32 m_nHUDSilhouettesParams;
  // Material layers blending amount
  uint32 m_nMaterialLayersBlend;

  typedef std::pair<int, IStatObj*> SlotGeometry;
	typedef std::vector<SlotGeometry> SlotGeometries;
	SlotGeometries m_queuedGeometryChanges;

	// Layer effects
	uint32 m_pLayerEffectParams;

	float m_fCustomData[4];

	uint8	m_nCloakBlendTimeScale;
	uint8 m_nOpacity;
	uint8 m_nCloakColorChannel;
	uint8 m_nCustomData;

  AABB m_WSBBox;
};

extern stl::PoolAllocatorNoMT<sizeof(CRenderProxy)> *g_Alloc_RenderProxy;

//////////////////////////////////////////////////////////////////////////
DECLARE_COMPONENT_POINTERS( CRenderProxy );

//////////////////////////////////////////////////////////////////////////
inline void* CRenderProxy::operator new( size_t nSize )
{
	void *ptr = g_Alloc_RenderProxy->Allocate();
	return ptr;
}

//////////////////////////////////////////////////////////////////////////
inline void CRenderProxy::operator delete( void *ptr )
{
	if (ptr)
		g_Alloc_RenderProxy->Deallocate(ptr);
}

#endif // __RenderProxy_h__
