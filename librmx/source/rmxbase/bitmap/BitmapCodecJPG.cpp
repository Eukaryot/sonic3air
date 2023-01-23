/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"

// TODO: There's a lot missing here, including progressive JPEG support


class BitmapJPG
{
private:
	struct ChannelInfo
	{
		int sample_x;
		int sample_y;
		int samples;
		int QT_number;
		int AC_number;
		int DC_number;
		short prev_DC;
	};

	struct QuantTable
	{
		unsigned char data[64];
	};

	struct HuffmanTable
	{
		char length[16];
		uint16 min_code[16];
		uint16 max_code[16];
		char value[256][256];
	};

private:
	typedef const uint8* Constuint8Ptr;

	static bool initialized;
	static float cos_lookup[8][8];
	static unsigned char zigzag_lookup[64];

	unsigned int* image;
	int width, height;
	int channels;
	bool progressive;
	bool found_SOF;
	bool found_SOS;
	int MCU_size_x, MCU_size_y;
	int MCU_count_x, MCU_count_y;
	short* coeff_buffer;

	Constuint8Ptr* mem_ptr = nullptr;
	uint32 bitbuffer = 0;
	uint32 unused_bits = 0;

	HuffmanTable htableDC[4];
	HuffmanTable htableAC[4];
	QuantTable qtable[4];
	ChannelInfo channel[3];

private:
	// Part 1: Read JPEG data
	inline void refillBitBuffer();
	void decodeHuffmanDC(short* output, HuffmanTable* htab, short& prev_DC);
	void decodeHuffmanAC(short* output, HuffmanTable* htab, int count);
	Bitmap::LoadResult::Error readDQT(const uint8* mem, int size);
	Bitmap::LoadResult::Error readDHT(const uint8* mem, int size);
	Bitmap::LoadResult::Error readSOF(const uint8* mem, int size);
	Bitmap::LoadResult::Error readSOS(const uint8* mem, int& size);
	Bitmap::LoadResult::Error readJPEG(const uint8* buffer, size_t bufsize);

	// Part 2: Decode into bitmap
	void applyIDCT(short* input, unsigned char* output);
	void writeImageData(uint8* data, int ch_number, int px, int py, int factor_x, int factor_y);
	void convertColors();
	void buildBitmap(Bitmap& bitmap);

public:
	BitmapJPG();
	bool decode(Bitmap& bitmap, const uint8* buffer, size_t bufsize, Bitmap::LoadResult& outResult);
};


// Static data
bool BitmapJPG::initialized = false;
float BitmapJPG::cos_lookup[8][8];
unsigned char BitmapJPG::zigzag_lookup[64] = {  0,  1,  5,  6, 14, 15, 27, 28,
												2,  4,  7, 13, 16, 26, 29, 42,
												3,  8, 12, 17, 25, 30, 41, 43,
												9, 11, 18, 24, 31, 40, 44, 53,
											   10, 19, 23, 32, 39, 45, 52, 54,
											   20, 22, 33, 38, 46, 51, 55, 60,
											   21, 34, 37, 47, 50, 56, 59, 61,
											   35, 36, 48, 49, 57, 58, 62, 63 };

// Chunk types
#define JPG_SOI  0xd8
#define JPG_EOI  0xd9
#define JPG_APP0 0xe0
#define JPG_APP1 0xe1
#define JPG_SOF0 0xc0
#define JPG_SOF2 0xc2
#define JPG_DQT  0xdb
#define JPG_DHT  0xc4
#define JPG_SOS  0xda
#define JPG_DRI  0xdd
#define JPG_COM  0xfe

// Lookups
#define IDCT1 2841		// 2048 * sqrt(2) * cos(1*PI/16)
#define IDCT2 2676		// 2048 * sqrt(2) * cos(2*PI/16)
#define IDCT3 2408		// 2048 * sqrt(2) * cos(3*PI/16)
#define IDCT5 1609		// 2048 * sqrt(2) * cos(5*PI/16)
#define IDCT6 1108		// 2048 * sqrt(2) * cos(6*PI/16)
#define IDCT7  565		// 2048 * sqrt(2) * cos(7*PI/16)

