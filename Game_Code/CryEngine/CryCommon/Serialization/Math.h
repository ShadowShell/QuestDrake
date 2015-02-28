// Copyright (c) 2013 Crytek GmbH
#pragma once

// This header extends serialization to support common geometrical types.
// It allows serialization of mentioned below types simple passing them to archive.
// For example:
//
//   #include <Serialization/Math.h>
//   #include <Serialization/IArchive.h>
//
//   Serialization::IArchive& ar;
//
//   Vec3 v;
//   ar(v, "v");
//
//   QuatT q;
//   ar(q, "q");
//
namespace Serialization { class IArchive; }

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Vec2_tpl<T>& v, const char* name, const char* label);

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Vec3_tpl<T>& v, const char* name, const char* label);

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Vec4_tpl<T>& v, const char* name, const char* label);

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Quat_tpl<T>& q, const char* name, const char* label);

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct QuatT_tpl<T>& qt, const char* name, const char* label);

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Ang3_tpl<T>& a, const char* name, const char* label);

template<class T>
bool Serialize(Serialization::IArchive& ar, struct Matrix34_tpl<T>& value, const char* name, const char* label);

// ---------------------------------------------------------------------------
// RadiansAsDeg allows to present radian values as degrees to the user in the
// editor.
//
// Example:
//   ...
//   float radians;
//   ar(RadiansAsDeg(radians), "degrees", "Degrees");
//
//   Ang3 euler;
//   ar(RadiansAsDeg(euler), "eulerDegrees", "Euler Degrees");
//
namespace Serialization 
{
template<class T>
struct SRadianAng3AsDeg
{
	Ang3_tpl<T>* ang3;
	SRadianAng3AsDeg(Ang3_tpl<T>* ang3) : ang3(ang3) {}
};

template<class T>
SRadianAng3AsDeg<T> RadiansAsDeg(Ang3_tpl<T>& radians)
{
	return SRadianAng3AsDeg<T>(&radians);
}

template<class T>
struct SRadiansAsDeg 
{
	T* radians;
	SRadiansAsDeg(T* radians) : radians(*radians) {}
};

template<class T>
SRadiansAsDeg<T> RadiansAsDeg(T& radians)
{
	return SRadiansAsDeg<T>(&radians);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadiansAsDeg<T>& value, const char* name, const char* label);
template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadianAng3AsDeg<T>& value, const char* name, const char* label);

// ---------------------------------------------------------------------------
// AsAng3 provides a wrapper that allows editing of quaternions as Ang3 (in degrees).
//
// Example:
//   ...
//   Quat q;
//   ar(AsAng3(q), "orientation", "Orientation");
//

template<class T>
struct AsAng3
{
	Quat_tpl<T>* quat;
	AsAng3(Quat_tpl<T>& quat) : quat(&quat) {}
};

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::AsAng3<T>& value, const char* name, const char* label);

}

#include "MathImpl.h"
