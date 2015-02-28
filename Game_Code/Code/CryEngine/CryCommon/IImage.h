#pragma once

#ifndef IIMAGE_H
#define IIMAGE_H

/**
 * Possible errors for IImageFile::mfGet_error.
 */
enum EImFileError { eIFE_OK = 0, eIFE_IOerror, eIFE_OutOfMemory, eIFE_BadFormat, eIFE_ChunkNotFound };

#define FIM_NORMALMAP						0x0001
#define FIM_NOTSUPPORTS_MIPS		0x0004
#define FIM_ALPHA								0x0008	// request attached alpha image
#define FIM_DECAL								0x0010
#define FIM_GREYSCALE						0x0020	// hint this texture is greyscale (could be DXT1 with colored artifacts)
#define FIM_STREAM_PREPARE			0x0080
#define FIM_FILESINGLE					0x0100	// info from rc: no need to search for other files (e.g. DDNDIF)
#define FIM_BIG_ENDIANNESS			0x0400	// for textures converted to big endianness format
#define FIM_SPLITTED						0x0800	// for dds textures stored in splitted files
#define FIM_SRGB_READ						0x1000
#define FIM_X360_NOT_PRETILED		0x2000	// for dds textures that cannot be pretiled
#define FIM_READ_VIA_STREAMS		0x4000	// issue file reads through the stream engine, for cases where reads may contend with the disc
#define	FIM_DX10IO							0x8000	// for dds textures with extended DX10+ header
#define FIM_HAS_ATTACHED_ALPHA	0x10000	// image has an attached alpha image
#define FIM_SUPPRESS_DOWNSCALING  0x20000 // don't allow to drop mips when texture is non-streamable


class IImageFile
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;

	virtual const string& mfGet_filename () const = 0;

  virtual int mfGet_width () const = 0;
  virtual int mfGet_height () const = 0;
	virtual int mfGet_depth () const = 0;
	virtual int mfGet_NumSides () const = 0;

	virtual EImFileError mfGet_error () const = 0;

  virtual byte* mfGet_image (const int nSide) = 0;
  virtual bool mfIs_image (const int nSide) const = 0;

  virtual ETEX_Format mfGetFormat() const = 0;
	virtual ETEX_TileMode mfGetTileMode() const = 0;
  virtual int  mfGet_numMips () const = 0;
	virtual int mfGet_numPersistantMips () const = 0;
  virtual int mfGet_Flags () const = 0;

protected:
	virtual ~IImageFile() {}
};

#endif
