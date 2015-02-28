// Copyright (c) 2012 Crytek GmbH
#pragma once

namespace Serialization
{

template<class T>
bool Serialize(IArchive& ar, ResourceSelector<T>& value, const char* name, const char* label)
{
	if ( ar.IsEdit() )
		return ar(Serialization::SStruct::ForEdit(static_cast<IResourceSelector&>(value)), name, label);
	else
		return ar(value.value, name, label);
}

inline bool Serialize(IArchive& ar, ResourceSelectorWithId& value, const char* name, const char* label)
{
	if ( ar.IsEdit() )
		return ar(Serialization::SStruct::ForEdit(static_cast<IResourceSelector&>(value)), name, label);
	else
		return ar(value.value, name, label);
}

}
