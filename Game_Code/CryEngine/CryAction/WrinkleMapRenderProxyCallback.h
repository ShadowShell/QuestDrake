// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __WrinkleMapRenderProxyCallback_H__
#define __WrinkleMapRenderProxyCallback_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IShaderParamCallback.h>
#include <CryExtension/Impl/ClassWeaver.h>

class CWrinkleMapShaderParamCallback : public IShaderParamCallback
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IShaderParamCallback)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CWrinkleMapShaderParamCallback, "WrinkleMapShaderParamCallback", 0x68c7f0e0c36446fe, 0x82a3bc01b54dc7bf)

public:

	//////////////////////////////////////////////////////////////////////////
	//	Implement IShaderParamCallback
	//////////////////////////////////////////////////////////////////////////

	virtual void SetCharacterInstance(ICharacterInstance* pCharInstance)
	{
		m_pCharacterInstance = pCharInstance;
	}

	virtual ICharacterInstance* GetCharacterInstance() const
	{
		return m_pCharacterInstance;
	}

	virtual bool Init();
	virtual void SetupShaderParams(IShaderPublicParams* pParams, IMaterial* pMaterial);
	virtual void Disable( IShaderPublicParams* pParams );

protected:
	
	void SetupBoneWrinkleMapInfo();

	//////////////////////////////////////////////////////////////////////////
	
	ICharacterInstance* m_pCharacterInstance;

	struct SWrinkleBoneInfo
	{
		int16 m_nChannelID;
		int16 m_nJointID;
	};
	typedef std::vector<SWrinkleBoneInfo> TWrinkleBoneInfo;
	TWrinkleBoneInfo m_WrinkleBoneInfo;

	IMaterial* m_pCachedMaterial;

	uint8 m_eSemantic[3];

	bool m_bWrinklesEnabled;
};

CREATE_SHADERPARAMCALLBACKUI_CLASS( CWrinkleMapShaderParamCallback, "bWrinkleMap", 0x1B9D46925918485B, 0xB7312C8FB3F5B763 )

#endif //__WrinkleMapRenderProxyCallback_H__
