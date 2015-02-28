/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Script Binding for Weapon
  
 -------------------------------------------------------------------------
  History:
  - 25:11:2004   11:30 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_WEAPON_H__
#define __SCRIPTBIND_WEAPON_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct IItemSystem;
struct IGameFramework;
class CItem;
class CWeapon;


class CScriptBind_Weapon :
	public CScriptableBase
{
public:
	CScriptBind_Weapon(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_Weapon();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	void AttachTo(CWeapon *pWeapon);

	int SetAmmoCount(IFunctionHandler *pH);
	int GetAmmoCount(IFunctionHandler *pH);
	int GetClipSize(IFunctionHandler *pH);

	int GetAmmoType(IFunctionHandler *pH);

	int SupportsAccessory(IFunctionHandler *pH, const char *accessoryName);
	int GetAccessory(IFunctionHandler *pH, const char *accessoryName);
	int AttachAccessory(IFunctionHandler *pH, const char *className, bool attach, bool force);
	int SwitchAccessory(IFunctionHandler *pH, const char *className);

	int SetCurrentFireMode(IFunctionHandler *pH, const char *name);

	int Reload(IFunctionHandler *pH);

private:
	void RegisterGlobals();
	void RegisterMethods();

	CItem *GetItem(IFunctionHandler *pH);
	CWeapon *GetWeapon(IFunctionHandler *pH);

	ISystem						*m_pSystem;
	IGameFramework		*m_pGameFW;
};


#endif //__SCRIPTBIND_ITEM_H__