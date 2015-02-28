// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.


#include "StdAfx.h"
#include "Level.h"


CLevel::CLevel()
{
}

CLevel::~CLevel()
{
}

void CLevel::Release()
{
	delete this;
}

ILevelInfo *CLevel::GetLevelInfo()
{
	return &m_levelInfo;
}

#include UNIQUE_VIRTUAL_WRAPPER(ILevel)