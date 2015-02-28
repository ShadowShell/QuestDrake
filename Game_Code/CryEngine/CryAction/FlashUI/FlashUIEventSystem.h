////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2010.
// -------------------------------------------------------------------------
//  File name:   FlashUIEventSystem.h
//  Version:     v1.00
//  Created:     10/9/2010 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FlashUIEventSystem_H__
#define __FlashUIEventSystem_H__

#include <IFlashUI.h>
#include <CryListenerSet.h>

class CFlashUIEventSystem 
	: public IUIEventSystem
{
public:
	CFlashUIEventSystem( const char* sName, EEventSystemType eType ) : m_sName(sName), m_eType(eType), m_listener(2) {};
	VIRTUAL ~CFlashUIEventSystem();

	VIRTUAL const char* GetName() const { return m_sName.c_str(); }
	VIRTUAL IUIEventSystem::EEventSystemType GetType() const { return m_eType; }

	VIRTUAL uint RegisterEvent( const SUIEventDesc& sEventDesc );

	VIRTUAL void RegisterListener( IUIEventListener* pListener, const char* name ) ;
	VIRTUAL void UnregisterListener( IUIEventListener* pListener );

	VIRTUAL SUIArgumentsRet SendEvent( const SUIEvent& event );

	VIRTUAL const SUIEventDesc* GetEventDesc( int index ) const { return index < m_eventDescriptions.size() ? m_eventDescriptions[index] : NULL; }
	VIRTUAL const SUIEventDesc* GetEventDesc( const char* sEventName ) const { return m_eventDescriptions(sEventName); }
	VIRTUAL int GetEventCount()  const { return m_eventDescriptions.size(); }

	VIRTUAL uint GetEventId( const char* sEventName );

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

private:
	string m_sName;
	EEventSystemType m_eType;
	TUIEventsLookup m_eventDescriptions;

	typedef CListenerSet< IUIEventListener* > TEventListener;
	TEventListener m_listener;
};

typedef std::map<string, CFlashUIEventSystem*> TUIEventSystemMap;

struct CUIEventSystemIterator : public IUIEventSystemIterator
{
	CUIEventSystemIterator(TUIEventSystemMap* pMap);
	VIRTUAL void AddRef();
	VIRTUAL void Release();
	VIRTUAL IUIEventSystem* Next( string &sName );

private:
	int m_iRefs;
	TUIEventSystemMap::iterator m_currIter;
	TUIEventSystemMap::iterator m_endIter;
};


#endif // #ifndef __FlashUIEventSystem_H__
