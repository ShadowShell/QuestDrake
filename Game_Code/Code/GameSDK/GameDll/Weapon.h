/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Weapon Implementation

-------------------------------------------------------------------------
History:
- 22:8:2005   12:50 : Created by Márcio Martins

*************************************************************************/
#ifndef __WEAPON_H__
#define __WEAPON_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IItemSystem.h>
#include <IWeapon.h>
#include <IAgent.h>
#include <VectorMap.h>
#include <IMusicSystem.h>
#include "Item.h"
#include "EntityUtility/EntityEffectsHeat.h"
#include "ScopeReticule.h"
#include "Utility/CryHash.h"
#include "ICryAnimation.h"
#include "PlayerPlugin_Interaction.h"
#include "ShotDebug.h"
#include "UI/UITypes.h"

#define WEAPON_FADECROSSHAIR_SELECT	(0.250f)
#define WEAPON_SHOOT_TIMER					(5000)

#define CHECK_OWNER_REQUEST()	\
	{ \
	uint16 channelId=m_pGameFramework->GetGameChannelId(pNetChannel);	\
	IActor *pOwnerActor=GetOwnerActor(); \
	if (pOwnerActor && pOwnerActor->GetChannelId()!=channelId && !IsDemoPlayback()) \
	return true; \
	}

class CFireMode;
class CProjectile;
class CWeaponSharedParams;
class CIronSight;
class CWeaponStats;
struct SZoomModeParams;
struct SFireModeParams;
struct SMeleeModeParams;
struct SParams_WeaponFPAiming;

struct SHazardDescriptor;

class CMelee;

//------------------------------------------------------------------------
class CWeapon :
	public CItem,
	public IWeapon
{
	class ScheduleLayer_Leave;
	class ScheduleLayer_Enter;
	struct EndChangeFireModeAction;
	struct MeleeReactionTimer;
	struct AnimationEventFireAutoStop;
	struct RefillBeltAction;

	typedef CItem	BaseClass;

	//------------------------------------------------------------------------
	// Used for firing animation events 
	class CAnimationFiringLocator : public IWeaponFiringLocator
	{
	public:
		CAnimationFiringLocator();
		bool				Init(CWeapon* pOwnerWeapon);

		void				Set();
		// Manages situations in which other systems want to set a firing locator while this is active
		void				SetOtherFiringLocator(IWeaponFiringLocator* pFiringLocator);
		ILINE bool	IsSet() const { CRY_ASSERT(m_pOwnerWeapon); return m_pOwnerWeapon->GetFiringLocator() == this; }
		void				Release();

		// IWeaponFiringLocator
		virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit);
		virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos) { return false; }
		virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
		virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos) { return GetFiringDir(weaponId, pFireMode, dir, probableHit, firingPos); }
		virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir) { return false; }
		virtual void WeaponReleased() {}
		// ~IWeaponFiringLocator

	private:
		IWeaponFiringLocator* m_pPreviousFiringLocator;
		CWeapon* m_pOwnerWeapon;

	};

protected:
	typedef VectorMap<CryHashStringId, int>					TFireModeIdMap;
	typedef std::vector<CFireMode *>						TFireModeVector;
	typedef VectorMap<CryHashStringId, int>					TZoomModeIdMap;
	typedef VectorMap<int, string>					TZoomModeNameMap;
	typedef std::vector<IZoomMode *>						TZoomModeVector;

	typedef CListenerSet<IWeaponEventListener*>	TWeaponEventListeners;

public:

	enum EWeaponActions
	{
		eWeaponAction_None	 = BIT(0),
		eWeaponAction_Fire	 = BIT(1),
		eWeaponAction_Zoom	 = BIT(2),
		eWeaponAction_Reload = BIT(3),
		eWeaponAction_Melee	 = BIT(4),
	};

	enum EWeaponCrosshair
	{
		eWeaponCrossHair_Default,
		eWeaponCrossHair_ForceOff,
	};

public:
	static void StaticReset();