// Macros
#define GET_SHORT(x)  ((*(x) << 8) + *(x+1))



BitmapJPG::BitmapJPG()
{
	if (!initialized)
	{
		for (int i = 0; i < 8; ++i)
		{
			for (int j = 0; j < 8; ++j)
			{
				cos_lookup[i][j] = cos(float(2*i+1) * j * 0.196349540849f);		// This constant is PI / 16
			}
		}
		initialized = true;
	}
}

void BitmapJPG::refillBitBuffer()
{
	while (unused_bits >= 8)
	{
		unused_bits -= 8;
		unsigned int next_uint8 = (*mem_ptr)[0];
		++(*mem_ptr);
		if (next_uint8 == 0xff)
			++(*mem_ptr);			// Skip next byte (usually 0x00)
		bitbuffer |= (next_uint8 << unused_bits);
	}
}

void BitmapJPG::decodeHuffmanDC(short* output, HuffmanTable* htab, short& prev_DC)
{
	uint16* min_c = htab->min_code;
	uint16* max_c = htab->max_code;
	uint16 look = bitbuffer >> 16;
	uint16 code;
	int k;
	for (k = 0; k < 16; ++k)
	{
		code = look >> (15-k);
		if (code >= min_c[k] && code <= max_c[k])
			break;
	}

	bitbuffer <<= (k+1);
	unused_bits += (k+1);
	uint8 length = htab->value[k][code - min_c[k]];
	if (length == 0)
	{
		output[0] = prev_DC;
	}
	else
	{
		int look = bitbuffer >> (32-length);
		if ((bitbuffer >> 31) == 0)
			look -= ((1 << length) - 1);
		prev_DC += look;
		output[0] = prev_DC;
		bitbuffer <<= length;
		unused_bits += length;
	}
	refillBitBuffer();
}

void BitmapJPG::decodeHuffmanAC(short* output, HuffmanTable* htab, int count)
{
	uint16* min_c = htab->min_code;
	uint16* max_c = htab->max_code;
	int num = 0;
	while (num < count)
	{
		uint16 look = bitbuffer >> 16;
		uint16 code;
		int k;
		for (k = 0; k <= 16; ++k)
		{
			code = look >> (15-k);
			if (code >= min_c[k] && code <= max_c[k])
				break;
		}
		if (k == 16)		// Unknown code (acteally an error)
		{
			++num;
			break;
		}

		bitbuffer <<= (k+1);
		unused_bits += (k+1);
		unsigned char value = htab->value[k][code - min_c[k]];
		if (value == 0)
			count = 0;				// Found EOB
		else if (value == 0xf0)
			num += 16;				// Skip 16 zeroes
		else
		{
			unsigned char length = value & 0x0f;
			unsigned char zero_count = value >> 4;
			num += zero_count;						// Skip leading zeroes
			output[num] = bitbuffer >> (32-length);
			if ((bitbuffer >> 31) == 0)				// Uppermost bit of vector[num]
				output[num] -= ((1 << length) - 1);
			bitbuffer <<= length;
			unused_bits += length;
			++num;
		}
		refillBitBuffer();
	}
}

Bitmap::LoadResult::Error BitmapJPG::readDQT(const uint8* mem, int size)
{
	// DQT: Define Quantization Table
	int pos = 0;
	while (pos < size)
	{
		int num = (mem[pos] & 0x0f);
		if (num >= 4)
			return Bitmap::LoadResult::Error::INVALID_FILE;
		if ((mem[pos] & 0xf0) != 0)
			return Bitmap::LoadResult::Error::UNSUPPORTED;		// TODO: Support 16-bit QT
		memcpy(qtable[num].data, mem+pos+1, 64);
		pos += 65;
	}
	return Bitmap::LoadResult::Error::OK;
}

