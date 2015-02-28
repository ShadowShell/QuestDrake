////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013
// ------------------------------------------------------------------------
//  File name:   IZlibDecompressor.h
//  Created:     5/9/2013 by Axel Gneiting
//  Description: Provides the interface for the lz4 hc decompress wrapper
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ILZ4DECOMPRESSOR_H__
#define __ILZ4DECOMPRESSOR_H__

UNIQUE_IFACE struct ILZ4Decompressor
{
	protected:
		virtual ~ILZ4Decompressor()	{}; // use Release()

	public:
		virtual bool DecompressData(const char *pIn, char *pOut, const uint outputSize) const = 0;

		virtual void Release() = 0;
};

#endif // __ILZ4DECOMPRESSOR_H__