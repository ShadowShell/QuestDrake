#pragma once
namespace Serialization {

class IArchive;

class IKeyValue : IString
{
public:
	virtual const char* get() const = 0;
	virtual void set(const char* key) = 0;
	virtual bool serializeValue(IArchive& ar, const char* name, const char* label) = 0;
	template<class TArchive> void Serialize(TArchive& ar)
	{
		ar(*(IString*)this, "", "^");
		serializeValue(ar, "", "^");
	}
};

}