Bitmap::LoadResult::Error BitmapJPG::readDHT(const uint8* mem, int size)
{
	// DHT: Define Huffman Table
	int pos = 0;
	while (pos < size)
	{
		int num = (mem[pos] & 0x0f);
		if (num >= 4)
			return Bitmap::LoadResult::Error::INVALID_FILE;
		if ((mem[pos] & 0xe0) != 0)
			return Bitmap::LoadResult::Error::INVALID_FILE;
		bool isAC = ((mem[pos] & 0x10) != 0);
		++pos;

		HuffmanTable* htab = isAC ? &htableAC[num] : &htableDC[num];
		memcpy(htab->length, mem+pos, 16);
		pos += 16;
		for (int k = 0; k < 16; ++k)
		{
			memcpy(htab->value[k], mem+pos, htab->length[k]);
			pos += htab->length[k];
		}

		// min/max-Code-Listen aufbauen
		int code = 0;
		for (int k = 0; k < 16; ++k)
		{
			htab->min_code[k] = (uint16)code;
			code += htab->length[k];
			htab->max_code[k] = (uint16)(code-1);
			code *= 2;
			if (htab->length[k] == 0)
			{
				htab->min_code[k] = 0xffff;
				htab->max_code[k] = 0;
			}
		}
	}
	return Bitmap::LoadResult::Error::OK;
}

Bitmap::LoadResult::Error BitmapJPG::readSOF(const uint8* mem, int size)
{
	// SOF: Start Of Frame
	if (mem[0] != 8)
		return Bitmap::LoadResult::Error::INVALID_FILE;				// TODO: Support for higher precision
	height = GET_SHORT(mem+1);
	width  = GET_SHORT(mem+3);
	channels = mem[5];
	if (width <= 0 || height <= 0)
		return Bitmap::LoadResult::Error::INVALID_FILE;
	if (channels != 1 && channels != 3)
		return Bitmap::LoadResult::Error::UNSUPPORTED;				// TODO: Support for CMYK

	// Channel infos
	int pos = 6;
	for (int i = 0; i < channels; ++i)
	{
		char id = mem[pos];
		if (id == 0 || id > 3)
			return Bitmap::LoadResult::Error::UNSUPPORTED;			// TODO: Support for other formats
		int c = id-1;
		channel[c].sample_x = (mem[pos+1] >> 4);
		channel[c].sample_y = (mem[pos+1] & 0x0f);
		channel[c].QT_number = mem[pos+2];
		channel[c].samples = channel[c].sample_x * channel[c].sample_y;
		channel[c].prev_DC = 0;
		pos += 3;
	}

	// Create coefficients buffer
	MCU_size_x = 1;
	MCU_size_y = 1;
	int samples_total = 0;
	for (int c = 0; c < channels; ++c)
	{
		if (MCU_size_x < channel[c].sample_x)
			MCU_size_x = channel[c].sample_x;
		if (MCU_size_y < channel[c].sample_y)
			MCU_size_y = channel[c].sample_y;
		samples_total += channel[c].samples;
	}
	MCU_size_x *= 8;
	MCU_size_y *= 8;
	MCU_count_x = (width - 1) / MCU_size_x + 1;
	MCU_count_y = (height - 1) / MCU_size_y + 1;
	int coeff_size = samples_total * MCU_count_x * MCU_count_y * 64;
	coeff_buffer = new short[coeff_size];
	memset(coeff_buffer, 0, coeff_size * sizeof(short));

	found_SOF = true;
	return Bitmap::LoadResult::Error::OK;
}

