/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


#pragma pack(1)
struct BmpHeader
{
	uint8  signature[2];
	uint32 fileSize;
	uint16 creator1;
	uint16 creator2;
	uint32 headerSize;
	uint32 dibHeaderSize;
	int32  width;
	int32  height;
	uint16 numPlanes;
	uint16 bpp;
	uint32 compression;
	uint32 dataSize;
	int32  resolutionX;
	int32  resolutionY;
	uint32 numColors;
	uint32 importantColors;
};
#pragma pack()

inline uint32 RGBA_to_BGRA(uint32 color)
{
	return (color & 0xff00ff00) | ((color & 0x00ff0000) >> 16) | ((color & 0x000000ff) << 16);
}

#define RETURN(errcode) \
{ \
	bitmap.mError = errcode; \
	return (errcode == Bitmap::Error::OK); \
}



bool BitmapCodecBMP::canDecode(const String& format) const
{
	return (format == "bmp");
}

bool BitmapCodecBMP::canEncode(const String& format) const
{
	return (format == "bmp");
}

bool BitmapCodecBMP::decode(Bitmap& bitmap, InputStream& stream)
{
	// Read header
	BmpHeader header;
	stream >> header;
	if (memcmp(header.signature, "BM", 2) != 0)
		RETURN(Bitmap::Error::INVALID_FILE);

	// Size
	const int width = header.width;
	const int height = header.height;
	const int bitdepth = header.bpp;
	const int stride = (width * bitdepth + 31) / 32 * 4;

	// Skip unrecognized parts of the header
	if (header.dibHeaderSize > 0x28)
	{
		stream.skip(header.dibHeaderSize - 0x28);
	}

	// Load palette
	int pal_size = 0;
	if (bitdepth == 1 || bitdepth == 4 || bitdepth == 8)
	{
		pal_size = (header.numColors != 0) ? header.numColors : (1 << bitdepth);
	}

	// Read palette
	uint32 palette[256];
	for (int i = 0; i < pal_size; ++i)
	{
		stream >> palette[i];
		palette[i] = RGBA_to_BGRA(palette[i] | 0xff000000);
	}

	// Skip unrecognized parts of the header
	if (header.headerSize > stream.getPosition())
	{
		stream.skip(header.headerSize - stream.getPosition());
	}

	// Create data buffer
	bitmap.create(width, height);
	uint32* data = bitmap.getData();
	MemInputStream mstream(stream);
	const uint8* buffer = mstream.getCursor();

	// Load image data
	for (int y = 0; y < height; ++y)
	{
		uint32* data_ptr = &data[(height-y-1)*width];
		for (int x = 0; x < width; ++x)
		{
			switch (bitdepth)
			{
				case 1:
					data_ptr[x] = palette[(buffer[x/8] >> (x%8)) & 1];
					break;
				case 4:
					data_ptr[x] = palette[(buffer[x/2] >> ((1-x%2)*4)) & 15];
					break;
				case 8:
					data_ptr[x] = palette[buffer[x]];
					break;
				case 24:
					data_ptr[x] = ((uint32)buffer[x*3]   << 16)
								+ ((uint32)buffer[x*3+1] <<  8)
								+ ((uint32)buffer[x*3+2]      ) + 0xff000000;
					break;
				case 32:
					data_ptr[x] = RGBA_to_BGRA(*(uint32*)&buffer[x*4]);
					break;
			}
		}
		buffer += stride;
	}

	// 32bit-BMP with or without alpha channel?
	if (bitdepth == 32)
	{
		const int size = width * height;

		bool no_alpha = true;
		for (int i = 0; i < size; ++i)
			if (data[i] >= 0x01000000)
			{
				no_alpha = false;
				break;
			}

		if (no_alpha)
		{
			for (int i = 0; i < size; ++i)
				data[i] |= 0xff000000;
		}
	}

	RETURN(Bitmap::Error::OK);
}

bool BitmapCodecBMP::encode(const Bitmap& bitmap, OutputStream& stream)
{
	const int width = bitmap.mWidth;
	const int height = bitmap.mHeight;
	const int dataSize = width * height * 4;
	const int headerSize = sizeof(BmpHeader);

	// Header
	BmpHeader header;
	memset(&header, 0, sizeof(BmpHeader));
	memcpy(header.signature, "BM", 2);
	header.fileSize = headerSize + dataSize;
	header.headerSize = headerSize;
	header.dibHeaderSize = 40;
	header.width = width;
	header.height = height;
	header.numPlanes = 1;
	header.bpp = 32;
	header.dataSize = dataSize;
	header.resolutionX = 0xb40;
	header.resolutionY = 0xb40;
	stream << header;

	uint32* output = new uint32[width];

	for (int y = 0; y < height; ++y)
	{
		uint32* src = &bitmap.mData[(height-y-1)*width];
		for (int x = 0; x < width; ++x)
			output[x] = RGBA_to_BGRA(src[x]);
		stream.write(output, width*4);
	}

	delete[] output;
	RETURN(Bitmap::Error::OK);
}

#undef RETURN
