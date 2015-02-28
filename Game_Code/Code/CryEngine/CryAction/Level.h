/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Level container. Implements method to perform specific
							 actions on a level.
  
 -------------------------------------------------------------------------
  History:
  - 18:8:2004   16:09 : Created by Márcio Martins

*************************************************************************/
#ifndef __LEVEL_H__
#define __LEVEL_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "ILevelSystem.h"
#include "LevelSystem.h"


class CLevel :
	public ILevel
{
	friend class CLevelSystem;
public:
	CLevel();
	virtual ~CLevel();
	
	virtual void Release();

	VIRTUAL ILevelInfo *GetLevelInfo();

private:
	CLevelInfo m_levelInfo;
};

#endif //__LEVEL_H__