Bitmap::LoadResult::Error BitmapJPG::readSOS(const uint8* mem, int& size)
{
	// SOS: Start Of Scan
	if (!found_SOF)
		return Bitmap::LoadResult::Error::INVALID_FILE;
	int count = mem[0];
	if (!progressive && count != channels)
		return Bitmap::LoadResult::Error::INVALID_FILE;
	int pos = 1;
	for (int i = 0; i < count; ++i)
	{
		char id = mem[pos];
		if (id == 0 || id > 3)
			return Bitmap::LoadResult::Error::UNSUPPORTED;		// TODO: Support for other formats
		int c = id-1;
		channel[c].DC_number = (mem[pos+1] >> 4);
		channel[c].AC_number = (mem[pos+1] & 0x0f);
		pos += 2;
	}
	pos += 3;
	found_SOS = true;

	// Read Huffman-encoded bitstream
	const uint8* mem0 = mem;
	mem += pos;
	mem_ptr = &mem;
	bitbuffer = 0;
	unused_bits = 32;
	refillBitBuffer();
	short* coeff_ptr = coeff_buffer;
	int MCU_count = MCU_count_x * MCU_count_y;
	if (progressive)
	{
		// Progressive mode
		int coeff_first = mem0[pos-3];
		int coeff_last = mem0[pos-2];
		if (coeff_first == 0)
		{
			for (int mcu = 0; mcu < MCU_count; ++mcu)
				for (int c = 0; c < channels; ++c)
					for (int s = 0; s < channel[c].samples; ++s)
					{
						decodeHuffmanDC(coeff_ptr, &htableDC[channel[c].DC_number], channel[c].prev_DC);
						coeff_ptr[0] *= 2;		// !!!
						coeff_ptr += 64;
					}
		}
		else
		{
			if (count != 1)			// Assume that count is always 1...
				return Bitmap::LoadResult::Error::INVALID_FILE;
			int c = mem0[pos-5]-1;

			for (int mcu = 0; mcu < MCU_count; ++mcu)
				for (int s = 0; s < channel[c].samples; ++s)
				{
					decodeHuffmanAC(coeff_ptr + coeff_first, &htableAC[channel[c].AC_number], coeff_last - coeff_first + 1);
					coeff_ptr += 64;
				}

/*
			short* buf = new short[channel[c].samples * MCU_count];
			for (int i = coeff_first; i <= coeff_last; ++i)
			{
				decodeHuffmanAC(buf, &htableDC[channel[c].AC_number], channel[c].samples * MCU_count);
				for (int mcu = 0; mcu < MCU_count; ++mcu)
					for (int s = 0; s < channel[c].samples; ++s)
						coeff_ptr[i+(mcu*channel[c].samples+s)*64] = buf[mcu*channel[c].samples+s];
			}
			delete[] buf;
*/
		}
	}
	else
	{
		// Normal mode
		for (int mcu = 0; mcu < MCU_count; ++mcu)
			for (int c = 0; c < channels; ++c)
				for (int s = 0; s < channel[c].samples; ++s)
				{
					decodeHuffmanDC(coeff_ptr,   &htableDC[channel[c].DC_number], channel[c].prev_DC);
					decodeHuffmanAC(coeff_ptr+1, &htableAC[channel[c].AC_number], 63);
					coeff_ptr += 64;
				}
	}
	size = (int)(mem - mem0) - (32 - unused_bits) / 8 - 1;
	return Bitmap::LoadResult::Error::OK;
}

