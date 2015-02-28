#pragma once

#include <vector>
#include "Serialization/IArchive.h"
#include "Serialization/Strings.h"
#include "Serialization/STL.h"

struct TagListContainer : Serialization::ContainerSTL<std::vector<Serialization::string>, Serialization::string>
{
	TagListContainer(TagList& tagList)
	: ContainerSTL(tagList.tags)
	{
	}

	Serialization::TypeID containerType() const override{ return Serialization::TypeID::get<TagList>(); };
};

inline bool Serialize(Serialization::IArchive& ar, TagList& tagList, const char* name, const char* label)
{
	TagListContainer container(tagList);
	return ar(static_cast<Serialization::IContainer&>(container), name, label);
}
