#pragma once

#include "Serialization/IArchive.h"

namespace Serialization {

inline void BitFlagsWrapper::Serialize(IArchive& ar)
{
	const Serialization::CEnumDescription& desc = *description;
	int count = desc.count();
	if (ar.IsInput()) {
		int previousValue = *variable;
		for (size_t i = 0; i < count; ++i) {
			int flagValue = desc.valueByIndex(i);
			if (!(flagValue & visibleMask))
				continue;
			bool flag = (previousValue & flagValue) == flagValue;
			bool previousFlag = flag;
			ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
			if (flag != previousFlag)
			{
				if (flag)
					*variable |= flagValue;
				else
					*variable &= ~flagValue;
			}
		}
	}
	else {
		for (size_t i = 0; i < count; ++i) {
			int flagValue = desc.valueByIndex(i);
			if (!(flagValue & visibleMask))
				continue;
			bool flag = (*variable & flagValue) == flagValue;
			ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
		}
	}
}

}
