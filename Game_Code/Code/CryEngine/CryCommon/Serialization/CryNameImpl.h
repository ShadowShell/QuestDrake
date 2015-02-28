// Copyright (c) 2012 Crytek GmbH
#pragma once

#include "CryName.h"
#include "IArchive.h"

class CryNameSerializer : public Serialization::IString
{
public:
	CryNameSerializer(CCryName& s)
		: m_s(s)
	{
	}

	virtual void set(const char* value)
	{
		m_s = value;
	}

	virtual const char* get() const
	{
		return m_s.c_str();
	}

	CCryName& m_s;
};


inline bool Serialize( Serialization::IArchive& ar, CCryName& cryName, const char* name, const char* label)
{
	CryNameSerializer serializer(cryName);
	return ar(static_cast<Serialization::IString&>(serializer), name, label);
}

