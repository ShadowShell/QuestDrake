/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2008.
-------------------------------------------------------------------------

Description: 
Simple tweak menu item for debugging and other items not requiring LUA

-------------------------------------------------------------------------
History:
- 1:11:2008  : Created by Adam Rutkowski

*************************************************************************/

#include "StdAfx.h"
#include "TweakItemSimple.h"

//-------------------------------------------------------------------------

CTweakItemSimple::CTweakItemSimple(CTweakMenuController* pController, const string& command)
: CTweakMetadata(pController, command)
{
	m_type = eTT_ItemSimple;
}