// Copyright (c) 2012 Crytek GmbH
// Authors: Evgeny Andreeshchev, Alexander Kotliar
// Based on: Yasli - the serialization library.

#pragma once

#include <vector>
#include <list>
#include <utility>
#include <map>

#include "Serialization/Serializer.h"

namespace Serialization{ class IArchive; }

namespace std 
{

template<class K, class V, class Alloc>
bool Serialize(Serialization::IArchive& ar, std::pair<K, V>& pair, const char* name, const char* label);

template<class T, class Alloc>
bool Serialize(Serialization::IArchive& ar, std::vector<T, Alloc>& container, const char* name, const char* label);

template<class T, class Alloc>
bool Serialize(Serialization::IArchive& ar, std::list<T, Alloc>& container, const char* name, const char* label);

template<class K, class V, class C, class Alloc>
bool Serialize(Serialization::IArchive& ar, std::map<K, V, C, Alloc>& container, const char* name, const char* label);

}

namespace Serialization 
{
bool Serialize(Serialization::IArchive& ar, Serialization::string& value, const char* name, const char* label);
bool Serialize(Serialization::IArchive& ar, Serialization::wstring& value, const char* name, const char* label);
}

#include "STLImpl.h"