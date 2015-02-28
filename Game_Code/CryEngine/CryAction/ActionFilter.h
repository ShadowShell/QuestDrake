/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 8:9:2004   10:32 : Created by Márcio Martins

*************************************************************************/
#ifndef __ACTIONFILTER_H__
#define __ACTIONFILTER_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "IActionMapManager.h"


typedef std::set<ActionId> TFilterActions;


class CActionMapManager;

class CActionFilter :
	public IActionFilter
{
public:
	CActionFilter(CActionMapManager *pActionMapManager, IInput *pInput, const char* name, EActionFilterType type=eAFT_ActionFail);
	virtual ~CActionFilter();

	// IActionFilter
	virtual void Release() { delete this; };
	VIRTUAL void Filter(const ActionId& action);
	VIRTUAL bool SerializeXML(const XmlNodeRef& root, bool bLoading);
	VIRTUAL const char* GetName() { return m_name.c_str(); }
	VIRTUAL void Enable(bool enable);
	VIRTUAL bool Enabled() { return m_enabled; };
	// ~IActionFilter

	bool ActionFiltered(const ActionId& action);

	VIRTUAL void GetMemoryUsage(ICrySizer *pSizer ) const;	 

private:
	bool							m_enabled;
	CActionMapManager*m_pActionMapManager;
	IInput*				m_pInput;
	TFilterActions		m_filterActions;
	EActionFilterType m_type;
	string            m_name;
};


#endif //__ACTIONFILTER_H__