#pragma once
// Means to serialize and edit CryExtension pointers.

#ifdef GetClassName
#undef GetClassName
#endif
#include <CryExtension/ICryFactory.h>

namespace Serialization
{

// Allows to have boost::shared_ptr<TPointer> but serialize it by
// interface-casting to TSerializable, i.e. implementing Serialization through
// separate interface.
template<class TPointer, class TSerializable = TPointer>
struct CryExtensionPointer
{
	boost::shared_ptr<TPointer>& ptr;

	CryExtensionPointer(boost::shared_ptr<TPointer>& ptr) : ptr(ptr) {}
	void Serialize(Serialization::IArchive& ar);
};

}

// This function treats T as a type derived from CryUnknown type.
template<class T>
bool Serialize(Serialization::IArchive& ar, boost::shared_ptr<T>& ptr, const char* name, const char* label);

#include "CryExtensionImpl.h"