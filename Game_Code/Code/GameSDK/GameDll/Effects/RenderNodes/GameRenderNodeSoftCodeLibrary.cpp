//==================================================================================================
// Name: GameRenderNodeSoftCodeLibrary
// Desc: Render Node Soft Code Library
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include <TypeLibrary.h>
#include "Effects/RenderNodes/IGameRenderNode.h"

#ifdef SOFTCODE
// Pull in the system and platform util functions
#include <platform_impl.h>
#endif

IMPLEMENT_TYPELIB(IGameRenderNode,GAME_RENDER_NODE_LIBRARY_NAME); // Implementation of Soft Coding library