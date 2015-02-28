/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Abstract class unifying metadata classes

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadata.h"

//-------------------------------------------------------------------------

CTweakMetadata::CTweakMetadata(CTweakMenuController* pController, const string& command)
: CTweakCommon(pController)
, m_fDelta(1.0f)
, m_fMin(0.0f)
, m_fMax(std::numeric_limits<float>::max())
, m_boolean(false)
{
	m_command = command;
}

//-------------------------------------------------------------------------

void CTweakMetadata::InitItem(XmlNodeRef xml)
{
	if(strcmp(xml->getTag(), "Entry"))
		return;

	m_sName = xml->getAttr("name");
	m_sDescription = xml->getAttr("description");

	string delta = xml->getAttr("delta");
	if(delta=="bool")
	{
		m_boolean = true;
	}
	else 
	{
		xml->getAttr("delta", m_fDelta);
	}		

	xml->getAttr("min", m_fMin);
	xml->getAttr("max", m_fMax);

	int settingCount = xml->getChildCount();
	for(int i=0; i<settingCount; ++i)
	{
		XmlNodeRef child = xml->getChild(i);
		if(child && !stricmp(child->getTag(), "Setting"))
		{
			int value = 0;
			const char* display = child->getAttr("display");
			child->getAttr("value", value);			

			m_intSettings[value] = display;
		}
	}
}

//-------------------------------------------------------------------------

double CTweakMetadata::ClampToLimits( double x ) const
{
	// This should stay as a conditional clamp because m_fMin and m_fMax are
	// often set to -FLT_MAX and FLT_MAX, and this seems to be problematic with
	// a mathematical clamp
	return CLAMP(x, (double)m_fMin, (double)m_fMax);
}

//////////////////////////////////////////////////////////////////////////

string CTweakMetadata::GetDisplayString(int value) const
{
	return stl::find_in_map(m_intSettings, value, "");
}

void CTweakMetadata::AddDisplayString(int value, const string& name)
{
	m_intSettings[value] = name;
}