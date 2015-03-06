/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: C++ Item Implementation

-------------------------------------------------------------------------
History:
- 11:9:2004   15:00 : Created by Márcio Martins

*************************************************************************/
#ifndef __AUTOMATIC_H__
#define __AUTOMATIC_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "Single.h"

class CAutomatic : public CSingle
{
public:
	CRY_DECLARE_GTI(CAutomatic);
	
	CAutomatic();
	virtual ~CAutomatic();

	// CSingle
	virtual void StartReload(int zoomed);

	virtual void GetMemoryUsage(ICrySizer * s) const;
	void GetInternalMemoryUsage(ICrySizer * s) const;
	virtual void Update(float frameTime, uint32 frameId);
	virtual void StartFire();
	virtual void StopFire();
	// ~CSingle

private:
	
	typedef CSingle BaseClass;

};


#endif //__AUTOMATIC_H__