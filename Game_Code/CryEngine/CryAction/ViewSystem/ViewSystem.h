/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: View System interfaces.
  
 -------------------------------------------------------------------------
  History:
  - 17:9:2004 : Created by Filippo De Luca
	24:11:2005: added movie system (Craig Tiller)
*************************************************************************/
#ifndef __VIEWSYSTEM_H__
#define __VIEWSYSTEM_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "View.h"
#include "IMovieSystem.h"
#include <ILevelSystem.h>

//typedef std::map<string, IView *(*)()>	TViewClassMap;


class CViewSystem : public IViewSystem, public IMovieUser, public ILevelSystemListener
{

private:

	typedef std::map<unsigned int,CView *>	TViewMap;
	typedef std::vector<unsigned int> TViewIdVector;
	
public:

	//IViewSystem
	VIRTUAL IView *CreateView();
	VIRTUAL void RemoveView(IView* pView);
	VIRTUAL void RemoveView(unsigned int viewId);

	VIRTUAL void SetActiveView(IView *pView);
	VIRTUAL void SetActiveView(unsigned int viewId);

	//utility functions
	VIRTUAL IView *GetView(unsigned int viewId);
	VIRTUAL IView *GetActiveView();

	VIRTUAL unsigned int GetViewId(IView *pView);
	VIRTUAL unsigned int GetActiveViewId();

	VIRTUAL void Serialize(TSerialize ser);
	VIRTUAL void PostSerialize();

	VIRTUAL IView *GetViewByEntityId(EntityId id, bool forceCreate);

	VIRTUAL float GetDefaultZNear() { return m_fDefaultCameraNearZ; };
	VIRTUAL void SetBlendParams(float fBlendPosSpeed, float fBlendRotSpeed, bool performBlendOut) { m_fBlendInPosSpeed = fBlendPosSpeed; m_fBlendInRotSpeed = fBlendRotSpeed; m_bPerformBlendOut = performBlendOut; };
	VIRTUAL void SetOverrideCameraRotation( bool bOverride,Quat rotation );
	VIRTUAL bool IsPlayingCutScene() const
	{
		return m_cutsceneCount > 0;
	}
	VIRTUAL void UpdateSoundListeners();

	VIRTUAL bool UseDeferredViewSystemUpdate() const { return m_useDeferredViewSystemUpdate; }
	//~IViewSystem

	//IMovieUser
	virtual void SetActiveCamera( const SCameraParams &Params );
	virtual void BeginCutScene(IAnimSequence* pSeq, unsigned long dwFlags,bool bResetFX);
	virtual void EndCutScene(IAnimSequence* pSeq, unsigned long dwFlags);
	virtual void SendGlobalEvent( const char *pszEvent );
	//virtual void PlaySubtitles( IAnimSequence* pSeq, ISound *pSound );
	//~IMovieUser

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {};
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel){};
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {};
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {};
	virtual void OnUnloadComplete(ILevel* pLevel);
	//~ILevelSystemListener

	CViewSystem(ISystem *pSystem);
	~CViewSystem();

	void Release() {delete this;};
	void Update(float frameTime);

	//void RegisterViewClass(const char *name, IView *(*func)());

	bool AddListener(IViewSystemListener* pListener)
	{
		return stl::push_back_unique(m_listeners, pListener);
	}

	bool RemoveListener(IViewSystemListener* pListener)
	{
		return stl::find_and_erase(m_listeners, pListener);
	}

	void GetMemoryUsage(ICrySizer * s) const;

	void ClearAllViews();

private:

	void RemoveViewById(unsigned int viewId);
	void ClearCutsceneViews();
	void DebugDraw();

	ISystem *m_pSystem;

	//TViewClassMap	m_viewClasses;
	TViewMap m_views;
	TViewIdVector m_cutsceneViewIdVector;

	// Listeners
	std::vector<IViewSystemListener*> m_listeners;

	unsigned int m_activeViewId;
	unsigned int m_nextViewIdToAssign;  // next id which will be assigned
	unsigned int m_preSequenceViewId; // viewId before a movie cam dropped in

	unsigned int m_cutsceneViewId;
	unsigned int m_cutsceneCount;

	bool m_bActiveViewFromSequence;
	
	bool	m_bOverridenCameraRotation;
	Quat	m_overridenCameraRotation;
	float m_fCameraNoise;
	float	m_fCameraNoiseFrequency;

	float m_fDefaultCameraNearZ;
	float m_fBlendInPosSpeed;
	float m_fBlendInRotSpeed;
	bool	m_bPerformBlendOut;
	int		m_nViewSystemDebug;

	bool m_useDeferredViewSystemUpdate;
};

#endif //__VIEWSYSTEM_H__
