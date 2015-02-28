////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////
#ifndef __MANNEQUININTERFACE_H__
#define __MANNEQUININTERFACE_H__

#include "ICryMannequin.h"

class CMannequinInterface : public IMannequin
{
public:
	CMannequinInterface();
	~CMannequinInterface();

	// IMannequin
	virtual void UnloadAll();
	virtual void ReloadAll();

	virtual IAnimationDatabaseManager &GetAnimationDatabaseManager();
	virtual IActionController *CreateActionController(IEntity* pEntity, SAnimationContext &context);
	virtual IActionController *FindActionController(const IEntity& entity);
	virtual IMannequinEditorManager *GetMannequinEditorManager();
	virtual CMannequinUserParamsManager &GetMannequinUserParamsManager();

	virtual void AddMannequinGameListener(IMannequinGameListener *pListener);
	virtual void RemoveMannequinGameListener(IMannequinGameListener *pListener);
	virtual uint32 GetNumMannequinGameListeners();
	virtual IMannequinGameListener *GetMannequinGameListener(uint32 idx);
	virtual void SetSilentPlaybackMode (bool bSilentPlaybackMode);
	virtual bool IsSilentPlaybackMode() const;
	// ~IMannequin

private:
	void RegisterCVars();

private:
	class CAnimationDatabaseManager *m_pAnimationDatabaseManager;
	std::vector<IMannequinGameListener *> m_mannequinGameListeners;
	CMannequinUserParamsManager m_userParamsManager;
	bool m_bSilentPlaybackMode;
};

#endif //!__MANNEQUININTERFACE_H__