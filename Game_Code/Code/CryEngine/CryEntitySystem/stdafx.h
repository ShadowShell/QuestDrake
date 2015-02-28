// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_)
#define AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <CryModuleDefs.h>
#define eCryModule eCryM_EntitySystem
#define RWI_NAME_TAG "RayWorldIntersection(EntitySys)"
#define PWI_NAME_TAG "PrimitiveWorldIntersection(EntitySys)"

#define CRYENTITYDLL_EXPORTS

#include <platform.h>

#pragma warning (error : 4018) //Cannot align catch objects to greater than 16 bytes 

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include MATH_H

#pragma warning (disable : 4768) //Cannot align catch objects to greater than 16 bytes 
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include <map>


//////////////////////////////////////////////////////////////////////////

#include "Cry_Math.h"
#include <Cry_Camera.h>
#include "Cry_XOptimise.h"
#include "Cry_GeoIntersect.h"
#include <CrySizer.h>
#include <smartptr.h>
#include <StlUtils.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


//#include "EntityDesc.h"
//#include <IEntitySystem.h>

#include <ISystem.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <I3DEngine.h>
#include <TimeValue.h>
#include <PoolAllocator.h>
#include <IEntitySystem.h>

#include "EntityCVars.h"

#if !defined(_RELEASE)
#define INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE
#endif // !_RELEASE

//////////////////////////////////////////////////////////////////////////
class CEntitySystem;
extern CEntitySystem *g_pIEntitySystem;	
ILINE CEntitySystem* GetIEntitySystem() { return g_pIEntitySystem; }
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//! Reports a Game Warning to validator with WARNING severity.
inline void EntityWarning( const char *format,... ) PRINTF_PARAMS(1, 2);
inline void EntityWarning( const char *format,... )
{
	if (!format)
		return;

	va_list args;
	va_start(args, format);
	gEnv->pSystem->WarningV( VALIDATOR_MODULE_ENTITYSYSTEM,VALIDATOR_WARNING,0,NULL,format,args );
	va_end(args);
}

inline void EntityFileWarning( const char *file,const char *format,... ) PRINTF_PARAMS(2, 3);
inline void EntityFileWarning( const char *file,const char *format,... )
{
	if (!format)
		return;

	va_list args;
	va_start(args, format);
	gEnv->pSystem->WarningV( VALIDATOR_MODULE_ENTITYSYSTEM,VALIDATOR_WARNING,0,file,format,args );
	va_end(args);
}


//////////////////////////////////////////////////////////////////////////
// Proxy creation functions.
//////////////////////////////////////////////////////////////////////////
extern IEntityProxyPtr CreateScriptProxy( class CEntity *pEntity,struct IEntityScript *pScript,SEntitySpawnParams *pSpawnParams );

//! Return random value in [-1,1] range.
inline float frand()
{
	return cry_frand()*2.0f - 1.0f;
}


#define ENTITY_PROFILER FUNCTION_PROFILER_FAST( GetISystem(),PROFILE_ENTITY,g_bProfilerEnabled );
#define ENTITY_PROFILER_NAME(str) FRAME_PROFILER_FAST( str,GetISystem(),PROFILE_ENTITY,g_bProfilerEnabled );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2E966576_9327_4B66_9CFD_329F604BE709__INCLUDED_)
