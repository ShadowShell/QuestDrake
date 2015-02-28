////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   UIHUD3D.h
//  Version:     v1.00
//  Created:     22/11/2011 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __UIHUD3D_H__
#define __UIHUD3D_H__

#include "IUIGameEventSystem.h"
#include <IMovieSystem.h>
#include <IFlashUI.h>

class CUIHUD3D
	: public IUIGameEventSystem
	, public ISystemEventListener
	, public IEntityEventListener
	, public IMovieListener
	, public IUIModule
{
public:
	CUIHUD3D();
	~CUIHUD3D();
	// IUIGameEventSystem
	UIEVENTSYSTEM( "UIHUD3D" );
	virtual void InitEventSystem();
	virtual void UnloadEventSystem();
	virtual void UpdateView( const SViewParams &viewParams );

	// ISystemEventListener
	virtual void OnSystemEvent( ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam );
	// ~ISystemEventListener

	// IEntityEventListener
	virtual void OnEntityEvent( IEntity *pEntity,SEntityEvent &event );
	// ~IEntityEventListener

	// IMovieEventListener
	virtual void OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence);
	// ~IMovieEventListener

	// IUIModule
	void Reload();
	void Update(float fDeltaTime);
	// ~IUIModule

	void SetVisible(bool visible);
	bool IsVisible() const;

private:
	void SpawnHudEntities();
	void RemoveHudEntities();


	static void OnVisCVarChange( ICVar * );

private:
	IEntity* m_pHUDRootEntity;
	EntityId m_HUDRootEntityId;
	typedef std::vector< EntityId > THUDEntityList;
	THUDEntityList m_HUDEnties;
	bool m_bCutSceneNoHud;

	struct SHudOffset
	{
		SHudOffset(float aspect, float dist, float offset)
			: Aspect(aspect)
			, HudDist(dist)
			, HudZOffset(offset)
		{
		}

		float Aspect;
		float HudDist;
		float HudZOffset;
	};
	typedef std::vector< SHudOffset > THudOffset;
	THudOffset m_Offsets;

	float m_fHudDist;
	float m_fHudZOffset;
	//IAnimSequence* m_pSequence;
};

#endif // __UIHUD3D_H__

