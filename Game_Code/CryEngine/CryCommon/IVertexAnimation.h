////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
////////////////////////////////////////////////////////////////////////////

#ifndef IVertexAnimation_h
#define IVertexAnimation_h

struct IVertexFrames
{
	virtual ~IVertexFrames() { }

	virtual uint GetCount() const = 0;

	virtual const char* GetNameByIndex(const uint index) const = 0;
	virtual uint GetIndexByName(const char* name) const = 0;
};

struct IVertexAnimation
{
	virtual ~IVertexAnimation() { }

	virtual void SetFrameWeightByName(const ISkin* pISkin, const char* name, float weight) = 0;
	virtual void ClearAllFramesWeight() = 0;
};

#endif // IVertexAnimation_h
