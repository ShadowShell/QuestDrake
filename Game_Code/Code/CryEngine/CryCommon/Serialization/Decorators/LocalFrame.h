#pragma once

#include <Cry_Math.h>
#include "Serialization/Math.h"

namespace Serialization
{

class IArchive;

struct LocalPosition
{
	Vec3* value;
	int space;
	const char* parentName;

	LocalPosition(Vec3& vec, int space, const char* parentName)
	: value(&vec)
	, space(space)
	, parentName(parentName)
	{
	}

	void Serialize(IArchive& ar);
};

struct LocalOrientation
{
	Quat* value;
	int space;
	const char* parentName;

	LocalOrientation(Quat& vec, int space, const char* parentName)
	: value(&vec)
	, space(space)
	, parentName(parentName)
	{
	}

	void Serialize(IArchive& ar);
};

struct LocalFrame
{
	QuatT* value;
	const char* parentName;
	int space;

	LocalFrame(QuatT& vec, int space, const char* parentName)
	: value(&vec)
	, space(space)
	, parentName(parentName)
	{
	}

	void Serialize(IArchive& ar);
};

enum 
{
	SPACE_JOINT,
	SPACE_ENTITY,
	SPACE_JOINT_WITH_PARENT_ROTATION
};

inline LocalFrame LocalToJoint(QuatT& transform, const string& jointName)
{
	return LocalFrame(transform, SPACE_JOINT, jointName.c_str());
}

inline LocalOrientation LocalToJoint(Quat& orientation, const string& jointName)
{
	return LocalOrientation(orientation, SPACE_JOINT, jointName.c_str());
}

inline LocalOrientation LocalToEntity(Quat& orientation)
{
	return LocalOrientation(orientation, SPACE_ENTITY, "");
}

inline LocalPosition LocalToJoint(Vec3& position, const string& jointName)
{
	return LocalPosition(position, SPACE_JOINT, jointName.c_str());
}

bool Serialize(Serialization::IArchive& ar, Serialization::LocalPosition& value, const char* name, const char* label);
bool Serialize(Serialization::IArchive& ar, Serialization::LocalOrientation& value, const char* name, const char* label);
bool Serialize(Serialization::IArchive& ar, Serialization::LocalFrame& value, const char* name, const char* label);

}

#include "LocalFrameImpl.h"
