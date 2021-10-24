/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"

// For data decompression, either use zlib (which is faster) or alternatively the RmxDeflate class
#define USE_ZLIB


namespace
{
	uint32 readUint32LE(const uint8* pointer)
	{
		// Read as little endian
	#if defined(__arm__)
		// Do not access memory directly, but byte-wise to avoid "SIGBUS illegal alignment" issues (this happened on Android Release builds, but not in Debug for some reason)
		return ((uint32)pointer[0]) + ((uint32)pointer[1] << 8) + ((uint32)pointer[2] << 16) + ((uint32)pointer[3] << 24);
	#else
		return *(uint32*)pointer;
	#endif
	}

	uint32 readUint32BE(const uint8* pointer)
	{
		// Read as big endian
		return ((uint32)pointer[0] << 24) + ((uint32)pointer[1] << 16) + ((uint32)pointer[2] << 8) + ((uint32)pointer[3]);
	}
}


#define PNG_IHDR 0x49484452
#define PNG_IDAT 0x49444154
#define PNG_IEND 0x49454e44
#define PNG_PLTE 0x504c5445

const uint32 PNGSignature[2] = { 0x474e5089, 0x0a1a0a0d };

// PNG header
struct PNGHeader
{
	uint32 width;
	uint32 height;
	uint8  bitdepth;
	uint8  colortype;
	uint8  compression;
	uint8  filter;
	uint8  interlace;
};

#define RETURN(errcode) \
{ \
	bitmap.mError = errcode; \
	return (errcode == Bitmap::Error::OK); \
}



bool BitmapCodecPNG::canDecode(const String& format) const
{
	return (format == "png");
}

bool BitmapCodecPNG::canEncode(const String& format) const
{
	return (format == "png");
}

