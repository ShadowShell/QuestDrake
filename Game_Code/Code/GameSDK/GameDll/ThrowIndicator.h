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

#ifndef __THROWINDICATOR_H__
#define __THROWINDICATOR_H__

#include "IronSight.h"

class CThrowIndicator : public CIronSight
{

public:
	CRY_DECLARE_GTI(CThrowIndicator);

	CThrowIndicator();
	virtual ~CThrowIndicator();

	virtual bool StartZoom(bool stayZoomed = false, bool fullZoomout = true, int zoomStep = 1);
	virtual void StopZoom();
	virtual void ExitZoom(bool force);

	virtual bool IsZoomed() const { return m_indicatorShowing; }

private:

	bool m_indicatorShowing;
};

#endif // __THROWINDICATOR_H__
