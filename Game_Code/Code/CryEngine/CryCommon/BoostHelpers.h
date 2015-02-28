/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 
Helper macros/methods/classes for boost.
-------------------------------------------------------------------------
History:
- 02:07:2009: Created by Alex McCarthy
*************************************************************************/

#ifndef __BOOST_HELPERS_H__
#define __BOOST_HELPERS_H__

#if defined(ORBIS)
// Boost expects type_info in the global namespace, to import
// it into the std namespace. But Orbis already provides
// typeinfo in the std namespace. Hence import type_info
// into the global namespace, to allow boost to compile
#include <typeinfo>
using std::type_info; 
#endif // ORBIS

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/enable_shared_from_this.hpp>

#ifndef __SPU__
#include <boost/mem_fn.hpp>
#endif

#define DECLARE_BOOST_POINTERS(name) \
	typedef boost::shared_ptr<name> name##Ptr; \
	typedef boost::shared_ptr<const name> name##ConstPtr; \
	typedef boost::weak_ptr<name> name##WeakPtr; \
	typedef boost::weak_ptr<const name> name##ConstWeakPtr;

// HACK for pre-VS2013 builds to avoid macro redefinitions
// Sandbox includes afxcontrolbars.h, boost variant and mpl include stdint.h.
// Both define the following macros unconditionally...

#if defined(SANDBOX_EXPORTS)
#	if defined(INT8_MIN)
#		undef INT8_MIN
#	endif
#	if defined(INT16_MIN)
#		undef INT16_MIN
#	endif
#	if defined(INT32_MIN)
#		undef INT32_MIN
#	endif
#	if defined(INT64_MIN)
#		undef INT64_MIN
#	endif

#	if defined(INT8_MAX)
#		undef INT8_MAX
#	endif
#	if defined(INT16_MAX)
#		undef INT16_MAX
#	endif
#	if defined(INT32_MAX)
#		undef INT32_MAX
#	endif
#	if defined(INT64_MAX)
#		undef INT64_MAX
#	endif

#	if defined(UINT8_MIN)
#		undef UINT8_MIN
#	endif
#	if defined(UINT16_MIN)
#		undef UINT16_MIN
#	endif
#	if defined(UINT32_MIN)
#		undef UINT32_MIN
#	endif
#	if defined(UINT64_MIN)
#		undef UINT64_MIN
#	endif

#	if defined(UINT8_MAX)
#		undef UINT8_MAX
#	endif
#	if defined(UINT16_MAX)
#		undef UINT16_MAX
#	endif
#	if defined(UINT32_MAX)
#		undef UINT32_MAX
#	endif
#	if defined(UINT64_MAX)
#		undef UINT64_MAX
#	endif
#endif // #if defined(SANDBOX_EXPORTS)

#pragma warning(push)
#pragma warning(disable : 4345)
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
#pragma warning(pop)

#endif // __BOOST_HELPERS_H__
