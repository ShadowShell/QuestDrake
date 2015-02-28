// Copyright (c) 2012 Crytek GmbH
#pragma once

namespace Serialization
{
	struct IResourceSelector
	{
		const char* resourceType;

		virtual ~IResourceSelector() {}
		virtual const char* GetValue() const = 0;
		virtual void SetValue( const char* s ) = 0;
		virtual int GetId() const{ return -1; }
	};

	// TString could be SCRCRef or CCryName as well.
	template<class TString>
	struct ResourceSelector : IResourceSelector
	{
		TString& value;

		const char* GetValue() const { return value.c_str(); } 
		void SetValue( const char* s ) { value = s; }

		ResourceSelector(TString& value, const char* resourceType)
		: value(value)
		{
			this->resourceType = resourceType;
		}
	};

	struct ResourceSelectorWithId : IResourceSelector
	{
		string& value;
		int id;

		const char* GetValue() const { return value.c_str(); } 
		void SetValue( const char* s ) { value = s; }
		int GetId() const { return id; }

		ResourceSelectorWithId(string& value, const char* resourceType, int id)
		: value(value)
		, id(id)
		{
			this->resourceType = resourceType;
		}
	};

	// animation resources
	template<class T> ResourceSelector<T> AnimationPath(T& s) { return ResourceSelector<T>(s, "Animation"); }
	inline ResourceSelectorWithId AnimationPathWithId(string& s, int id) { return ResourceSelectorWithId(s, "Animation", id); }
	template<class T> ResourceSelector<T> CharacterPath(T& s) { return ResourceSelector<T>(s, "Character"); }
	template<class T> ResourceSelector<T> CharacterPhysicsPath(T& s) { return ResourceSelector<T>(s, "CharacterPhysics"); }
	template<class T> ResourceSelector<T> CharacterRigPath(T& s) { return ResourceSelector<T>(s, "CharacterRig"); }
	template<class T> ResourceSelector<T> SkeletonPath(T& s) { return ResourceSelector<T>(s, "Skeleton"); }
	template<class T> ResourceSelector<T> JointName(T& s) { return ResourceSelector<T>(s, "Joint"); }
	template<class T> ResourceSelector<T> AttachmentName(T& s) { return ResourceSelector<T>(s, "Attachment"); }

	// miscelaneous resources
	template<class T> ResourceSelector<T> SoundName(T& s) { return ResourceSelector<T>(s, "Sound"); }
	template<class T> ResourceSelector<T> DialogName(T& s) { return ResourceSelector<T>(s, "Dialog"); }
	template<class T> ResourceSelector<T> ForceFeedbackIdName(T& s) { return ResourceSelector<T>(s, "ForceFeedbackId"); }
	template<class T> ResourceSelector<T> ModelFilename(T& s) { return ResourceSelector<T>(s, "Model"); }
	template<class T> ResourceSelector<T> ParticleName(T& s) { return ResourceSelector<T>(s, "Particle"); }
	
	namespace Decorators 
	{
		// Decorators namespace is obsolete now, SHOULD NOT BE USED.
		template<class T> ResourceSelector<T> AnimationName(T& s) { return ResourceSelector<T>(s, "Animation"); }
		using Serialization::SoundName;
		using Serialization::AttachmentName;
		template<class T> ResourceSelector<T> ObjectFilename(T& s) { return ResourceSelector<T>(s, "Model"); }
		using Serialization::JointName;
		using Serialization::ForceFeedbackIdName;
	}

	template<class T>
	bool Serialize(IArchive&, ResourceSelector<T>& value, const char* name, const char* label);
	bool Serialize(IArchive&, ResourceSelectorWithId& value, const char* name, const char* label);
}

#include "ResourcesImpl.h"
