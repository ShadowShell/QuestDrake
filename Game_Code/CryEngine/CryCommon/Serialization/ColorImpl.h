// Copyright (c) 2013 Crytek GmbH
#pragma once

#include "Color.h"
#include "Cry_Color.h"

//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableColorF : Color_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->r, "r", "R");
		ar(this->g, "g", "G");
		ar(this->b, "b", "B");
		ar(this->a, "a", "A");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Color_tpl<T>& c, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES))
		return Serialize(ar, static_cast<SerializableColorF<T>&>(c), name, label);
	else
	{
		typedef T(*Array)[4];
		return ar(*((Array)&c.r), name, label);
	}
}