bool BitmapCodecPNG::decode(Bitmap& bitmap, InputStream& stream)
{
	// Load from PNG image data in memory
	MemInputStream mstream(stream);
	if (mstream.getRemaining() < 8)
		RETURN(Bitmap::Error::INVALID_FILE);

	const uint8* mem = mstream.getCursor();
	const uint8* end = mstream.getCursor() + mstream.getRemaining();
	if (memcmp(mem, PNGSignature, 8) != 0)
		RETURN(Bitmap::Error::INVALID_FILE);
	mem += 8;

	// Read header
	PNGHeader header;
	int width = 0;
	int height = 0;
	uint32 palette[0x100];
	int palette_size = 0;

	// Temporary buffer for the image data
	std::vector<uint8> content;
	content.reserve(mstream.getRemaining());

	// Read chunks
	bool finished = false;
	while (!finished)
	{
		// Length & chunk type
		if ((size_t)(end - mem) < 8)
			RETURN(Bitmap::Error::INVALID_FILE);

		const uint8* chunkStart = mem;
		const uint32 length = readUint32BE(mem);
		const uint32 type   = readUint32BE(mem + 4);
		mem += 8;
		if ((size_t)(end - mem) < length + 4)
			RETURN(Bitmap::Error::INVALID_FILE);

		switch (type)
		{
			// IHDR
			case PNG_IHDR:
			{
				if (length != 13)
					RETURN(Bitmap::Error::FILE_ERROR);
				memcpy(&header, mem, length);
				width = swapBytes32(header.width);
				height = swapBytes32(header.height);
				break;
			}

			// IEND
			case PNG_IEND:
			{
				finished = true;
				break;
			}

			// IDAT
			case PNG_IDAT:
			{
				const size_t position = content.size();
				content.resize(position + length);
				memcpy(&content[position], &mem[0], length);
				break;
			}

			// PLTE
			case PNG_PLTE:
			{
				palette_size = length / 3;
				for (int i = 0; i < palette_size; ++i)
					palette[i] = 0xff000000 + (readUint32LE(mem + i * 3) & 0xffffff);
				for (int i = palette_size; i < 0x100; ++i)
					palette[i] = 0x00000000;
				break;
			}
		}

		// CRC
		const uint32 crc = rmx::getCRC32(chunkStart+4, length+4);
		mem += length;
		if (readUint32BE(mem) != crc)
			RETURN(Bitmap::Error::INVALID_FILE);
		mem += 4;
	}

	// Check for empty image data
	if (content.empty())
		RETURN(Bitmap::Error::INVALID_FILE);

	// This function supports only 8-bit depth, nothing else
	if (header.bitdepth != 8)
		RETURN(Bitmap::Error::UNSUPPORTED);

	int bpp = 0;
	switch (header.colortype)
	{
		case 0:  bpp = 1;  break;
		case 2:  bpp = 3;  break;
		case 3:  bpp = 1;  break;
		case 4:  bpp = 2;  break;
		case 6:  bpp = 4;  break;
		default:
			RETURN(Bitmap::Error::INVALID_FILE);
	}
	const int bytesPerLine = width * bpp;

	// Decompress image data
#if !defined(USE_ZLIB)

	int outsize = 0;
	if ((content[0] & 15) != 8)		// Check zlib header for deflate algorithm
		RETURN(Bitmap::Error::INVALID_FILE);

	uint8* output = Deflate::decode(outsize, &content[2], (int)content.size() - 2);		// Skip the zlib header
	if (nullptr == output)
		RETURN(Bitmap::Error::INVALID_FILE);

#else

	std::vector<uint8> outputMemory;
	outputMemory.reserve(bytesPerLine * height + 0x4000);	// 0x4000 is the internal chunk size used by "ZlibDeflate::decode"
	if (!ZlibDeflate::decode(outputMemory, &content[0], content.size()))
		RETURN(Bitmap::Error::INVALID_FILE);

	uint8* output = &outputMemory[0];
	const int outsize = (int)outputMemory.size();

#endif

	// Create output bitmap
	bitmap.create(width, height);
	uint32* data = bitmap.mData;

	// Remove the filter
	int outpos = 0;
	uint8* currentLineBuffer = nullptr;		// Pointer to current line
	for (int line = 0; line < height; ++line)
	{
		uint8* previousLineBuffer = currentLineBuffer;
		if (line == 0)
		{
			// Misuse the last line (or parts of it) of the output as a temporary buffer storing zeroes
			previousLineBuffer = (uint8*)&data[width * (height - 1)];
			memset(previousLineBuffer, 0, width * bpp);
		}

		const uint8 filter = output[outpos];
		currentLineBuffer = &output[outpos+1];
		outpos += bytesPerLine + 1;
		if (outpos > outsize)
			RETURN(Bitmap::Error::INVALID_FILE);

		if (filter != 0)
		{
			uint8* buf = currentLineBuffer;
			uint8* buf0 = previousLineBuffer;
			const int leftPixelOffset = -bpp;
			switch (filter)
			{
				// "Sub" filter
				case 1:
				{
					buf += bpp;
					for (int i = bpp; i < bytesPerLine; ++i)
					{
						*buf += buf[leftPixelOffset];
						++buf;
					}
					break;
				}

				// "Up" filter
				case 2:
				{
					for (int i = 0; i < bytesPerLine; ++i)
					{
						*buf += *buf0;
						++buf;
						++buf0;
					}
					break;
				}

				// "Average" filter
				case 3:
				{
					for (int i = 0; i < bpp; ++i)
					{
						*buf += *buf0 / 2;
						++buf;
						++buf0;
					}
					for (int i = bpp; i < bytesPerLine; ++i)
					{
						const uint8 left = buf[leftPixelOffset];
						const uint8 up = *buf0;
						*buf += (left + up) / 2;
						++buf;
						++buf0;
					}
					break;
				}

				// "Paeth" filter
				case 4:
				{
					for (int i = 0; i < bpp; ++i)
					{
						*buf += *buf0;
						++buf;
						++buf0;
					}
					for (int i = bpp; i < bytesPerLine; ++i)
					{
						const uint8 left = buf[leftPixelOffset];
						const uint8 up = *buf0;
						const uint8 upLeft = buf0[leftPixelOffset];
						const int paeth = left + up - upLeft;
						const int d1 = abs(paeth - left);
						const int d2 = abs(paeth - up);
						const int d3 = abs(paeth - upLeft);
						if ((d1 <= d2) && (d1 <= d3))
							*buf += left;
						else if (d2 <= d3)
							*buf += up;
						else
							*buf += upLeft;
						++buf;
						++buf0;
					}
					break;
				}
			}
		}

		// Convert to 32-bit
		{
			const uint8* src = currentLineBuffer;
			uint32* dst = &data[line*width];
			switch (header.colortype)
			{
				// 8-bit grayscale
				case 0:
					for (int i = 0; i < width; ++i)
						dst[i] = 0xff000000 + (0x010101 * src[i]);
					break;

				// 24-bit RGB
				case 2:
					for (int i = 0; i < width; ++i)
						dst[i] = 0xff000000 + (*(uint32*)(&src[i*3]) & 0x00ffffff);
					break;

				// Palette image
				case 3:
					for (int i = 0; i < width; ++i)
						dst[i] = palette[src[i]];
					break;

				// 16-bit gray + alpha
				case 4:
					for (int i = 0; i < width; ++i)
						dst[i] = (0x010101 * src[i*2]) + (src[i*2+1] << 24);
					break;

				// 32-bit RGB + alpha
				case 6:
					memcpy(dst, src, width*sizeof(uint32));
					break;
			}
		}
	}

#if !defined(USE_ZLIB)
	delete[] output;
#endif
	RETURN(Bitmap::Error::OK);
}

