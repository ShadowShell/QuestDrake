////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2010.
// -------------------------------------------------------------------------
//  File name:   FlashUIElement.h
//  Version:     v1.00
//  Created:     10/9/2010 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FlashUIElement_H__
#define __FlashUIElement_H__

#include <IFlashUI.h>
#include <CryListenerSet.h>
#include <IPlatformOS.h>

struct SUIElementSerializer;
class CFlashUI;

class CFlashUIElement 
	: public IUIElement
	, public IFSCommandHandler
	, public IVirtualKeyboardEvents
{
public:
	CFlashUIElement(CFlashUI* pFlashUI, CFlashUIElement* pBaseInstance = NULL, uint instanceId = 0);
	VIRTUAL ~CFlashUIElement();

	VIRTUAL void AddRef();
	VIRTUAL void Release();

	VIRTUAL uint GetInstanceID() const { return m_iInstanceID; }
	VIRTUAL IUIElement* GetInstance( uint instanceID );
	VIRTUAL IUIElementIteratorPtr GetInstances() const;
	VIRTUAL bool DestroyInstance( uint instanceID );
	VIRTUAL bool DestroyThis() { return DestroyInstance(m_iInstanceID); }

	VIRTUAL void SetName( const char* sName ) { m_sName = sName; }
	VIRTUAL const char* GetName() const { return m_sName.c_str(); }

	VIRTUAL void SetGroupName( const char* sGroupName ) { m_sGroupName = sGroupName; }
	VIRTUAL const char* GetGroupName() const { return m_sGroupName.c_str(); }

	VIRTUAL void SetFlashFile( const char* sFlashFile );
	VIRTUAL const char* GetFlashFile() const { return m_sFlashFile.c_str(); }

	VIRTUAL bool Init( bool bLoadAsset = true );
	VIRTUAL void Unload( bool bAllInstances = false );
	VIRTUAL void Reload( bool bAllInstances = false );
	VIRTUAL bool IsInit() const { return m_pFlashplayer != NULL; }

	VIRTUAL bool IsValid() const { return m_pBaseInstance ? m_pBaseInstance->m_bIsValid : m_bIsValid; }

	VIRTUAL void UnloadBootStrapper();
	VIRTUAL void ReloadBootStrapper();

	VIRTUAL void Update( float fDeltaTime );
	VIRTUAL void Render();
	VIRTUAL void RenderLockless();

	VIRTUAL void RequestHide();
	VIRTUAL bool IsHiding() const { return m_bIsHideRequest; }

	VIRTUAL void SetVisible( bool bVisible );
	VIRTUAL inline bool IsVisible() const { return m_bVisible; }

	VIRTUAL void SetFlag( EFlashUIFlags flag, bool bSet );
	VIRTUAL bool HasFlag( EFlashUIFlags flag ) const;

	VIRTUAL float GetAlpha() const { return m_fAlpha; }
	VIRTUAL void SetAlpha( float fAlpha );

	VIRTUAL int GetLayer() const  { return m_iLayer; }
	VIRTUAL void SetLayer( int iLayer );

	VIRTUAL void SetConstraints( const SUIConstraints& newConstraints );
	VIRTUAL inline const IUIElement::SUIConstraints& GetConstraints() const { return m_constraints; }

	VIRTUAL void ForceLazyUpdate() { ForceLazyUpdateInl(); }
	VIRTUAL void LazyRendered() { m_bNeedLazyRender = false; }
	VIRTUAL bool NeedLazyRender() const { return (m_iFlags & (uint) eFUI_LAZY_UPDATE) == 0 || m_bNeedLazyRender; }

	VIRTUAL IFlashPlayer* GetFlashPlayer();

	VIRTUAL const SUIParameterDesc* GetVariableDesc( int index ) const { return index < m_variables.size() ? m_variables[index] : NULL; }
	VIRTUAL const SUIParameterDesc* GetVariableDesc( const char* sVarName ) const { return m_variables(sVarName); }
	VIRTUAL int GetVariableCount() const { return m_variables.size(); }

	VIRTUAL const SUIParameterDesc* GetArrayDesc( int index ) const { return index < m_arrays.size() ? m_arrays[index] : NULL; }
	VIRTUAL const SUIParameterDesc* GetArrayDesc( const char* sArrayName ) const { return  m_arrays(sArrayName); }
	VIRTUAL int GetArrayCount() const { return m_arrays.size(); }

	VIRTUAL const SUIMovieClipDesc* GetMovieClipDesc( int index ) const { return index < m_displayObjects.size() ? m_displayObjects[index] : NULL; }
	VIRTUAL const SUIMovieClipDesc* GetMovieClipDesc( const char* sMovieClipName ) const { return  m_displayObjects(sMovieClipName); }
	VIRTUAL int GetMovieClipCount() const { return m_displayObjects.size(); }

	VIRTUAL const SUIMovieClipDesc* GetMovieClipTmplDesc( int index ) const { return index < m_displayObjectsTmpl.size() ? m_displayObjectsTmpl[index] : NULL; }
	VIRTUAL const SUIMovieClipDesc* GetMovieClipTmplDesc( const char* movieClipTmplName ) const { return  m_displayObjectsTmpl(movieClipTmplName); }
	VIRTUAL int GetMovieClipTmplCount() const { return m_displayObjectsTmpl.size(); }

	VIRTUAL const SUIEventDesc* GetEventDesc( int index ) const { return index < m_events.size() ? m_events[index] : NULL; }
	VIRTUAL const SUIEventDesc* GetEventDesc( const char* sEventName ) const { return  m_events(sEventName); }
	VIRTUAL int GetEventCount() const { return m_events.size(); }

	VIRTUAL const SUIEventDesc* GetFunctionDesc( int index ) const { return index < m_functions.size() ? m_functions[index] : NULL; }
	VIRTUAL const SUIEventDesc* GetFunctionDesc( const char* sFunctionName ) const { return  m_functions(sFunctionName); }
	VIRTUAL int GetFunctionCount() const { return m_functions.size(); }

	VIRTUAL void UpdateViewPort();
	VIRTUAL void GetViewPort(int &x, int &y, int &width, int &height, float& aspectRatio);

	VIRTUAL bool Serialize( XmlNodeRef& xmlNode, bool bIsLoading );

	VIRTUAL void AddEventListener( IUIElementEventListener* pListener, const char* name );
	VIRTUAL void RemoveEventListener( IUIElementEventListener* pListener );

	VIRTUAL bool CallFunction( const char* fctName, const SUIArguments& args = SUIArguments(), TUIData* pDataRes = NULL, const char* pTmplName = NULL );
	VIRTUAL bool CallFunction( const SUIEventDesc* pFctDesc, const SUIArguments& args = SUIArguments(), TUIData* pDataRes = NULL, const SUIMovieClipDesc* pTmplDesc = NULL );

	VIRTUAL IFlashVariableObject* GetMovieClip( const char* movieClipName, const char* pTmplName = NULL );
	VIRTUAL IFlashVariableObject* GetMovieClip( const SUIMovieClipDesc* pMovieClipDesc, const SUIMovieClipDesc* pTmplDesc = NULL );
	VIRTUAL IFlashVariableObject* CreateMovieClip( const SUIMovieClipDesc*& pNewInstanceDesc, const char* movieClipTemplate, const char* mcParentName = NULL, const char* mcInstanceName = NULL );
	VIRTUAL IFlashVariableObject* CreateMovieClip( const SUIMovieClipDesc*& pNewInstanceDesc, const SUIMovieClipDesc* pMovieClipTemplateDesc, const SUIMovieClipDesc* pParentMC = NULL, const char* mcInstanceName = NULL );
	VIRTUAL void RemoveMovieClip( const char* movieClipName );
	VIRTUAL void RemoveMovieClip( const SUIParameterDesc* pMovieClipDesc );
	VIRTUAL void RemoveMovieClip( IFlashVariableObject* pVarObject );

	VIRTUAL bool SetVariable( const char* varName, const TUIData& value, const char* pTmplName = NULL );
	VIRTUAL bool SetVariable( const SUIParameterDesc* pVarDesc, const TUIData& value, const SUIMovieClipDesc* pTmplDesc = NULL );
	VIRTUAL bool GetVariable( const char* varName, TUIData& valueOut, const char* pTmplName = NULL );
	VIRTUAL bool GetVariable( const SUIParameterDesc* pVarDesc, TUIData& valueOut, const SUIMovieClipDesc* pTmplDesc = NULL );
	VIRTUAL bool CreateVariable( const SUIParameterDesc*& pNewDesc, const char* varName, const TUIData& value, const char* pTmplName = NULL );
	VIRTUAL bool CreateVariable( const SUIParameterDesc*& pNewDesc, const char* varName, const TUIData& value, const SUIMovieClipDesc* pTmplDesc = NULL );

	VIRTUAL bool SetArray( const char* arrayName, const SUIArguments& values, const char* pTmplName = NULL );
	VIRTUAL bool SetArray( const SUIParameterDesc* pArrayDesc, const SUIArguments& values, const SUIMovieClipDesc* pTmplDesc = NULL );
	VIRTUAL bool GetArray( const char* arrayName, SUIArguments& valuesOut, const char* pTmplName = NULL );
	VIRTUAL bool GetArray( const SUIParameterDesc* pArrayDesc, SUIArguments& valuesOut, const SUIMovieClipDesc* pTmplDesc = NULL );
	VIRTUAL bool CreateArray(  const SUIParameterDesc*& pNewDesc, const char* arrayName, const SUIArguments& values, const char* pTmplName = NULL );
	VIRTUAL bool CreateArray(  const SUIParameterDesc*& pNewDesc, const char* arrayName, const SUIArguments& values, const SUIMovieClipDesc* pTmplDesc = NULL );

	VIRTUAL void ScreenToFlash(const float& px, const float& py, float& rx, float & ry, bool bStageScaleMode = false) const;
	VIRTUAL void WorldToFlash(const Matrix34& camMat, const Vec3& worldpos, Vec3& flashpos, Vec2& borders, float& scale, bool bStageScaleMode = false) const;

	VIRTUAL void LoadTexIntoMc( const char* movieClip, ITexture* pTexture, const char* pTmplName = NULL  );
	VIRTUAL void LoadTexIntoMc( const SUIParameterDesc* pMovieClipDesc, ITexture* pTexture, const SUIMovieClipDesc* pTmplDesc = NULL );

	VIRTUAL void UnloadTexFromMc( const char* movieClip, ITexture* pTexture, const char* pTmplName = NULL  );
	VIRTUAL void UnloadTexFromMc( const SUIParameterDesc* pMovieClipDesc, ITexture* pTexture, const SUIMovieClipDesc* pTmplDesc = NULL );

	VIRTUAL void AddTexture( IDynTextureSource* pDynTexture );
	VIRTUAL void RemoveTexture( IDynTextureSource* pDynTexture );
	VIRTUAL int GetNumExtTextures() const { return m_textures.GetCount(); }
	VIRTUAL bool GetDynTexSize( int& width, int& height ) const { width = m_constraints.iWidth; height = m_constraints.iHeight; return m_constraints.eType == SUIConstraints::ePT_FixedDynTexSize; }

	VIRTUAL void SendCursorEvent( SFlashCursorEvent::ECursorState evt, int iX, int iY, int iButton = 0, float fWheel = 0.f );
	VIRTUAL void SendKeyEvent( const SFlashKeyEvent& evt );
	VIRTUAL void SendCharEvent( const SFlashCharEvent& charEvent );
	VIRTUAL void SendControllerEvent( EControllerInputEvent event, EControllerInputState state );

	VIRTUAL void GetMemoryUsage(ICrySizer * s) const;

	// IFSCommandHandler
	void HandleFSCommand( const char* pCommand, const char* pArgs, void* pUserData = 0 );
	// ~IFSCommandHandler

	// IVirtualKeyboardEvents
	virtual void KeyboardCancelled();
	virtual void KeyboardFinished(const wchar_t *pInString);
	// ~IVirtualKeyboardEvents


	void SetValid( bool bValid ) { if (m_pBaseInstance) { m_pBaseInstance->SetValid( bValid ); return; } if (!bValid) DestroyBootStrapper(); m_bIsValid = bValid; }

private:
	IFlashPlayerBootStrapper* InitBootStrapper();
	void DestroyBootStrapper();

	const SUIParameterDesc* GetOrCreateVariableDesc( const char* pVarName, bool* bExist = NULL );
	const SUIParameterDesc* GetOrCreateArrayDesc( const char* pArrayName, bool* bExist = NULL );
	const SUIMovieClipDesc* GetOrCreateMovieClipDesc( const char* pMovieClipName, bool* bExist = NULL );
	const SUIMovieClipDesc* GetOrCreateMovieClipTmplDesc( const char* pMovieClipTmplName );
	const SUIEventDesc* GetOrCreateFunctionDesc( const char* pFunctionName );

	bool SetVariableInt( const SUIParameterDesc* pVarDesc, const TUIData& value, const SUIMovieClipDesc* pTmplDesc, bool bCreate = false );

	inline void SetFlagInt( EFlashUIFlags flag, bool bSet );

	inline bool HasExtTexture() const { return !m_textures.IsEmpty(); }
	void UpdateFlags();
	bool HandleInternalCommand( const char* pCommand, const SUIArguments& args );

	struct SFlashObjectInfo
	{
		IFlashVariableObject* pObj;
		IFlashVariableObject* pParent;
		string sMember;
	};

	bool DefaultInfoCheck(SFlashObjectInfo*& pInfo, const SUIParameterDesc* pDesc, const SUIMovieClipDesc* pTmplDesc );

	SFlashObjectInfo* GetFlashVarObj( const SUIParameterDesc* pDesc, const SUIMovieClipDesc* pTmplDesc = NULL );
	void RemoveFlashVarObj( const SUIParameterDesc* pDesc );
	void FreeVarObjects();

	typedef std::vector<IUIElement*> TUIElements;
	typedef std::set< IUIElementEventListener* > TUIEventListenerUnique;
	inline TUIElements::iterator GetAllListeners(TUIEventListenerUnique& listeners, uint instanceID = 0);

	inline bool LazyInit();
	inline void ShowCursor();
	inline void HideCursor();

	inline const char* GetStringBuffer(const char* str);
	inline Vec3 MatMulVec3(const Matrix44& m, const Vec3& v) const;

	inline void ForceLazyUpdateInl() { m_bNeedLazyUpdate = true; m_bNeedLazyRender = false; }

	const SUIParameterDesc* GetDescForInfo(SFlashObjectInfo* pInfo, const SUIParameterDesc** pParent = NULL) const;
private:
	volatile int m_refCount;
	CFlashUI* m_pFlashUI;
	string m_sName;
	string m_sGroupName;
	string m_sFlashFile;
	float m_fAlpha;
	int m_iLayer;
	bool m_bIsValid;
	bool m_bIsHideRequest;
	IFlashPlayer* m_pFlashplayer;
	IFlashPlayerBootStrapper* m_pBootStrapper;
	const SUIMovieClipDesc* m_pRoot; 

	bool m_bVisible;
	uint64 m_iFlags;
	bool m_bCursorVisible;
	bool m_bNeedLazyUpdate;
	bool m_bNeedLazyRender;
	SUIConstraints m_constraints;
	XmlNodeRef m_baseInfo;
	typedef std::set< string > TStringBuffer;
	TStringBuffer m_StringBufferSet;

	CFlashUIElement* m_pBaseInstance;
	uint m_iInstanceID;
	TUIElements m_instances;

	TUIParamsLookup m_variables;
	TUIParamsLookup m_arrays;
	TUIMovieClipLookup m_displayObjects;
	TUIMovieClipLookup m_displayObjectsTmpl;
	TUIEventsLookup m_events;
	TUIEventsLookup m_functions;
	int m_firstDynamicDisplObjIndex;

	typedef CListenerSet< IUIElementEventListener* > TUIEventListener;
	TUIEventListener m_eventListener;

	typedef std::map< CCryName, SFlashObjectInfo > TVarMap;
	typedef std::map< const SUIParameterDesc*, SFlashObjectInfo* > TVarMapLookup;
	typedef std::map< const SUIParameterDesc*, TVarMapLookup > TTmplMapLookup;
	TVarMap m_variableObjects;
	TTmplMapLookup m_variableObjectsLookup;

	template <class T>
	struct SThreadSafeVec
	{
		typedef std::vector< T > TVec;

		TVec GetCopy() const
		{
			CryAutoCriticalSection lock(m_lock);
			return m_vec;
		}

		bool PushBackUnique(const T& v)
		{
			CryAutoCriticalSection lock(m_lock);
			return stl::push_back_unique(m_vec, v);
		}

		bool FindAndErase(const T& v)
		{
			CryAutoCriticalSection lock(m_lock);
			return stl::find_and_erase(m_vec, v);
		}

		size_t GetCount() const
		{
			CryAutoCriticalSection lock(m_lock);
			return m_vec.size();
		}

		bool IsEmpty() const
		{
			CryAutoCriticalSection lock(m_lock);
			return m_vec.empty();
		}

	private:
		TVec m_vec;
		mutable CryCriticalSection m_lock;
	};

	typedef SThreadSafeVec< IDynTextureSource* > TDynTextures;
	TDynTextures m_textures;

	friend struct SUIElementSerializer;
	friend struct CUIElementIterator;

#if !defined (_RELEASE)
	friend void RenderDebugInfo();
	typedef std::vector< CFlashUIElement* > TElementInstanceList;
	static TElementInstanceList s_ElementDebugList;
#endif
};

struct CUIElementIterator : public IUIElementIterator
{
	CUIElementIterator(const CFlashUIElement* pElement);
	VIRTUAL void AddRef();
	VIRTUAL void Release();
	VIRTUAL IUIElement* Next();
	VIRTUAL int GetCount() const { return m_pElement->m_instances.size(); }

private:
	int m_iRefs;
	const CFlashUIElement* m_pElement;
	CFlashUIElement::TUIElements::const_iterator m_currIter;
};

#endif // #ifndef __FlashUIElement_H__