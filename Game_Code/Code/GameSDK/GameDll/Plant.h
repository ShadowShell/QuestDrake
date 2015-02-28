/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Throw Fire Mode Implementation

-------------------------------------------------------------------------
History:
- 26ye:10:2005   15:45 : Created by Márcio Martins

*************************************************************************/
#ifndef __PLANT_H__
#define __PLANT_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "FireMode.h"
#include "ItemString.h"


class CPlant : public CFireMode
{
	struct StartPlantAction;
	struct MidPlantAction;
	struct EndPlantAction;

private:
	typedef CFireMode BaseClass;

public:
	CRY_DECLARE_GTI(CPlant);

	CPlant();
	virtual ~CPlant();

	virtual void PostInit();
	virtual void PostUpdate(float frameTime) {};
	virtual void UpdateFPView(float frameTime);
	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void Activate(bool activate);

#ifdef SERVER_CHECKS
	virtual float GetDamageAmountAtXMeters(float x);
#endif

	virtual int GetAmmoCount() const;
	virtual int GetClipSize() const;

	virtual bool OutOfAmmo() const;
	virtual bool LowAmmo(float thresholdPerCent) const;
	virtual bool CanReload() const { return false; };
	virtual void Reload(int zoomed) {};
	virtual bool IsReloading(bool includePending=true) { return false; };
	virtual void CancelReload() {};
	virtual bool CanCancelReload() { return false; };

	virtual bool AllowZoom() const { return true; };
	virtual void Cancel() {};

	virtual float GetRecoil() const { return 0.0f; };
	virtual float GetSpread() const { return 0.0f; };
	virtual float GetSpreadForHUD() const { return 0.0f; }
	virtual float GetMinSpread() const { return 0.0f; };
	virtual float GetMaxSpread() const { return 0.0f; };
	virtual const char *GetCrosshair() const { return ""; };

	virtual bool CanFire(bool considerAmmo=true) const;
	virtual void StartFire();
	virtual void StopFire();
	virtual bool IsFiring() const { return m_planting; };
	virtual bool IsSilenced() const;

	virtual void NetShoot(const Vec3 &hit, int ph);
	virtual void NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int ph);
	virtual void NetEndReload() {};

	virtual void NetStartFire();
	virtual void NetStopFire();

	virtual EntityId GetProjectileId() const;
	virtual void SetProjectileId(EntityId id);
	virtual EntityId RemoveProjectileId();
	int GetNumProjectiles() const;

	virtual IEntityClass* GetAmmoType() const;
	virtual int GetDamage() const;

	virtual float GetSpinUpTime() const { return 0.0f; };
	virtual float GetNextShotTime() const { return 0.0f; };
	virtual void SetNextShotTime(float time) {};
	virtual float GetFireRate() const { return 0.0f; };

	virtual Vec3 GetFiringPos(const Vec3 &probableHit) const { return ZERO;}
	virtual Vec3 GetFiringDir(const Vec3 &probableHit, const Vec3& firingPos) const { return ZERO;}

	virtual bool HasFireHelper() const { return false; }
	virtual Vec3 GetFireHelperPos() const { return Vec3(ZERO); }
	virtual Vec3 GetFireHelperDir() const { return FORWARD_DIRECTION; }

	virtual int GetCurrentBarrel() const { return 0; }
	virtual void Serialize(TSerialize ser);
	virtual void PostSerialize() {};

	virtual void OnZoomStateChanged();
	virtual void CheckAmmo();

protected:

	virtual void Plant(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, bool net=false, int ph=0);
	virtual bool GetPlantingParameters(Vec3& pos, Vec3& dir, Vec3& vel) const;

	void CacheAmmoGeometry();

	std::vector<EntityId> m_projectiles;

	bool		m_planting;
	bool		m_pressing;

};

#endif 
