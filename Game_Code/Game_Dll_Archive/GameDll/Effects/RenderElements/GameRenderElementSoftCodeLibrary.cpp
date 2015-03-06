//==================================================================================================
// Name: GameRenderElementSoftCodeLibrary
// Desc: Render Node Soft Code Library
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include <TypeLibrary.h>
#include "Effects/RenderElements/GameRenderElement.h"

#ifdef SOFTCODE
// Pull in the system and platform util functions
#include <platform_impl.h>
#endif

IMPLEMENT_TYPELIB(IGameRenderElement,GAME_RENDER_ELEMENT_LIBRARY_NAME); // Implementation of Soft Coding library