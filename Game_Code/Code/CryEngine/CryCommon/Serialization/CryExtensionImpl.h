#pragma once

#include <Serialization/StringList.h>
#include <CryExtension/ICryFactoryRegistry.h>
#include <CryExtension/CryTypeID.h>
#include <ISystem.h>

namespace Serialization {

// Generate user-friendly class name, e.g. convert
//  "AnimationPoseModifier_FootStore" -> "Foot Store"
inline string MakePrettyClassName(const char* className)
{
	const char* firstSep = strchr(className, '_');
	if (!firstSep)
	{
		// name doesn't follow expected convention, return as is
		return className;
	}

	const char* start = firstSep + 1;
	string result;
	result.reserve(strlen(start) + 4);

	const char* p = start;
	while (*p != '\0')
	{
		if (*p >= 'A' && *p <= 'Z' &&
			*(p-1) >= 'a' && *(p-1) <= 'z')
		{
			result += ' ';
		}
		if (*p == '_')
			result += ' ';
		else
			result += *p;
		++p;
	}

	return result;
}

template<class TPointer, class TSerializable>
void CryExtensionPointer<TPointer, TSerializable>::Serialize(IArchive& ar)
{
	ICryFactoryRegistry* factoryRegistry = gEnv->pSystem->GetCryFactoryRegistry();

	if (!ar.IsEdit())
	{
		CryClassID previousClassId = { 0, 0 };
		if (ptr)
			previousClassId = boost::static_pointer_cast<TPointer>(ptr)->GetFactory()->GetClassID();
		CryClassID classId = previousClassId;
		ar(classId.hipart, "guidHiPart");
		ar(classId.lopart, "guidLoPart");

		if (ar.IsInput())
		{
			if (classId != previousClassId)
			{
				if (ICryFactory* factory = factoryRegistry->GetFactory(classId))
					ptr = boost::static_pointer_cast<TPointer>(factory->CreateClassInstance());
			}
		}

		if (TSerializable* ser = cryinterface_cast<TSerializable>(ptr.get()))
			ser->Serialize(ar);
		else if (ar.IsInput())
			ptr.reset();
	}
	else
	{
		static std::vector<CryInterfaceID> classIds;
		static StringList prettyNames;
		if (classIds.empty())
		{
			classIds.push_back(CryInterfaceID());
			prettyNames.push_back("None");

			size_t factoryCount = 0;
			factoryRegistry->IterateFactories(cryiidof<TPointer>(), 0, factoryCount);
			PREFAST_SUPPRESS_WARNING(6255)
			ICryFactory** factories = (ICryFactory**)(factoryCount ? alloca(sizeof(ICryFactory*) * factoryCount) : 0);
			PREFAST_ASSUME(factories);
			factoryRegistry->IterateFactories(cryiidof<TPointer>(), factories, factoryCount);

			for (size_t i = 0; i < factoryCount; ++i)
			{
				if (!factories[i]->ClassSupports(cryiidof<TSerializable>()))
					continue;
				classIds.push_back(factories[i]->GetClassID());
				prettyNames.push_back(MakePrettyClassName(factories[i]->GetClassName()));
			}
		}	 

		std::size_t previousIndex = 0;
		if (ptr)
		{
			previousIndex = (int)std::distance(classIds.begin(), std::find(classIds.begin(), classIds.end(), ptr->GetFactory()->GetClassID()));
			if (previousIndex >= classIds.size())
				previousIndex = 0;
		}

		StringListValue name(prettyNames, previousIndex);
		ar(name, "type", "<^");
		if (name.index() != previousIndex)
		{
			if (name.index() == 0)
				ptr.reset();
			else
			{
				ICryFactory* factory = factoryRegistry->GetFactory(classIds[name.index()]);
				if (factory) 
					ptr = boost::static_pointer_cast<TPointer>(factory->CreateClassInstance());
			}
		}

		if (ptr)
			if (TSerializable* ser = cryinterface_cast<TSerializable>(ptr.get()))
				ser->Serialize(ar);
	}
}

}

template<class T>
bool Serialize(Serialization::IArchive& ar, boost::shared_ptr<T>& ptr, const char* name, const char* label)
{
	Serialization::CryExtensionPointer<T, T> extensionPointer(ptr);
	return ar(extensionPointer, name, label);
}
