#pragma once

#include "Serialization/Strings.h"

namespace Serialization
{

class IArchive;

struct ResourceFilePath
{
	enum { STRIP_EXTENSION = 1 << 0 };

	string* path;
	string filter;
	string startFolder;
	int flags;

	// filters are defined in the following format:
	// "All Images (bmp, jpg, tga)|*.bmp;*.jpg;*.tga|Targa (tga)|*.tga"
	explicit ResourceFilePath(string& path, const char* filter = "All files|*.*", const char* startFolder = "", int flags = 0)
	: path(&path)
	, filter(filter)
	, startFolder(startFolder)
	, flags(flags)
	{
	}

	// the function should stay virtual to ensure cross-dll calls are using right heap
	virtual void SetPath(const char* path) { *this->path = path; }
};

inline ResourceFilePath MaterialPath(string& path)
{
	return ResourceFilePath(path, "Materials (mtl)|*.mtl", "Materials", ResourceFilePath::STRIP_EXTENSION);
}

bool Serialize(Serialization::IArchive& ar, Serialization::ResourceFilePath& value, const char* name, const char* label);

}

#include "ResourceFilePathImpl.h"