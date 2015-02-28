#pragma once

#include <Serialization/Enum.h>

namespace Serialization {

class IArchive;

struct BitFlagsWrapper{
	int* variable;
	unsigned int visibleMask;
	const CEnumDescription* description;

	void Serialize(IArchive& ar);
};

template<class Enum>
BitFlagsWrapper BitFlags(Enum& value)
{
	BitFlagsWrapper wrapper;
	wrapper.variable = (int*)&value;
	wrapper.description = &getEnumDescription<Enum>();
	return wrapper;
}

template<class Enum>
BitFlagsWrapper BitFlags(int& value)
{
	BitFlagsWrapper wrapper;
	wrapper.variable = &value;
	wrapper.visibleMask = ~0U;
	wrapper.description = &getEnumDescription<Enum>();
	return wrapper;
}

template<class Enum>
BitFlagsWrapper BitFlags(unsigned int& value, unsigned int visibleMask = ~0)
{
	BitFlagsWrapper wrapper;
	wrapper.variable = (int*)&value;
	wrapper.visibleMask = visibleMask;
	wrapper.description = &getEnumDescription<Enum>();
	return wrapper;
}

}

#include "BitFlagsImpl.h"