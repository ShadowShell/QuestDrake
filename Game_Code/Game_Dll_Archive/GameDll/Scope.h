/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Iron Sight

-------------------------------------------------------------------------
History:
- 28:10:2005   16:00 : Created by Márcio Martins

*************************************************************************/
#pragma once

#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "IronSight.h"

class CScope : public CIronSight
{
	public:
		CRY_DECLARE_GTI(CScope);

	CScope();

	// IZoomMode
	virtual void Update(float frameTime, uint32 frameId);

	virtual void GetMemoryUsage(ICrySizer * s) const;

	virtual void UpdateFPView(float frameTime) {};
	// ~IZoomMode

	// CIronSight
	virtual void OnEnterZoom();
	virtual void OnLeaveZoom();
	virtual void OnZoomStep(bool zoomingIn, float t);
	virtual void OnZoomedOut();
	virtual void Activate(bool activate);
	virtual bool IsScope() const { return true; }
	// ~CIronSight

protected:
	float					m_showTimer;
	float					m_hideTimer;
	bool					m_musicLogicEventEnterTriggered;
	
private:
	void InformActorOfScope(bool active);
	void ToggleScopeVisionMode(bool enabled, bool toggleOffscreen);
};

#endif // __SCOPE_H__