public:
	CWeapon();
	virtual ~CWeapon();

	// IItem, IGameObjectExtension
	virtual bool Init(IGameObject * pGameObject);
	virtual void InitClient(int channelId) { CItem::InitClient(channelId); };
	virtual void Release();
	virtual void FullSerialize( TSerialize ser );
	virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	virtual NetworkAspectType GetNetSerializeAspects();
	virtual void PostSerialize();
	virtual void SerializeLTL(TSerialize ser);
	virtual void Update(SEntityUpdateContext& ctx, int);
	virtual void PostUpdate( float frameTime );
	virtual void ProcessEvent(SEntityEvent& event);
	virtual void HandleEvent(const SGameObjectEvent &evt);
	virtual void SetChannelId(uint16 id) {};
	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void Reset();
	virtual bool ResetParams();
	virtual void PreResetParams();
	virtual float GetMeleeRange() const;

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView(float frameTime);

	virtual IWeapon *GetIWeapon() { return this; };
	virtual const IWeapon *GetIWeapon() const { return this; };

	virtual void MeleeAttack(bool bShort = false);
	virtual bool CanMeleeAttack() const;

	virtual IFireMode *GetMeleeFireMode() const { CRY_ASSERT_MESSAGE(0, "DEPRECATED FUNCTIONALITY: use GetMelee() instead"); return NULL; };
	CMelee* GetMelee() const { return m_melee; }

	virtual void Select(bool select);
	virtual void PickUp(EntityId picker, bool sound, bool select=true, bool keepHistory=true, const char *setup = NULL);
	virtual void Drop(float impulseScale, bool selectNext = true, bool byDeath = false);

	virtual bool CanDrop() const;

	virtual void OnPickedUp(EntityId actorId, bool destroyed);
	virtual void OnDropped(EntityId actorId, bool ownerWasAI);

  virtual void OnDestroyed();

	virtual void FumbleGrenade() {};
	virtual void StartQuickGrenadeThrow() {};
	virtual void StopQuickGrenadeThrow() {};

	virtual void Use(EntityId userId);

	//Needed for the mounted weapon
	virtual void StartUse(EntityId userId); 
	virtual void StopUse(EntityId userId); 

	virtual bool CheckAmmoRestrictions(IInventory *pInventory);

	virtual bool FilterView(SViewParams &viewParams);
	virtual void PostFilterView(struct SViewParams &viewParams);

	virtual void GetFPOffset(QuatT &offset) const;

	// ~IItem
	virtual bool HasAttachmentAtHelper(const char *helper);
	virtual void GetAttachmentsAtHelper(const char *helper, CCryFixedStringListT<5, 30> &attachments);

	virtual	uint32 StartDeselection(bool fastDeselect);
	virtual void CancelDeselection();
	virtual bool IsDeselecting() const;

	virtual void PickUpAmmo(EntityId pickerId);
	virtual bool HasSomeAmmoToPickUp(EntityId pickerId) const;
	virtual ColorF GetSilhouetteColor() const;
	void AddShootHeatPulse(CActor* pOwnerActor, const float heatWeapon, const float weaponHeatTime, const float heatOwner, const float ownerHeatTime);

	// Events
	virtual void OnShoot(EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, 
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(EntityId shooterId);
	virtual void OnStopFire(EntityId shooterId);
	virtual void OnFireModeChanged(int currentFireMode);
	virtual void OnStartReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnEndReload(EntityId shooterId, IEntityClass* pAmmoType);
	virtual void OnSetAmmoCount(EntityId shooterId);
	virtual void OnOutOfAmmo(IEntityClass* pAmmoType);
	virtual void OnReadyToFire();
	virtual void OnMelee(EntityId shooterId);
	virtual void OnStartTargetting(IWeapon *pWeapon);
	virtual void OnStopTargetting(IWeapon *pWeapon);
	virtual void OnSelected(bool selected);
	virtual void OnProjectileCollided(EntityId projectileId, IPhysicalEntity* pCollider, const Vec3& pos) {};
	virtual void OnEnterFirstPerson();
	virtual void OnFireWhenOutOfAmmo();
	void				 OutOfAmmoDeselect();
	void				 OutOfAmmoType();
	void				 SetToDefaultFireModeIfNeeded(const CActor& ownerActor);
	void				 OnAnimationEventStartFire(const char* szCustomParameter);
	void				 OnAnimationEventStopFire();
	virtual void OnAnimationEventShootGrenade(const AnimEventInstance &event) {};
	bool				 RefillAllAmmo(const char* refillType, bool refillAll = false);
	virtual void OnZoomChanged(bool zoomed, int idx);

	// IWeapon
	virtual void SetFiringLocator(IWeaponFiringLocator *pLocator);
	virtual IWeaponFiringLocator *GetFiringLocator() const;

	virtual void AddEventListener(IWeaponEventListener *pListener, const char *who);
	virtual void RemoveEventListener(IWeaponEventListener *pListener);

  virtual void SetDestinationEntity(EntityId targetId);
  virtual void SetDestination(const Vec3& pos){ m_destination = pos; }
	virtual const Vec3& GetDestination(){ return m_destination; }

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const;
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const;
	
	virtual void StartFire();
	virtual void StartFire(const SProjectileLaunchParams& launchParams);
	virtual void StopFire();
	virtual bool CanFire() const;
	virtual bool CanStopFire() const;

	virtual void StartZoom(EntityId shooterId, int zoomed = 0);
	virtual void StopZoom(EntityId shooterId);
	virtual bool CanZoom() const;
	virtual void ExitZoom(bool force=false);
	virtual bool IsZoomed() const;
	virtual bool IsZoomingInOrOut() const;
	virtual EZoomState GetZoomState() const;
	float	GetZoomTransition() const;
	virtual bool CanModify() const;
	virtual bool CanSprint() const { return (!IsBusy() && !IsFiring()) || ( gEnv->bMultiplayer && IsReloading() ); }  //Allowing sprinting while reloading in MP now

	bool IsFiring() const;
	virtual bool IsReloading(bool includePending=true) const;
	
	virtual void MountAt(const Vec3 &pos);
	virtual void MountAtEntity(EntityId entityId, const Vec3 &pos, const Ang3 &angles);

	virtual void Reload(bool force=false);
	virtual bool CanReload() const;

	virtual bool OutOfAmmo(bool allFireModes) const;
	virtual bool OutOfAmmoTypes() const;
	virtual bool LowAmmo(float thresholdPerCent) const;

	void SetBonusAmmoCount(IEntityClass* pAmmoType, int amount);
	int GetBonusAmmoCount(IEntityClass* pAmmoType) const;
	virtual int GetAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetAmmoCount(IEntityClass* pAmmoType, int count);
	bool CanPickUpAmmo(IInventory* pDestinationInventory);
	
	virtual int GetInventoryAmmoCount(IEntityClass* pAmmoType) const;
	virtual void SetInventoryAmmoCount(IEntityClass* pAmmoType, int count);

	virtual int GetNumOfFireModes() const { return m_firemodes.size(); }
	virtual IFireMode* GetFireMode(int idx) const;
	virtual IFireMode* GetFireMode(const char *name) const;
	CFireMode* GetCFireMode(int idx) const;
	CFireMode* GetCFireMode(const char *name) const;
	virtual int GetFireModeIdx(const char *name) const;
	virtual int GetFireModeIdxWithAmmo(const IEntityClass* pAmmoClass) const;
	virtual int GetCurrentFireMode() const;
	virtual int GetPreviousFireMode() const;
	virtual void SetCurrentFireMode(int idx);
	virtual void SetCurrentFireMode(const char *name);
	virtual void ChangeFireMode();
	virtual int GetNextFireMode(int currMode) const;
	bool IsFiremodeDisabledByAccessory(int idx) const;
	virtual void FixAccessories(const SAccessoryParams *newParams, bool attach);

	virtual IZoomMode *GetZoomMode(int idx) const;
	virtual IZoomMode *GetZoomMode(const char *name) const;
	virtual int GetZoomModeIdx(const char *name) const;
	virtual const char *GetZoomModeName(int idx) const;
	virtual int GetCurrentZoomMode() const;
	virtual void SetCurrentZoomMode(int idx);
	virtual void SetCurrentZoomMode(const char *name);
	virtual void ChangeZoomMode();
	virtual void EnableZoomMode(int idx, bool enable);
	virtual void RestartZoom(bool force = false);

	virtual void FadeCrosshair(float to, float time, float delay = 0.0f);
	void UpdateCrosshair(float frameTime);
	void SetCrosshairMode(EWeaponCrosshair mode);
	CScopeReticule& GetScopeReticule() {return m_scopeReticule;}
	const CScopeReticule& GetScopeReticule() const {return m_scopeReticule;}

	const CWeaponStats* GetWeaponStats() const { return m_pWeaponStats; }
	CWeaponStats* GetWeaponStats() { return m_pWeaponStats; }
  
	virtual void AccessoriesChanged(bool initialLoadoutSetup);
	void GetCurrentAccessories(IEntityClass** pAccessoriesOut);
	const SFireModeParams* GetAccessoryAlteredFireModeParams(CFireMode* pFireMode, IEntityClass** pAccessories);
	const SZoomModeParams* GetAccessoryAlteredZoomModeParams(CIronSight* pZoomMode, IEntityClass** pAccessories);
	const SMeleeModeParams* GetAccessoryAlteredMeleeParams(IEntityClass** pAccessories);

	virtual bool CancelCharge() { return false; }; //Returns true if the weapon is chargeable and currently being charged (after canceling the charge)

	virtual void SetHostId(EntityId hostId);
	virtual EntityId GetHostId() const;

	virtual bool AllowInteraction(EntityId interactionEntity, EInteractionType interactionType);

	virtual bool PredictProjectileHit(IPhysicalEntity *pShooter, const Vec3 &pos, const Vec3 &dir,
		const Vec3 &velocity, float speed, Vec3& predictedPosOut, float& projectileSpeedOut,
		Vec3* pTrajectoryPositions = 0, unsigned int* trajectorySizeInOut = 0, float timeStep = 0.24f,
		Vec3* pTrajectoryVelocities = 0, const bool predictionForAI = false) const OVERRIDE;

	virtual const AIWeaponDescriptor& GetAIWeaponDescriptor( ) const;

	virtual bool Query(EWeaponQuery query, const void* param = NULL);

	virtual bool IsRippedOff() const { return false; }
	virtual bool IsRippingOff() const {return false;}
	bool CanLedgeGrab() const;

	virtual void UpdateCurrentActionController();
	// ~IWeapon

	bool	 IsZoomStable() const;
	bool	 IsZoomingIn() const;
	bool	 IsZoomingOut() const;
	bool	 IsZoomOutScheduled() const;
	void	 CancelZoomOutSchedule();
	bool	 ShouldSnapToTarget() const;
	float  GetZoomInTime() const;

	int  GetMaxZoomSteps();
	bool IsValidAssistTarget(IEntity *pEntity, IEntity *pSelf, bool includeVehicles=false);
	virtual void AssistAiming(float magnification=1.0f, bool accurate=false) {}
	virtual void AdvancedAssistAiming(float range, const Vec3& pos, Vec3 &dir) {}

	void		StartChangeFireMode();
	void		EndChangeFireMode();
	bool    IsSwitchingFireMode() { return m_switchingFireMode; };

	//Targeting stuff
	bool		IsTargetOn() { return m_targetOn; }
	void		ActivateTarget(bool activate) { m_targetOn = activate; }
	void		SetAimLocation(Vec3 &location) { m_aimPosition = location; }
	void		SetTargetLocation(Vec3 &location) { m_targetPosition = location; }
	Vec3&   GetAimLocation(){ return m_aimPosition; }
	Vec3&		GetTargetLocation() { return m_targetPosition; }

	bool		GetFireAlternation() { return m_fire_alternation;}
	void		SetFireAlternation(bool fireAlt) { m_fire_alternation = fireAlt;}

	//LAW special stuff
	virtual	void		AutoDrop() {};
	virtual void    AddFiredRocket() {};

	virtual EntityId	GetHeldEntityId() const { return 0; }

	virtual void SendMusicLogicEvent(EMusicLogicEvents event);

	//Zoom in/out events
	virtual void OnZoomIn();
	virtual void OnZoomOut();
	void OnZoomedIn();
	void OnZoomedOut();
	bool    GetScopePosition(Vec3& pos);

	bool HasScopeAttachment() const;

	ILINE void SetPlaySelectAction( bool bSelect ) { m_shouldPlayWeaponSelectAction = bSelect; }
	virtual bool ShouldPlaySelectAction() const;
	virtual void GetAngleLimits(EStance stance, float& minAngle, float& maxAngle);
	virtual bool	UpdateAimAnims(SParams_WeaponFPAiming &aimAnimParams);
	virtual float GetMovementModifier() const;
	virtual float GetRotationModifier(bool usingMouse) const;
	
	const CWeaponSharedParams* GetWeaponSharedParams() const { return m_weaponsharedparams; }

	virtual bool ShouldSendOnShootHUDEvent() const;

	virtual bool IsVehicleWeapon() const { return false; }
	void ShowDebugInfo();

	void BoostMelee(bool enableBoost);
	void SetAmmoCountFragmentTags(CTagState& fragTags, int ammoCount);
	virtual void SetFragmentTags(CTagState& fragTags);

	void HighlightWeapon(bool highlight, bool fromDrop = false);
	ILINE EntityId GetOriginalOwnerId() const { return m_previousOwnerId; }

	// network
	enum ENetReloadState
	{
		eNRS_NoReload,
		eNRS_StartReload,
		eNRS_EndReload,
		eNRS_CancelReload
	};

	struct SNetWeaponData
	{
		int		m_weapon_ammo;
		uint8	m_firemode;
		bool	m_isFiring;
		bool  m_zoomState;
		uint8 m_fireCounter;

		void NetSerialise(TSerialize ser);
	};

	struct SNetWeaponReloadData
	{
		uint8 m_reload;
		uint8 m_expended_ammo;
		int		m_inventory_ammo;

		void NetSerialise(TSerialize ser);
	};

	struct SNetWeaponMeleeData
	{
		uint8 m_meleeCounter;
		int8  m_attackIndex; 

		void NetSerialise(TSerialize ser);
	};

	struct SNetWeaponChargeData
	{
		SNetWeaponChargeData() : m_charging(false) {}

		bool m_charging;

		void NetSerialise(TSerialize ser);
	};

	
	struct WeaponRaiseParams
	{
		WeaponRaiseParams() : raise(false) {}
		WeaponRaiseParams(bool _raise) : raise(_raise) {}

		bool raise;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("raise", raise);
		}
	};

	struct ZoomStateParams
	{
		ZoomStateParams() : zoomed(false) {}
		ZoomStateParams(bool _zoomed) : zoomed(_zoomed) {}

		bool zoomed;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("zoomed", zoomed);
		}
	};

	struct SvRequestShootParams
	{
		SvRequestShootParams() {};
#if defined(ANTI_CHEAT)	
		SvRequestShootParams(const Vec3 &at, uint32 _sid, int _fireModeId) : hit(at), shotId(_sid), fireModeId(_fireModeId) {};
		uint32 shotId;
#else
		SvRequestShootParams(const Vec3 &at, int _fireModeId) : hit(at), fireModeId(_fireModeId) {};
#endif
		Vec3 hit;
		int fireModeId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("hit", hit, 'sHit');
			ser.Value("fireModeId", fireModeId, 'fmod');
#if defined(ANTI_CHEAT)		
			ser.Value("shotId", shotId, 'ui32');
#endif
		};
	};

	struct SvRequestShootExParams
	{
		SvRequestShootExParams() {};
		SvRequestShootExParams(const Vec3 &_pos, const Vec3 &_dir, const Vec3 &_vel, const Vec3 &_hit, float _extra, int ph, int _fireModeId)
		: pos(_pos), dir(_dir), vel(_vel), hit(_hit), extra(_extra), predictionHandle(ph), fireModeId(_fireModeId) {};

		Vec3 pos;
		Vec3 dir;
		Vec3 vel;
		Vec3 hit;
		float extra;
		int predictionHandle;
		int fireModeId;

		void SerializeWith(TSerialize ser)
		{
			ser.Value("pos", pos, 'wrld');
			ser.Value("dir", dir, 'dir3');
			ser.Value("vel", vel, 'vel0');
			ser.Value("hit", hit, 'wrld');
			ser.Value("extra", extra, 'smal');
			ser.Value("predictionHandle", predictionHandle, 'phdl');
			ser.Value("fireModeId", fireModeId, 'fmod');
		};
	};

	struct SvRequestFireModeParams
	{
		SvRequestFireModeParams(): id(0) {};
		SvRequestFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct ClSetFireModeParams
	{
		ClSetFireModeParams(): id(0) {};
		ClSetFireModeParams(int fmId): id(fmId) {};

		int id;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", id, 'fmod');
		};
	};

	struct MeleeRMIParams
	{
		MeleeRMIParams() : boostedAttack(false), attackIndex(-1) {}
		MeleeRMIParams(bool _boostedAttack) : boostedAttack(_boostedAttack), attackIndex(-1) {}
		MeleeRMIParams(bool _boostedAttack, int8 _attackIndex) : boostedAttack(_boostedAttack), attackIndex(_attackIndex) {}

		bool boostedAttack;
		int8 attackIndex;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("boostedAttack", boostedAttack);
			ser.Value("attackIndex", attackIndex);
		}
	};

	struct DefaultParams
	{
		void SerializeWith(const TSerialize& ser) {};
	};

	struct SvRequestInstantReloadParams
	{
		SvRequestInstantReloadParams(): fireModeId(0) {};
		SvRequestInstantReloadParams(int fmId) : fireModeId(fmId) {};

		int fireModeId;
		void SerializeWith(TSerialize ser)
		{
			ser.Value("id", fireModeId, 'fmod');
		};
	};

	static const EEntityAspects ASPECT_FIREMODE	= eEA_GameServerA;
	static const EEntityAspects ASPECT_STREAM		= eEA_GameServerB;
	static const EEntityAspects ASPECT_MELEE		= eEA_GameServerC;
	static const EEntityAspects ASPECT_RELOAD		= eEA_GameServerD;
	static const EEntityAspects ASPECT_CHARGING	= eEA_GameServerStatic;

	DECLARE_SERVER_RMI_NOATTACH(SvRequestShoot, SvRequestShootParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestShootEx, SvRequestShootExParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestStartFire, DefaultParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestStopFire, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestStartMeleeAttack, MeleeRMIParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestFireMode, SvRequestFireModeParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestReload, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestCancelReload, DefaultParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestWeaponRaised, WeaponRaiseParams, eNRT_ReliableUnordered);
	DECLARE_SERVER_RMI_NOATTACH(SvRequestSetZoomState, ZoomStateParams, eNRT_ReliableOrdered);

	DECLARE_SERVER_RMI_NOATTACH(SvStartedCharging, DefaultParams, eNRT_ReliableUnordered);

	DECLARE_SERVER_RMI_NOATTACH(SvRequestInstantReload, SvRequestInstantReloadParams, eNRT_ReliableUnordered);

	virtual int		NetGetCurrentAmmoCount() const;
	virtual void	NetSetCurrentAmmoCount(int count);

	virtual int		GetReloadState() const;
	virtual void	SvSetReloadState(int state);
	virtual void	ClSetReloadState(int state);
	virtual void	SvCancelReload();

	void	NetStateSent();
	void	NetUpdateFireMode(SEntityUpdateContext& ctx);
	virtual bool	NetAllowUpdate(bool requireActor);

	virtual void NetShoot(const Vec3 &hit, int predictionHandle, int fireModeId);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, int fireModeId);
	
	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual void NetStartMeleeAttack(bool boostedAttack, int8 attackIndex = -1);

	virtual void NetZoom(float fov);
	virtual void NetSetIsFiring(bool isFiring);

	void SendEndReload();
	virtual void RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, bool forceExtended);
	void RequestStartFire();
	void RequestStopFire();
	void RequestReload();
	void RequestFireMode(int fmId);
	void RequestWeaponRaised(bool raise);
	void RequestSetZoomState(bool zoomed);

	virtual void RequestStartMeleeAttack(bool weaponMelee, bool boostedAttack, int8 attackIndex = -1);
	virtual void RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir) {};
	
	void RequestCancelReload();
	void RequestLock(EntityId id, int partId = 0);
	void RequestUnlock();
	virtual void RequestDetonate() {};
	
	// PROTOTYPE (need for PhGun)
	virtual void RequestEntityDelegation(EntityId id, bool authorize) {};
	//------------------------------------
	
	bool IsServerSpawn(IEntityClass* pAmmoType) const;
	CProjectile *SpawnAmmo(IEntityClass* pAmmoType, bool remote=false);

	bool IsProxyWeapon() const { return m_isProxyWeapon; }
	void SetIsProxyWeapon(bool isProxy) { m_isProxyWeapon = isProxy; }

	virtual bool AllowZoomToggle() { return true; }

	bool	AIUseEyeOffset() const;
	bool	AIUseOverrideOffset(EStance stance, float lean, float peekOver, Vec3& offset) const;

	virtual bool ApplyActorRecoil() const { return true; }
	virtual void ApplyFPViewRecoil(int nFrameId, Ang3 recoilAngles);

	ILINE void	AddPendingReload() { SetInputFlag(eWeaponAction_Reload); }
	virtual void ForcePendingActions(uint8 blockedActions = 0);

	ILINE const TAmmoVector& GetAmmoVector() const { return m_ammo; }

	const char	*GetName();

	bool IsOwnerSliding() const;
	bool IsOwnerClient() const;
	virtual void SetOwnerId(EntityId ownerId);
	void SetOwnerClientOverride(bool isClient);

	virtual bool	  IsLaserActivated() const;
	ILINE bool IsInputFlagSet(uint8 actionFlag) const { return ((s_requestedActions & actionFlag) != 0); }
	void TriggerMeleeReaction();

	virtual float GetZoomTimeMultiplier();

	float GetMuzzleFlashScale() const;

