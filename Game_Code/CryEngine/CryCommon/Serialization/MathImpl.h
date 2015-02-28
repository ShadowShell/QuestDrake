// Copyright (c) 2013 Crytek GmbH
#pragma once

#include "Serialization/IArchive.h"

#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Vector4.h"
#include "Cry_Quat.h"
#include "Cry_Matrix34.h"

//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableVec2 : Vec2_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->x, "x", "X");
		ar(this->y, "y", "Y");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec2_tpl<T>& value, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES)) // used for XML and VariableOArchive, needs consolidation
		return Serialize(ar, static_cast<SerializableVec2<T>&>(value), name, label);
	else
	{
		typedef T(*Array)[2];
		return ar(*((Array)&value.x), name, label);
	}
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableVec3 : Vec3_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->x, "x", "X");
		ar(this->y, "y", "Y");
		ar(this->z, "z", "Z");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Vec3_tpl<T>& value, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES))
		return Serialize(ar, static_cast<SerializableVec3<T>&>(value), name, label);
	else
	{
		typedef T(*Array)[3];
		return ar(*((Array)&value.x), name, label);
	}
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableVec4 : Vec4_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->x, "x", "X");
		ar(this->y, "y", "Y");
		ar(this->z, "z", "Z");
		ar(this->w, "w", "W");
	}
};

template<typename T>
inline bool Serialize(Serialization::IArchive& ar, struct Vec4_tpl<T>& v, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES))
		return Serialize(ar, static_cast<SerializableVec4<T>&>(v), name, label);
	else
	{
		typedef T(*Array)[4];
		return ar(*((Array)&v.x), name, label);
	}
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableQuat : Quat_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->v.x, "x", "X");
		ar(this->v.y, "y", "Y");
		ar(this->v.z, "z", "Z");
		ar(this->w, "w", "W");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct Quat_tpl<T>& value, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES))
		return Serialize(ar, static_cast<SerializableQuat<T>&>(value), name, label);
	else
	{
		typedef T(*Array)[4];
		return ar(*((Array)&value.v.x), name, label);
	}
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableQuatT : QuatT_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->q, "q", "Quaternion");
		ar(this->t, "t", "Translation");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, struct QuatT_tpl<T>& value, const char* name, const char* label)
{
	return Serialize(ar, static_cast<SerializableQuatT<T>&>(value), name, label);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableMatrix34 : Matrix34_tpl<T>
{
	struct SRow
	{
		float x, y, z, w;
		void Serialize(Serialization::IArchive& ar)
		{
			ar(x, "x", "^");
			ar(y, "y", "^");
			ar(z, "z", "^");
			ar(w, "w", "^");
		}
	};

	void Serialize(Serialization::IArchive& ar)
	{
		ar(*(SRow*)(&this->m00), "r0", "<");
		ar(*(SRow*)(&this->m10), "r1", "<");
		ar(*(SRow*)(&this->m20), "r2", "<");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Matrix34_tpl<T>& value, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES)) 
		return ar(static_cast<SerializableMatrix34<T>&>(value), name, label);
	else	
	{
		typedef T (*Array)[3][4];
		T* m = &value.m00;
		return ar(*((Array)m), name, label);
	}
}

//////////////////////////////////////////////////////////////////////////


namespace Serialization
{

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadiansAsDeg<T>& value, const char* name, const char* label)
{
	if (ar.IsEdit()) 
	{
		float degrees = RAD2DEG(*value.radians);
		float oldDegrees = degrees;
		if (!ar(degrees, name, label))
			return false;
		if (oldDegrees != degrees)
			*value.radians = DEG2RAD(degrees);
		return true;
	}
	else
		return ar(*value.radians, name, label);
}

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::SRadianAng3AsDeg<T>& value, const char* name, const char* label)
{
	if (ar.IsEdit()) 
	{
		Ang3 degrees(RAD2DEG(value.ang3->x), RAD2DEG(value.ang3->y), RAD2DEG(value.ang3->z));
		Ang3 oldDegrees = degrees;
		if (!ar(degrees, name, label))
			return false;
		if (oldDegrees != degrees)
			*value.ang3 = Ang3(DEG2RAD(degrees.x), DEG2RAD(degrees.y), DEG2RAD(degrees.z));
		return true;
	}
	else
		return ar(*value.ang3, name, label);
}

}

//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableAng3 : Ang3_tpl<T>
{
	void Serialize(Serialization::IArchive& ar)
	{
		ar(this->x, "x", "X");
		ar(this->y, "y", "Y");
		ar(this->z, "z", "Z");
	}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Ang3_tpl<T>& value, const char* name, const char* label)
{
	if (ar.GetCaps(ar.NO_EMPTY_NAMES))
		return Serialize(ar, static_cast<SerializableAng3<T>&>(value), name, label);
	else
	{
		typedef T(*Array)[3];
		return ar(*((Array)&value.x), name, label);
	}
}

//////////////////////////////////////////////////////////////////////////

namespace Serialization
{

template<class T>
bool Serialize(Serialization::IArchive& ar, Serialization::AsAng3<T>& value, const char* name, const char* label)
{
	if (ar.IsEdit()) 
	{
		Ang3 ang3(*value.quat);
		Ang3 oldAng3 = ang3;
		if (!ar(Serialization::RadiansAsDeg(ang3), name, label))
			return false;
		if (ang3 != oldAng3)
			*value.quat = Quat(ang3);
		return true;
	}
	else
		return ar(*value.quat, name, label);
}

}