Bitmap::LoadResult::Error BitmapJPG::readJPEG(const uint8* buffer, size_t bufsize)
{
	// Read JPEG data stream
	if (buffer[0] != 0xff || buffer[1] != JPG_SOI)
		return Bitmap::LoadResult::Error::INVALID_FILE;

	width = 0;
	height = 0;
	channels = 0;
	progressive = false;
	found_SOF = false;
	found_SOS = false;

	int pos = 2;
	while (pos < (int)bufsize)
	{
		if (buffer[pos] != 0xff)
			return Bitmap::LoadResult::Error::INVALID_FILE;
		while (buffer[pos] == 0xff)
			++pos;
		int type = buffer[pos];
		int size = GET_SHORT(buffer+pos+1) - 2;
		pos += 3;
		const uint8* mem = &buffer[pos];

		Bitmap::LoadResult::Error result = Bitmap::LoadResult::Error::OK;
		switch (type)
		{
			// APP0: Header with signature & version number
			case JPG_APP0:
				if (pos != 6)
					return Bitmap::LoadResult::Error::INVALID_FILE;
				if (strcmp((char*)mem, "JFIF") != 0)
					return Bitmap::LoadResult::Error::INVALID_FILE;
				if (mem[5] != 1 || mem[6] > 2)
					return Bitmap::LoadResult::Error::INVALID_FILE;
				break;

			// EXPERIMENTAL (for my digicam images)
			case JPG_APP1:
				if (pos != 6)
					return Bitmap::LoadResult::Error::INVALID_FILE;
				break;

			// DRI: Define Restart Interval
			case JPG_DRI:
				break;

			// DQT: Define Quantization Table
			case JPG_DQT:
				result = readDQT(mem, size);
				break;

			// DHT: Define Huffman Table
			case JPG_DHT:
				result = readDHT(mem, size);
				break;

			// SOF: Start Of Frame
			case JPG_SOF0:
			case JPG_SOF2:
				progressive = (type == JPG_SOF2);
				result = readSOF(mem, size);
				break;

			// SOS: Start Of Scan
			case JPG_SOS:
				result = readSOS(mem, size);
//				if (size==4088)
				return Bitmap::LoadResult::Error::OK;		// !!!
				break;

			// EOI: End Of Image
			case JPG_EOI:
				if (!found_SOS)
					return Bitmap::LoadResult::Error::INVALID_FILE;
				return Bitmap::LoadResult::Error::OK;

			// COM: Text comment
			case JPG_COM:
				break;

			// Everything else is regarded as an error
			default:
				return Bitmap::LoadResult::Error::INVALID_FILE;
		}
		if (result != Bitmap::LoadResult::Error::OK)
			return result;

		pos += size;
		if (buffer[pos] != 0xff && buffer[pos+1] == 0xff)
			++pos;
	}
	return Bitmap::LoadResult::Error::OK;
}

