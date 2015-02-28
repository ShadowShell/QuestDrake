// Copyright (c) 2012 Crytek GmbH
#pragma once

namespace Serialization
{

template<class T>
bool Serialize(IArchive& ar, RangeDecorator<T>& value, const char* name, const char* label)
{
	if (ar.IsEdit())
		return ar(SStruct::ForEdit(value), name, label);
	else
	{
		if (!ar(*value.value, name, label))
			return false;
		if (ar.IsInput())
			*value.value = clamp_tpl(*value.value, value.hardMin, value.hardMax);
		return true;
	}
}

}
