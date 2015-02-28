// Copyright (c) 2013 Crytek GmbH
#pragma once

namespace Serialization
{
	class IArchive;
}

template <uint32 StoreStrings, typename THash>
bool Serialize(Serialization::IArchive& ar, struct SCRCRef<StoreStrings, THash>& crcRef, const char* name, const char* label);

#include "CRCRefImpl.h"