#ifndef _IGAME_RENDERELEMENT_
#define _IGAME_RENDERELEMENT_

#pragma once

// Includes
#include "Effects/GameEffectsSystem.h"
#include "CREGameEffect.h"

// Forward declares
struct IGameRenderElementParams;

//==================================================================================================
// Name: IGameRenderElement
// Desc: Base interface for all game render elements
// Author: James Chilvers
//==================================================================================================
struct IGameRenderElement : public IREGameEffect, public _i_reference_target_t
{
	DECLARE_TYPELIB(IGameRenderElement); // Allow soft coding on this interface

	virtual ~IGameRenderElement() {}

	virtual bool InitialiseGameRenderElement() = 0;
	virtual void ReleaseGameRenderElement() = 0;
	virtual void UpdatePrivateImplementation() = 0;

	virtual CREGameEffect*	GetCREGameEffect() = 0;

	virtual IGameRenderElementParams* GetParams() = 0;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CGameRenderElement
// Desc: Base class for all game render elements
// Author: James Chilvers
//==================================================================================================
class CGameRenderElement : public IGameRenderElement
{
	DECLARE_TYPE(CGameRenderElement, IGameRenderElement); // Exposes this type for SoftCoding
public:
	CGameRenderElement();
	virtual ~CGameRenderElement() {}

	virtual bool InitialiseGameRenderElement();
	virtual void ReleaseGameRenderElement();
	virtual void UpdatePrivateImplementation();

	virtual CREGameEffect*	GetCREGameEffect();

protected:
	CREGameEffect*		SOFT(m_pREGameEffect);
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: IGameRenderElementParams
// Desc: Game Render Element params
// Author: James Chilvers
//==================================================================================================
struct IGameRenderElementParams
{
	virtual ~IGameRenderElementParams() {}
};//------------------------------------------------------------------------------------------------

#endif // _IGAME_RENDERELEMENT_
