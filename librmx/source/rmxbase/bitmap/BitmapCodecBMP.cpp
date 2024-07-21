/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace rmx
{
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

	inline uint32 swapRedBlue(uint32 color)
	{
		return (color & 0xff00ff00) | ((color & 0x00ff0000) >> 16) | ((color & 0x000000ff) << 16);
	}

	#define RETURN(errcode) \
	{ \
		outResult.mError = errcode; \
		return (errcode == Bitmap::LoadResult::Error::OK); \
	}



	bool BitmapCodecBMP::canDecode(const String& format) const
	{
		return (format == "bmp");
	}

	bool BitmapCodecBMP::canEncode(const String& format) const
	{
		return (format == "bmp");
	}

	bool BitmapCodecBMP::decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult)
	{
		// Read header
		BmpHeader header;
		stream >> header;
		if (memcmp(header.signature, "BM", 2) != 0)
			RETURN(Bitmap::LoadResult::Error::INVALID_FILE);

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
		int palSize = 0;
		if (bitdepth == 1 || bitdepth == 4 || bitdepth == 8)
		{
			palSize = (header.numColors != 0) ? header.numColors : (1 << bitdepth);
		}

		// Read and convert palette
		uint32 palette[256];
		stream.read(palette, palSize * sizeof(uint32));
		for (int i = 0; i < palSize; ++i)
		{
			palette[i] = swapRedBlue(palette[i] | 0xff000000);
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
			uint32* dataPtr = &data[(height-y-1)*width];
			switch (bitdepth)
			{
				case 1:
					for (int x = 0; x < width; ++x)
						dataPtr[x] = palette[(buffer[x/8] >> (x%8)) & 0x01];
					break;

				case 4:
					for (int x = 0; x < width; ++x)
						dataPtr[x] = palette[(buffer[x/2] >> ((1-x%2)*4)) & 0x0f];
					break;

				case 8:
					for (int x = 0; x < width; ++x)
						dataPtr[x] = palette[buffer[x]];
					break;

				case 24:
					for (int x = 0; x < width; ++x)
						dataPtr[x] = ((uint32)buffer[x*3] << 16) | ((uint32)buffer[x*3+1] << 8) | ((uint32)buffer[x*3+2]) | 0xff000000;
					break;

				case 32:
					for (int x = 0; x < width; ++x)
						dataPtr[x] = swapRedBlue(*(uint32*)&buffer[x*4]);
					break;
			}
			buffer += stride;
		}

		// 32bit-BMP with or without alpha channel?
		if (bitdepth == 32)
		{
			bool noAlpha = true;
			const int size = width * height;
			for (int i = 0; i < size; ++i)
			{
				if (data[i] >= 0x01000000)
				{
					noAlpha = false;
					break;
				}
			}
			if (noAlpha)
			{
				for (int i = 0; i < size; ++i)
					data[i] |= 0xff000000;
			}
		}

		RETURN(Bitmap::LoadResult::Error::OK);
	}

	bool BitmapCodecBMP::encode(const Bitmap& bitmap, OutputStream& stream)
	{
		const int width = bitmap.getWidth();
		const int height = bitmap.getHeight();
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
			const uint32* src = bitmap.getPixelPointer(0, height-y-1);
			for (int x = 0; x < width; ++x)
				output[x] = swapRedBlue(src[x]);
			stream.write(output, width*4);
		}

		delete[] output;
		return true;
	}

	#undef RETURN
}