bool BitmapCodecPNG::encode(const Bitmap& bitmap, OutputStream& stream)
{
	// Save image data to memory in PNG format
	if (nullptr == bitmap.mData)
		return false;

	int width = bitmap.mWidth;
	int height = bitmap.mHeight;

	// Setup PNG header
	PNGHeader header;
	header.width = swapBytes32(bitmap.mWidth);
	header.height = swapBytes32(bitmap.mHeight);
	header.bitdepth = 8;
	header.colortype = 6;
	header.compression = 0;
	header.filter = 0;
	header.interlace = 0;

	// Pack image data
	uint8* tmpdata = new uint8[(width*4+1)*height];
	for (int line = 0; line < height; ++line)
	{
		tmpdata[line*(width*4+1)] = 0;
		memcpy(&tmpdata[line*(width*4+1)+1], &bitmap.mData[line*width], width*4);
	}
	int outsize = 0;
	uint8* output = Deflate::encode(outsize, tmpdata, (width*4+1)*height);			// TODO: Optionally use ZlibDeflate here as well
	const uint32 adler = swapBytes32(rmx::getAdler32(tmpdata, (width*4+1)*height));
	delete[] tmpdata;

	// Write PNG data
	const int bufsize = 3*12 + 13 + outsize+6;
	uint8* buffer = new uint8[bufsize];
	uint8* mem = buffer;

	// Write chunks
	for (int chunknum = 0; chunknum < 3; ++chunknum)
	{
		uint8* chunkStart = mem;
		mem += 8;
		uint32 type = 0;
		uint32 length = 0;
		if (chunknum == 0)
		{
			type = PNG_IHDR;
			length = 13;
			memcpy(mem, &header, length);
		}
		else if (chunknum == 1)
		{
			type = PNG_IDAT;
			length = outsize + 6;
			mem[0] = 0x78;		// zlib header
			mem[1] = 0xda;		// zlib header
			memcpy(&mem[2], output, outsize);
			*(uint32*)&chunkStart[length+4] = adler;
		}
		else
		{
			type = PNG_IEND;
		}

		*(uint32*)&chunkStart[0] = swapBytes32(length);
		*(uint32*)&chunkStart[4] = swapBytes32(type);
		const uint32 crc = rmx::getCRC32(chunkStart+4, length+4);
		*(uint32*)&chunkStart[length+8] = swapBytes32(crc);
		mem += length + 4;
	}
	delete[] output;

	stream.write(PNGSignature, 8);
	stream.write(buffer, bufsize);
	delete[] buffer;

	return true;
}
