/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Action Map implementation. Maps Actions to Keys.
  
 -------------------------------------------------------------------------
  History:
  - 7:9:2004   17:47 : Created by Márcio Martins
	- 15:9:2010  12:30 : Revised by Dean Claassen

*************************************************************************/
#ifndef __ACTIONMAP_H__
#define __ACTIONMAP_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "IActionMapManager.h"

class CActionMapManager;
class CActionMap;

typedef std::vector<SActionInput*> TActionInputs;

class CActionMapAction : public IActionMapAction
{
public:
	CActionMapAction();
	virtual ~CActionMapAction();

	// IActionMapAction
	VIRTUAL void GetMemoryUsage(ICrySizer *pSizer ) const;
	virtual void Release() { delete this; };
	VIRTUAL int GetNumActionInputs() const { return m_actionInputs.size(); };
	VIRTUAL const SActionInput* FindActionInput(const char* szInput) const;
	VIRTUAL const SActionInput* GetActionInput(const int iIndex) const;
	VIRTUAL const SActionInput* GetActionInput(const EActionInputDevice device, const int iIndexByDevice) const;
	VIRTUAL const ActionId& GetActionId() const { return m_actionId; }
	VIRTUAL const char* GetTriggeredActionInput() const { return m_triggeredInput; };
	// IActionMapAction

	bool AddActionInput(const SActionInput& actionInput, const int iByDeviceIndex = -1);
	bool RemoveActionInput(uint32 inputCRC);
	void RemoveAllActionInputs();
	SActionInput* AddAndGetActionInput(const SActionInput& actionInput);
	void SetParentActionMap(CActionMap* pParentActionMap) { m_pParentActionMap = pParentActionMap; }
	SActionInput* FindActionInput(uint32 inputCRC);
	SActionInput* GetActionInput(const int iIndex);
	SActionInput* GetActionInput(const EActionInputDevice device, const int iIndexByDevice);
	void SetActionId(const ActionId& actionId) { m_actionId = actionId; }
	void SetNumRebindedInputs(const int iNumRebindedInputs) { m_iNumRebindedInputs = iNumRebindedInputs; }
	int GetNumRebindedInputs() const { return m_iNumRebindedInputs; }

private:
	TActionInputString	m_triggeredInput;
	ActionId						m_actionId;
	TActionInputs				m_actionInputs;
	CActionMap*					m_pParentActionMap;
	int									m_iNumRebindedInputs;
};

class CActionMap :
	public IActionMap
{
public:
	CActionMap(CActionMapManager *pActionMapManager, const char* name);
	virtual ~CActionMap();

	// IActionMap
	VIRTUAL void GetMemoryUsage(ICrySizer *pSizer ) const;
	virtual void Release();
	VIRTUAL void Clear();
	VIRTUAL const IActionMapAction* GetAction(const ActionId& actionId) const;
	VIRTUAL IActionMapAction* GetAction(const ActionId& actionId);
	VIRTUAL bool CreateAction(const ActionId& actionId);
	VIRTUAL bool RemoveAction(const ActionId& actionId);
	VIRTUAL int  GetActionsCount() const { return m_actions.size(); };
	VIRTUAL bool AddActionInput(const ActionId& actionId, const SActionInput& actionInput, const int iByDeviceIndex = -1);
	VIRTUAL bool RemoveActionInput(const ActionId& actionId, const char* szInput);
	VIRTUAL bool ReBindActionInput(const ActionId& actionId, const char* szCurrentInput, const char* szNewInput);
	VIRTUAL bool ReBindActionInput(const ActionId& actionId, 
																 const char* szNewInput, 
																 const EActionInputDevice device,
																 const int iByDeviceIndex);
	VIRTUAL int GetNumRebindedInputs() { return m_iNumRebindedInputs; }
	VIRTUAL bool Reset();
	VIRTUAL bool LoadFromXML(const XmlNodeRef& actionMapNode);
	VIRTUAL bool LoadRebindingDataFromXML(const XmlNodeRef& actionMapNode);
	VIRTUAL bool SaveRebindingDataToXML(XmlNodeRef& actionMapNode) const;
	VIRTUAL IActionMapActionIteratorPtr CreateActionIterator();
	VIRTUAL void SetActionListener(EntityId id);
	VIRTUAL EntityId GetActionListener() const;
	VIRTUAL const char* GetName() { return m_name.c_str(); }
	VIRTUAL void Enable(bool enable);
	VIRTUAL bool Enabled() const { return m_enabled; };
	// ~IActionMap

	void EnumerateActions( IActionMapPopulateCallBack* pCallBack ) const; 
	bool CanProcessInput(const SInputEvent& inputEvent, CActionMap* pActionMap, CActionMapAction* pAction, SActionInput* pActionInput);
	bool IsActionInputTriggered(const SInputEvent& inputEvent, CActionMap* pActionMap, CActionMapAction* pAction, SActionInput* pActionInput) const;
	void InputProcessed();
	void ReleaseActionsIfActive();
	void ReleaseActionIfActive(const ActionId& actionId);
	void ReleaseFilteredActions();

private:
	CActionMapAction* CreateAndGetAction(const ActionId& actionId);
	bool AddAndBindActionInput(CActionMapAction* pAction, const SActionInput& actionInput);
	bool ReBindActionInput(CActionMapAction* pAction, const char* szCurrentInput, const char* szNewInput);
	bool ReBindActionInput(CActionMapAction* pAction, SActionInput& actionInput, const char* szNewInput);
	bool ReBindActionInput(CActionMapAction* pAction, 
												 const char* szNewInput, 
												 const EActionInputDevice device,
												 const int iByDeviceIndex);
	void ReleaseActionIfActiveInternal(CActionMapAction &action);
	EActionAnalogCompareOperation GetAnalogCompareOpTypeFromStr(const char* szTypeStr);
	const char* GetAnalogCompareOpStr(EActionAnalogCompareOperation compareOpType) const;
	void SetNumRebindedInputs(const int iNumRebindedInputs) { m_iNumRebindedInputs = iNumRebindedInputs; }
	bool LoadActionInputAttributesFromXML(const XmlNodeRef& actionInputNode, SActionInput& actionInput);
	bool SaveActionInputAttributesToXML(XmlNodeRef& actionInputNode, const SActionInput& actionInput) const;
	void LoadActivationModeBitAttributeFromXML(const XmlNodeRef& attributeNode, int& activationFlags, const char* szActivationModeName, EActionActivationMode activationMode);
	void SaveActivationModeBitAttributeToXML(XmlNodeRef& attributeNode, const int activationFlags, const char* szActivationModeName, EActionActivationMode activationMode) const;

	typedef std::map<ActionId, CActionMapAction> TActionMap;

	bool								m_enabled;
	CActionMapManager*	m_pActionMapManager;
	TActionMap					m_actions;
	EntityId						m_listenerId;
	string							m_name;
	int									m_iNumRebindedInputs;
};


#endif //__ACTIONMAP_H__
