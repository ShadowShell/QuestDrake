// Copyright (c) 2013 Crytek GmbH
#pragma once

namespace Serialization { class IArchive; }

template<typename T>
inline bool Serialize(Serialization::IArchive& ar, Color_tpl<T>& c, const char* name, const char* label);

#include "ColorImpl.h"