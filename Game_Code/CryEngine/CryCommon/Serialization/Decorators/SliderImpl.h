#pragma once

#include "Slider.h"
#include "Serialization/IArchive.h"

namespace Serialization
{

inline bool Serialize(IArchive& ar, SSliderF& slider, const char* name, const char* label)
{
	if (ar.IsEdit()) 
		return ar(SStruct::ForEdit(slider), name, label);
	else
		return ar(*slider.valuePointer, name, label);
}

inline bool Serialize(IArchive& ar, SSliderI& slider, const char* name, const char* label)
{
	if (ar.IsEdit()) 
		return ar(SStruct::ForEdit(slider), name, label);
	else
		return ar(*slider.valuePointer, name, label);
}

}