void BitmapJPG::applyIDCT(short* input, unsigned char* output)
{
	// Lines
	int block[64];
	for (int y = 0; y < 8; ++y)
	{
		short* inp = &input[y*8];
		int* data = &block[y*8];

		// Almost all coefficients are zero?
		if (!inp[1] && !inp[2] && !inp[3] && !inp[4] && !inp[5] && !inp[6] && !inp[7])
		{
			int tmp = (inp[0] * 8);
			for (int x = 0; x < 8; ++x)
				data[x] = tmp;
			continue;
		}

		// Step 1
		int x0, x1, x2, x3, x4, x5, x6, x7, x8;
		x8 = IDCT7 * (inp[1] + inp[7]);
		x4 = x8 + (IDCT1 - IDCT7) * inp[1];
		x5 = x8 - (IDCT1 + IDCT7) * inp[7];
		x8 = IDCT3 * (inp[5] + inp[3]);
		x6 = x8 - (IDCT3 - IDCT5) * inp[5];
		x7 = x8 - (IDCT3 + IDCT5) * inp[3];

		// Step 2
		x0 = inp[0] << 11;
		x1 = inp[4] << 11;
		x8 = x0 + x1;
		x0 -= x1;
		x1 = IDCT6 * (inp[6] + inp[2]);
		x2 = x1 - (IDCT2 + IDCT6) * inp[6];
		x3 = x1 + (IDCT2 - IDCT6) * inp[2];
		x1 = x4 + x6;
		x4 -= x6;
		x6 = x5 + x7;
		x5 -= x7;

		// Step 3
		x7 = x8 + x3;
		x8 -= x3;
		x3 = x0 + x2;
		x0 -= x2;
		x2 = ((x4+x5) * 181 + 128) >> 8;
		x4 = ((x4-x5) * 181 + 128) >> 8;

		// Step 4
		data[0] = (x7+x1+128) >> 8;
		data[1] = (x3+x2+128) >> 8;
		data[2] = (x0+x4+128) >> 8;
		data[3] = (x8+x6+128) >> 8;
		data[4] = (x8-x6+128) >> 8;
		data[5] = (x0-x4+128) >> 8;
		data[6] = (x3-x2+128) >> 8;
		data[7] = (x7-x1+128) >> 8;
	}

	// Columns
	for (int x = 0; x < 8; ++x)
	{
		int* data = &block[x];

		// Almost all coefficients are zero?
		if (!data[8] && !data[16] && !data[24] && !data[32] && !data[40] && !data[48] && !data[56])
		{
			int tmp = (data[0] + 32) >> 6;
			for (int y = 0; y < 8; ++y)
				data[y*8] = tmp;
			continue;
		}

		// Step 1
		int x0, x1, x2, x3, x4, x5, x6, x7, x8;
		x8 = IDCT7 * (data[8] + data[56]) + 4;
		x4 = (x8 + (IDCT1 - IDCT7) * data[8]) >> 3;
		x5 = (x8 - (IDCT1 + IDCT7) * data[56]) >> 3;
		x8 = IDCT3 * (data[40] + data[24]) + 4;
		x6 = (x8 - (IDCT3 - IDCT5) * data[40]) >> 3;
		x7 = (x8 - (IDCT3 + IDCT5) * data[24]) >> 3;

		// Step 2
		x0 = data[0] << 8;
		x1 = data[32] << 8;
		x8 = x0 + x1;
		x0 -= x1;
		x1 = IDCT6 * (data[48]+data[16]) + 4;
		x2 = (x1 - (IDCT2 + IDCT6) * data[48]) >> 3;
		x3 = (x1 + (IDCT2 - IDCT6) * data[16]) >> 3;
		x1 = x4 + x6;
		x4 -= x6;
		x6 = x5 + x7;
		x5 -= x7;

		// Step 3
		x7 = x8 + x3;
		x8 -= x3;
		x3 = x0 + x2;
		x0 -= x2;
		x2 = ((x4+x5) * 181 + 128) >> 8;
		x4 = ((x4-x5) * 181 + 128) >> 8;

		// Step 4
		data[0]  = (x7+x1+8192) >> 14;
		data[8]  = (x3+x2+8192) >> 14;
		data[16] = (x0+x4+8192) >> 14;
		data[24] = (x8+x6+8192) >> 14;
		data[32] = (x8-x6+8192) >> 14;
		data[40] = (x0-x4+8192) >> 14;
		data[48] = (x3-x2+8192) >> 14;
		data[56] = (x7-x1+8192) >> 14;
	}

	// Output as unit8
	for (int i = 0; i < 64; ++i)
	{
		if (block[i] < -128)  block[i] = -128;
		if (block[i] > +127)  block[i] = +127;
		output[i] = block[i] + 128;
	}
}

void BitmapJPG::writeImageData(uint8* data, int ch_number, int px, int py, int factor_x, int factor_y)
{
	int limit_x = factor_x * 8;
	if (px + limit_x > width)
		limit_x = width - px;
	int limit_y = factor_y * 8;
	if (py + limit_y > height)
		limit_y = height - py;
	uint8* img_ptr = (uint8*)image;
	img_ptr += (px + py*width) * 4 + ch_number;
	int wid4 = width * 4;

	if ((factor_x == 1) && (factor_y == 1))
	{
		// No upsampling needed
		for (int y = 0; y < limit_y; ++y)
		{
			uint8* data_ptr = &data[y*8];
			for (int x = 0; x < limit_x; ++x)
				img_ptr[x*4] = data_ptr[x];
			img_ptr += wid4;
		}
	}
	else
	{
		// Upsampling is needed
		for (int y = 0; y < limit_y; ++y)
		{
			uint8* data_ptr = &data[(y / factor_y) * 8];
			if (factor_x == 1)
				for (int x = 0; x < limit_x; ++x)
					img_ptr[x*4] = data_ptr[x];
			else
				for (int x = 0; x < limit_x; ++x)
					img_ptr[x*4] = data_ptr[x/2];
			img_ptr += wid4;
		}
	}
}

