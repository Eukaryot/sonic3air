/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"


namespace
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
}


PaletteBitmap::PaletteBitmap(const PaletteBitmap& toCopy)
{
	copy(toCopy, Recti(0, 0, toCopy.mWidth, toCopy.mHeight));
}

PaletteBitmap::~PaletteBitmap()
{
	delete[] mData;
}

void PaletteBitmap::create(uint32 width, uint32 height)
{
	if (width != mWidth || height != mHeight)
	{
		delete[] mData;
		mData = new uint8[width * height];
		mWidth = width;
		mHeight = height;
	}
}

void PaletteBitmap::create(uint32 width, uint32 height, uint8 fillValue)
{
	create(width, height);
	clear(fillValue);
}

void PaletteBitmap::copy(const PaletteBitmap& source)
{
	if (nullptr == source.mData)
	{
		delete[] mData;
		mData = nullptr;
		return;
	}

	create(source.mWidth, source.mHeight);
	memcpy(mData, source.mData, mWidth * mHeight);
}

void PaletteBitmap::copy(const PaletteBitmap& source, const Recti& rect)
{
	if (nullptr == source.mData)
	{
		delete[] mData;
		mData = nullptr;
		return;
	}

	int px = rect.x;
	int py = rect.y;
	int sx = rect.width;
	int sy = rect.height;
	if (px < 0)  { sx += px;  px = 0; }
	if (py < 0)  { sy += py;  py = 0; }
	if (px + sx > (int)source.mWidth)   sx = source.mWidth - px;
	if (py + sy > (int)source.mHeight)  sy = source.mHeight - py;
	if (sx <= 0 || sy <= 0)
		return;

	create(sx, sy);
	memcpyRect(mData, mWidth, &source.mData[px+py*source.mWidth], source.mWidth, sx, sy);
}

void PaletteBitmap::copyRect(const PaletteBitmap& source, const Recti& rect, const Vec2i& destination)
{
	if (nullptr == source.mData || nullptr == mData)
		return;

	int px = rect.x;
	int py = rect.y;
	int sx = rect.width;
	int sy = rect.height;
	if (px < 0) { sx += px;  px = 0; }
	if (py < 0) { sy += py;  py = 0; }
	if (px + sx > (int)source.mWidth)   sx = source.mWidth - px;
	if (py + sy > (int)source.mHeight)  sy = source.mHeight - py;
	if (sx <= 0 || sy <= 0)
		return;

	// TODO: Add checks for destination
	memcpyRect(&mData[destination.x + destination.y * mWidth], mWidth, &source.mData[px + py * source.mWidth], source.mWidth, sx, sy);
}

void PaletteBitmap::swap(PaletteBitmap& other)
{
	std::swap(mData, other.mData);
	std::swap(mWidth, other.mWidth);
	std::swap(mHeight, other.mHeight);
}

void PaletteBitmap::clear(uint8 color)
{
	memset(mData, color, getPixelCount());
}

void PaletteBitmap::shiftAllIndices(int8 indexShift)
{
	if (indexShift == 0)
		return;

	const int pixels = getPixelCount();
	for (int i = 0; i < pixels; ++i)
	{
		mData[i] += indexShift;
	}
}

void PaletteBitmap::overwriteUnusedPaletteEntries(Color* palette)
{
	bool used[0x100] = { false };
	const int pixels = getPixelCount();
	for (int i = 0; i < pixels; ++i)
	{
		used[mData[i]] = true;
	}
	for (int i = 0; i < 0x100; ++i)
	{
		if (!used[i])
			palette[i] = Color::fromABGR32(0x00660145);
	}
}

bool PaletteBitmap::loadBMP(const std::vector<uint8>& bmpContent, Color* outPalette)
{
	VectorBinarySerializer serializer(true, bmpContent);

	// Read header
	BmpHeader header;
	serializer.read(&header, sizeof(header));
	if (memcmp(header.signature, "BM", 2) != 0)
		return false;

	// Size
	const int width = header.width;
	const int height = header.height;
	const int bitdepth = header.bpp;
	const int stride = (width * bitdepth + 31) / 32 * 4;

	// Skip unrecognized parts of the header
	if (header.dibHeaderSize > 0x28)
	{
		serializer.skip(header.dibHeaderSize - 0x28);
	}

	// Load palette
	int pal_size = 0;
	if (bitdepth == 1 || bitdepth == 4 || bitdepth == 8)
	{
		pal_size = (header.numColors != 0) ? header.numColors : (1 << bitdepth);
	}

	// Can't load non-palette bitmaps into a PaletteBitmap instance!
	if (pal_size == 0)
		return false;

	// Read palette
	uint32 palette[256];
	serializer.read(palette, pal_size * 4);
	if (nullptr != outPalette)
	{
		for (int i = 0; i < pal_size; ++i)
		{
			Color color = Color::fromABGR32(palette[i]);
			outPalette[i].set(color.b, color.g, color.r, 1.0f);
		}
	}

	// Skip unrecognized parts of the header
	if (header.headerSize > serializer.getReadPosition())
	{
		serializer.skip(header.headerSize - serializer.getReadPosition());
	}

	// Create data buffer
	create(width, height);
	const uint8* buffer = &bmpContent[serializer.getReadPosition()];

	// Load image data
	for (int y = 0; y < height; ++y)
	{
		uint8* data_ptr = &mData[(height-y-1)*width];
		for (int x = 0; x < width; ++x)
		{
			switch (bitdepth)
			{
				case 1:
					data_ptr[x] = (buffer[x/8] >> (x & 0x07)) & 0x01;
					break;
				case 4:
					data_ptr[x] = (buffer[x/2] >> (4 - (x & 0x01) * 4)) & 0x0f;
					break;
				case 8:
					data_ptr[x] = buffer[x];
					break;
			}
		}
		buffer += stride;
	}

	return true;
}

bool PaletteBitmap::saveBMP(std::vector<uint8>& bmpContent, const Color* palette)
{
	VectorBinarySerializer serializer(false, bmpContent);
	const uint32 stride = (mWidth * 8 + 31) / 32 * 4;

	BmpHeader header;
	header.signature[0] = 'B';
	header.signature[1] = 'M';
	header.fileSize = sizeof(BmpHeader) + 256 * 4 + stride * mHeight;
	header.creator1 = 0;
	header.creator2 = 0;
	header.headerSize = sizeof(BmpHeader) + 256 * 4;
	header.dibHeaderSize = 40;
	header.width = mWidth;
	header.height = mHeight;
	header.numPlanes = 1;
	header.bpp = 8;
	header.compression = 0;
	header.dataSize = stride * mHeight;
	header.resolutionX = 3828;
	header.resolutionY = 3828;
	header.numColors = 256;
	header.importantColors = 256;
	serializer.write(&header, sizeof(BmpHeader));

	for (int i = 0; i < 256; ++i)
	{
		const Color color(palette[i].b, palette[i].g, palette[i].r, 1.0f);
		serializer.write(color.getABGR32());
	}

	for (uint32 line = 0; line < mHeight; ++line)
	{
		serializer.write(&mData[(mHeight - 1 - line) * mWidth], mWidth);
		if (mWidth != stride)
		{
			static const uint8 zeroes[] = { 0, 0, 0, 0 };
			serializer.write(zeroes, stride - mWidth);
		}
	}
	return true;
}

void PaletteBitmap::memcpyRect(uint8* dst, int dwid, uint8* src, int swid, int wid, int hgt)
{
	for (int y = 0; y < hgt; ++y)
	{
		memcpy(&dst[y*dwid], &src[y*swid], wid);
	}
}
