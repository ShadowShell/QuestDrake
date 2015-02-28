#ifndef __MODDESCRIPTION_H__
#define __MODDESCRIPTION_H__

enum EModInfoType
{
	eMIT_UNDEFINED,
	eMIT_SPMOD,
	eMIT_MPMOD,
	eMIT_SPANDMPMOD,
	eMIT_SPLEVEL,
};

// intended to be used for communication with UI
struct ModInfo
{
	CryFixedStringT<256> keyName; // Directory name
	CryFixedStringT<256> displayName;
	EModInfoType modType;
	CryFixedStringT<256> description;
	CryFixedStringT<256> mainImage;
	CryFixedStringT<256> logoImage;
	CryFixedStringT<256> author;
	CryFixedStringT<256> website;
	int versionMajor;
	int versionMinor;

	ModInfo()
	: modType(eMIT_UNDEFINED)
	, versionMajor(0)
	, versionMinor(0)
	{
	}

	bool operator<(const ModInfo& rhs) const
	{
		if (keyName == rhs.keyName)
			return modType < rhs.modType;

		return keyName < rhs.keyName;
	}
};

#endif