#if defined(ANTI_CHEAT)
	ILINE uint32				GetLastShotId()		const		{ return m_lastShotId; }
	ILINE static uint32	GetMaxShotCount()					{ return 0xFFF; }
	ILINE static uint32	GetShotIdCountOffset()		{ return 20; }
	ILINE static uint32	GetShotIdFireModeOffset() { return 16; }
	ILINE static uint32 GetShotIdFireModeMask()		{ return 0x000f0000; }
#endif

	virtual void EndBurst();

	void RefillBelt() { m_refillBelt = true; }

	void ClearDelayedFireTimeout()
	{
		m_delayedFireActionTimeOut = 0.f;
	}

	bool CanZoomInState(float fallingMinAirTime = 0.f) const;
	ILINE bool ReloadWhenSelected() const { return m_bReloadWhenSelected; }

	void AllowDrop();
	void DisallowDrop();

#ifdef SHOT_DEBUG
	class CShotDebug* GetShotDebug() const { return m_pShotDebug; }
#endif //SHOT_DEBUG

	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event);

	void OnHostMigrationCompleted();
	
	const DynArray<string>& GetCompatibleAccessories() const { return m_compatibleAccessories; }
protected:

	virtual bool OnActionSpecial(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	void ClearModes();
	virtual float GetSelectSpeed(CActor* pOwnerActor);
	
	bool HasCompatibleAmmo(IInventory* pInventory) const;
	bool CheckAmmoRestrictionsForAccessories(IInventory * pInventory) const;
	bool CheckAmmoRestrictionsForBonusAndMagazineAmmo(IInventory& inventory) const;

	virtual void InitItemFromParams();
	virtual void InitFireModes();
	void InitZoomModes();
	void InitAmmo();
	void InitAIData();
	void InitWeaponStats();
	void InitCompatibleAccessories();

#if defined(ANTI_CHEAT)
	ILINE void		IncShotCount(uint8 inc)
	{
		if(m_roundsFired + inc > CWeapon::GetMaxShotCount())
		{
			m_roundsFired = inc;
		}
		else
		{
			m_roundsFired += inc;
		}
	}

	ILINE uint16		GetShotCount() { return m_roundsFired; }
#endif

	//Activate/Deactivate Laser for the AI (also player if necessary)
	bool		IsLaserAttached() const;
	void    ActivateLaser(bool activate);

	bool		IsSilent() const;

	ILINE bool IsAnimationControlled() const { return m_animationFiringLocator.IsSet(); }

	void RegisterUsedAmmoWithInventory(IInventory* pInventory);
	void UnregisterUsedAmmoWithInventory(IInventory* pInventory);
	
	void OnDroppedByAI(IInventory* pAIInventory);
	void OnDroppedByPlayer(IInventory* pPlayerInventory);

	bool SetInventoryAmmoCountInternal(IInventory* pInventory, IEntityClass* pAmmoType, int count);

	EntityId	GetLaserAttachment() const;

	void SetNextShotTime(bool activate);
	void UpdateBulletBelt();

	void TestClipAmmoCountIsValid();

	const SPlayerMovementModifiers& GetPlayerMovementModifiers() const;

	ILINE void ClearInputFlags() { s_requestedActions = eWeaponAction_None;}
	void SetInputFlag(uint8 actionFlag);
	ILINE void ClearInputFlag(uint8 actionFlag) { s_requestedActions &= ~actionFlag; }

	virtual bool IsCurrentFireModeFromAccessory() const;

	EntityEffects::CHeatController		m_heatController;

	CScopeReticule				m_scopeReticule;

	CFireMode					*m_fm;

	CMelee*					m_melee;

	IZoomMode					*m_zm;
	int							m_zmId;
	int							m_primaryZmId;
	int							m_secondaryZmId;

	TFireModeIdMap		m_fmIds;
	TFireModeVector		m_firemodes;

	TZoomModeIdMap		m_zmIds;
	TZoomModeNameMap  m_zmNames;
	TZoomModeVector		m_zoommodes;
	
	DynArray<string>	m_compatibleAccessories;

	TAmmoVector				m_ammo;
	TAmmoVector				m_bonusammo;

#if defined(ANTI_CHEAT)
	uint32					m_lastShotId;
	uint16					m_roundsFired;
#endif

	bool							m_fire_alternation;

	bool							m_restartZoom; //this is a serialization helper
	int								m_restartZoomStep;

	CWeaponStats*			m_pWeaponStats;

	TWeaponEventListeners	m_listeners;

	IWeaponFiringLocator	*m_pFiringLocator;

	_smart_ptr<CWeaponSharedParams> m_weaponsharedparams;

	static float	s_dofValue;
	static float	s_dofSpeed;
	static float	s_focusValue;
	static TAmmoVector s_tmpCollectedAmmo;

  Vec3	m_destination;
	Vec3	m_aimPosition;
	Vec3	m_targetPosition;

	bool	m_forcingRaise;
	bool	m_targetOn;

	bool	m_switchingFireMode;
	bool	m_doingMagazineSwap;

	float m_reloadButtonTimeStamp;

	float m_nextShotTime;

	float m_zoomTimeMultiplier;
	float m_selectSpeedMultiplier;

	int		m_lastRecoilUpdate;

	float	m_delayedFireActionTimeOut;
	float	m_delayedZoomActionTimeOut;
	float	m_delayedMeleeActionTimeOut;
	float	m_switchFireModeTimeStap;
	bool	m_delayedZoomStayZoomedVal;

	bool	m_isClientOwnerOverride;
	bool	m_minDropAmmoAvailable;
	bool  m_isRegisteredAmmoWithInventory;

	float	m_snapToTargetTimer;

	bool	m_shouldPlayWeaponSelectAction;
	bool	m_isProxyWeapon;
	bool	m_refillBelt;
	bool	m_addedAmmoCapacity;
	bool	m_extendedClipAdded;

	bool    m_DropAllowedFlag;
	bool		m_bIsHighlighted;
	bool		m_bReloadWhenSelected;

	// network
	int		m_reloadState;
	int		m_firemode;
	int		m_prevFiremode;
	int		m_shootCounter;	// num bullets to shoot
	int		m_lastRecvInventoryAmmo;
	float	m_netNextShot;
	float m_weaponNextShotTimer;
	bool	m_isFiring;
	bool	m_isFiringStarted;
	uint8	m_fireCounter;	// total that have been fired
	uint8 m_expended_ammo;
	uint8 m_meleeCounter;
	int8  m_attackIndex; 
	bool	m_doMelee;
	bool	m_netInitialised;
	bool	m_isDeselecting;

	static const uint8 kMeleeCounterMax = 4;

	float					m_currentCrosshairVisibility;
	EWeaponCrosshair		m_crosshairMode;

	class IAction *m_deselectAction;
	_smart_ptr<class IAction> m_enterModifyAction;
	
private:
	CFireMode* FindFireModeForAmmoType(IEntityClass* pAmmoType) const;
	bool CanRefillAmmoType(IEntityClass* pAmmoType, const char* refillType) const;
	bool RefillInventoryAmmo(IInventory* pInventory, IEntityClass* pAmmoTypeClass, CFireMode* pFireMode);

	void  RegisterActions();

	virtual bool OnActionAttackPrimary(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual bool OnActionAttackSecondary(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual bool OnActionFiremode(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionSprint(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionReload(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionModify(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomToggle(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomIn(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomOut(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual bool OnActionZoom(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionZoomXI(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionStabilize(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionToggleFlashLight(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	void ToggleFlashLight();

	bool CanStabilize() const;
	bool PreActionAttack(bool startFire);
	bool PreMeleeAttack();
	bool CheckSprint();

	void AutoSelectNextItem();

	virtual void AddAmmoCapacity();
	virtual void DropAmmoCapacity();
	void ProcessAllAccessoryAmmoCapacities(IInventory* pOwnerInventory, bool addCapacity);
	void PlayChangeFireModeTransition(CFireMode* pNewFiremode);

	CAnimationFiringLocator	m_animationFiringLocator;
	IAnimationOperatorQueuePtr m_BeltModifier;

	EntityId m_previousOwnerId;
	
#ifdef SHOT_DEBUG
	class CShotDebug* m_pShotDebug;
#endif //SHOT_DEBUG

	//Flags for force input states (make weapon more responsive)
	//Static: It's client only, and this way they can be remembered through weapon switches
	static uint8 s_requestedActions;
	static bool  s_lockActionRequests;

#ifdef SERVER_CHECKS
	float m_fSampleStartTime;
	float m_fLastSampleTakenTime;
	float	m_fSampleNumShots;
	
	void StartVerificationSample(float fCurrentTime);
	bool ShouldEndVerificationSample(float fCurrentTime, uint32 shotId);
	void EndVerificationSample(IActor * pActor, uint32 oldShotId);
#else	//Use #define to avoid 'a' being compiled in if it is a function call
#define StartVerificationSample(a)				(void)(0)
#define ShouldEndVerificationSample(a, b)	false
#define EndVerificationSample(a, b)				(void)(0)
#endif
};

class CSimpleFiringLocator : public IWeaponFiringLocator
{
public:
	virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit)
	{
		return false;
	}
	virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos)
	{
		pos=m_pos;
		return true;
	}
	virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
	{
		dir=m_dir;
		return true;
	}
	virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos)
	{
		dir=m_dir;
		return true;
	}
	virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir)
	{
		return false;
	}
	virtual void WeaponReleased()
	{
		delete this;
	}

	Vec3 m_dir;
	Vec3 m_pos;
};

#endif //__WEAPON_H__
