// Copyright (c) 2012 Crytek GmbH
#pragma once

namespace Serialization { class IArchive; }

inline bool Serialize(Serialization::IArchive& ar, class CCryName& cryName, const char* name, const char* label);

#include "CryNameImpl.h"
