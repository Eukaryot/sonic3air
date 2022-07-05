/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


#pragma pack(1)
struct IcoHeader
{
	uint16 idReserved;
	uint16 idType;
	uint16 idCount;
};

struct IconDirEntry
{
	uint8  width;
	uint8  height;
	uint8  colorCount;
	uint8  reserved;
	uint16 planes;
	uint16 bitCount;
	uint32 bytesInRes;
	uint32 imageOffset;
};
#pragma pack()

#define RETURN(errcode) \
{ \
	outResult.mError = errcode; \
	return (errcode == Bitmap::LoadResult::Error::OK); \
}



bool BitmapCodecICO::canDecode(const String& format) const
{
	return (format == "ico");
}

bool BitmapCodecICO::canEncode(const String& format) const
{
	return false;
}

bool BitmapCodecICO::decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult)
{
	MemInputStream mstream(stream);
	const uint8* buffer = mstream.getCursor();

	IcoHeader header;
	stream >> header;
	if (header.idReserved != 0 || header.idType != 1)
		RETURN(Bitmap::LoadResult::Error::INVALID_FILE);

	const int imageCount = header.idCount;

	const IconDirEntry* entries = (const IconDirEntry*)mstream.getCursor();
	mstream.skip(imageCount * sizeof(IconDirEntry));

	// Choose the best fitting one from the icons
	int optimal_wid = (bitmap.getWidth() > 0)  ? bitmap.getWidth()  : 32;
	int optimal_hgt = (bitmap.getHeight() > 0) ? bitmap.getHeight() : 32;
	int optimal_bpp = 32;
	int best_image = -1;
	uint32 best_diff = 0xffffffff;

	for (int i = 0; i < imageCount; ++i)
	{
		int bpp = entries[i].bitCount;
		if (bpp == 0)
			bpp = *(unsigned short*)&buffer[entries[i].imageOffset + 14];

		int dx = abs(entries[i].width - optimal_wid);
		int dy = abs(entries[i].height - optimal_hgt);
		int db = abs(bpp - optimal_bpp) * 1000;

		uint32 difference = dx*dx + dy*dy + db*db;
		if (difference < best_diff)
		{
			best_diff = difference;
			best_image = i;
		}
	}

	if (best_image == -1)
		RETURN(Bitmap::LoadResult::Error::INVALID_FILE);

	// Load icon
	const IconDirEntry& bestEntry = entries[best_image];
	unsigned int size = bestEntry.bytesInRes;
	unsigned int offset = bestEntry.imageOffset;

	uint8* mem = new uint8[size+14];
	memcpy(&mem[14], &buffer[offset], size);
	mem[0] = 'B';
	mem[1] = 'M';
	*(unsigned int*)&mem[2]  = 54 + *(unsigned int*)&mem[34];
	*(unsigned int*)&mem[6]  = 0;
	*(unsigned int*)&mem[10] = 54;
	*(unsigned int*)&mem[22] /= 2;		// File contains double image height for some reason
	int wid = *(unsigned int*)&mem[18];
	int hgt = *(unsigned int*)&mem[22];
	int bpp = *(unsigned short*)&mem[28];

	// Decode as BMP
	MemInputStream bmpstream(mem, size+14, true);
	BitmapCodecBMP codec;
	bool result = codec.decode(bitmap, bmpstream, outResult);
	if (!result)
		RETURN(Bitmap::LoadResult::Error::INVALID_FILE);

	// Add alpha channel
	if (bpp < 32)
	{
		int pal_size = (bpp <= 8) ? ((1 << bpp) * 4) : 0;
		int img_size = wid * hgt * bpp / 8;
		int mask_line = (wid + 31) / 32 * 4;
		uint8* bitmask = &mem[54 + pal_size + img_size];
		for (int y = 0; y < hgt; ++y)
		{
			uint32* src = bitmap.getPixelPointer(0, y);
			for (int x = 0; x < wid; ++x)
			{
				if ((bitmask[x/8 + (hgt-1-y)*mask_line] >> (7-x%8)) & 1)
					src[x] &= 0x00ffffff;
			}
		}
	}

	RETURN(Bitmap::LoadResult::Error::OK);
}

bool BitmapCodecICO::encode(const Bitmap& bitmap, OutputStream& stream)
{
	return false;
}