void BitmapJPG::convertColors()
{
	// Convert to RGB
	int val_140[256];
	int val_034[256];
	int val_071[256];
	int val_177[256];
	for (int i = 0; i < 256; ++i)
	{
		val_140[i] = int(1.402f * (i-128));
		val_034[i] = int(0.34414f * (i-128));
		val_071[i] = int(0.71414f * (i-128));
		val_177[i] = int(1.772f * (i-128));
	}

	uint8* ptr = (uint8*)image;
	int size = width * height;
	int value;
	if (channels == 1)
	{
		// Grayscale image
		for (int pos = 0; pos < size; ++pos)
		{
			ptr[1] = ptr[0];
			ptr[2] = ptr[0];
			ptr[3] = 0xff;
			ptr += 4;
		}
	}
	else
	{
		// RGB
		for (int pos = 0; pos < size; ++pos)
		{
			int Y  = ptr[0];
			int Cb = ptr[1];
			int Cr = ptr[2];
			value = Y + val_140[Cr];
			ptr[0] = clamp(value, 0, 255);
			value = Y - val_034[Cb] - val_071[Cr];
			ptr[1] = clamp(value, 0, 255);
			value = Y + val_177[Cb];
			ptr[2] = clamp(value, 0, 255);
			ptr[3] = 0xff;
			ptr += 4;
		}
	}
}

void BitmapJPG::buildBitmap(Bitmap& bitmap)
{
	bitmap.create(width, height);
	image = bitmap.getData();

	short* coeff_ptr = coeff_buffer;
	short vector[64];
	uint8 data[64];
	for (int my = 0; my < MCU_count_y; ++my)
	{
		for (int mx = 0; mx < MCU_count_x; ++mx)
		{
			for (int c = 0; c < channels; ++c)
			{
				QuantTable* qtab = &qtable[channel[c].QT_number];
				for (int s = 0; s < channel[c].samples; ++s)
				{
					// Dequantisation
					for (int i = 0; i < 64; ++i)
						vector[i] = coeff_ptr[zigzag_lookup[i]] * qtab->data[zigzag_lookup[i]];
					coeff_ptr += 64;

					// IDCT
					applyIDCT(vector, data);

					// Write image data
					int pos_x = mx * MCU_size_x + (s % channel[c].sample_x) * 8;
					int pos_y = my * MCU_size_y + (s / channel[c].sample_x) * 8;
					writeImageData(data, c, pos_x, pos_y, MCU_size_x / (channel[c].sample_x * 8),
															MCU_size_y / (channel[c].sample_y * 8));
				}
			}
		}
	}

	// Convert to RGB
	convertColors();
	delete[] coeff_buffer;
	return;
}

bool BitmapJPG::decode(Bitmap& bitmap, const uint8* buffer, size_t bufsize, Bitmap::LoadResult& outResult)
{
	// Load JPEG from memory
	outResult.mError = readJPEG(buffer, bufsize);
	if (outResult.mError != Bitmap::LoadResult::Error::OK)
		return false;

	buildBitmap(bitmap);
	return true;
}



bool BitmapCodecJPG::canDecode(const String& format) const
{
	return (format == "jpg" || format == "jpeg");
}

bool BitmapCodecJPG::canEncode(const String& format) const
{
	return false;
}

bool BitmapCodecJPG::decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult)
{
	// Load JPEG from memory
	MemInputStream mstream(stream);
	BitmapJPG codec;
	return codec.decode(bitmap, mstream.getCursor(), mstream.getSize(), outResult);
}

bool BitmapCodecJPG::encode(const Bitmap& bitmap, OutputStream& stream)
{
	return false;
}
