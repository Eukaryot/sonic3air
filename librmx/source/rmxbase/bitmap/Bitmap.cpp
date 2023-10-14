/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


Bitmap::CodecList Bitmap::mCodecs;

Bitmap::Bitmap()
{
}

Bitmap::Bitmap(const Bitmap& bitmap)
{
	copy(bitmap);
}

Bitmap::Bitmap(const String& filename)
{
	load(filename.toWString());
}

Bitmap::Bitmap(const WString& filename)
{
	load(filename);
}

Bitmap::~Bitmap()
{
	clear();
}

void Bitmap::copy(const Bitmap& source)
{
	if (source.mWidth != mWidth || source.mHeight != mHeight)
	{
		clear();
		if (nullptr == source.mData)
			return;

		mWidth = source.mWidth;
		mHeight = source.mHeight;
		mData = new uint32[mWidth*mHeight];
	}
	memcpy(mData, source.mData, mWidth*mHeight*4);
}

void Bitmap::copy(const Bitmap& source, const Recti& rect)
{
	clear();
	if (nullptr == source.mData)
		return;

	int px = rect.x;
	int py = rect.y;
	int sx = rect.width;
	int sy = rect.height;
	if (px < 0)  { sx += px;  px = 0; }
	if (py < 0)  { sy += py;  py = 0; }
	if (px + sx > source.mWidth)   sx = source.mWidth - px;
	if (py + sy > source.mHeight)  sy = source.mHeight - py;
	if (sx <= 0 || sy <= 0)
		return;

	mWidth = sx;
	mHeight = sy;
	mData = new uint32[mWidth*mHeight];
	memcpyRect(mData, mWidth, &source.mData[px+py*source.mWidth], source.mWidth, mWidth, mHeight);
}

void Bitmap::copy(void* source, int wid, int hgt)
{
	clear();
	if (nullptr == source)
		return;

	mWidth = wid;
	mHeight = hgt;
	mData = new uint32[mWidth*mHeight];
	memcpy(mData, source, mWidth*mHeight*4);
}

void Bitmap::create(int wid, int hgt)
{
	if (nullptr != mData && mWidth == wid && mHeight == hgt)
		return;

	delete[] mData;
	mWidth = wid;
	mHeight = hgt;
	mData = new uint32[mWidth*mHeight];
}

void Bitmap::create(int wid, int hgt, uint32 color)
{
	create(wid, hgt);
	clear(color);
}

void Bitmap::createReusingMemory(int wid, int hgt, int& reservedSize)
{
	const int size = wid * hgt;
	if (reservedSize < size)
	{
		create(wid, hgt);
		reservedSize = size;
	}
	else
	{
		mWidth = wid;
		mHeight = hgt;
	}
}

void Bitmap::createReusingMemory(int wid, int hgt, int& reservedSize, uint32 color)
{
	createReusingMemory(wid, hgt, reservedSize);
	clear(color);
}

void Bitmap::clear()
{
	if (nullptr != mData)
		delete[] mData;
	mData = nullptr;
	mWidth = 0;
	mHeight = 0;
}

void Bitmap::clear(uint32 color)
{
	if (nullptr == mData)
		return;

	if (color == 0)
	{
		memset(mData, 0, mWidth*mHeight*4);
	}
	else
	{
		for (int x = 0; x < mWidth; ++x)
			mData[x] = color;
		for (int y = 1; y < mHeight; ++y)
			memcpy(&mData[y*mWidth], mData, mWidth*4);
	}
}

void Bitmap::clear(const Color& color)
{
	clear(color.getABGR32());
}

void Bitmap::clearRGB(uint32 color)
{
	if (nullptr == mData)
		return;

	uint32* dst = mData;
	uint32* end = dst + getPixelCount();
	for (; dst < end; ++dst)
	{
		*dst = (*dst & 0xff000000) | color;
	}
}

void Bitmap::clearAlpha(uint8 alpha)
{
	if (nullptr == mData)
		return;

	uint8* dst = (uint8*)mData + 3;
	uint8* end = dst + getPixelCount() * 4;
	for (; dst < end; dst += 4)
	{
		*dst = alpha;
	}
}

uint32 Bitmap::getPixelSafe(int x, int y) const
{
	if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		return 0;
	return mData[x + y * mWidth];
}

uint32* Bitmap::getPixelPointerSafe(int x, int y)
{
	if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		return nullptr;
	return &mData[x + y * mWidth];
}

const uint32* Bitmap::getPixelPointerSafe(int x, int y) const
{
	if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		return nullptr;
	return &mData[x + y * mWidth];
}

inline void get_int_frac(float x, int& ix, float& fx, int limit)
{
	if (x < 0.0f)
	{
		ix = 0;
		fx = 0.0f;
		return;
	}
	ix = (int)x;
	fx = x - (float)ix;
	if (ix >= limit-1)
	{
		ix = limit-2;
		fx = 1.0f;
	}
}

uint32 Bitmap::sampleLinear(float x, float y) const
{
	// Bilinear interpolation
	int ix, iy;
	float fx, fy;
	get_int_frac(x, ix, fx, mWidth);
	get_int_frac(y, iy, fy, mHeight);
	uint32 color = 0;
	for (int i = 0; i < 3; ++i)
	{
		const float c = (float)((mData[ix+iy*mWidth]	   >> (i*8)) & 0xff) * (1.0f - fx) * (1.0f - fy)
					  + (float)((mData[ix+1+iy*mWidth]	   >> (i*8)) & 0xff) * fx * (1.0f - fy)
					  + (float)((mData[ix+(iy+1)*mWidth]   >> (i*8)) & 0xff) * (1.0f - fx) * fy
					  + (float)((mData[ix+1+(iy+1)*mWidth] >> (i*8)) & 0xff) * fx * fy;
		color += (int)(c + 0.5f) << (i*8);
	}
	return color;
}

void Bitmap::setPixel(int x, int y, uint32 color)
{
	if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		return;
	mData[x+y*mWidth] = color;
}

void Bitmap::setPixel(int x, int y, float red, float green, float blue, float alpha)
{
	if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		return;
	mData[x+y*mWidth] = (uint32)(saturate(red)   * 255.0f)
					 + ((uint32)(saturate(green) * 255.0f) << 8)
					 + ((uint32)(saturate(blue)  * 255.0f) << 16)
					 + ((uint32)(saturate(alpha) * 255.0f) << 24);
}

bool Bitmap::decode(InputStream& stream, Bitmap::LoadResult& outResult, const char* format)
{
	// Load bitmap from memory
	for (IBitmapCodec* codec : mCodecs.mList)
	{
		if (nullptr != format && !codec->canDecode(format))
			continue;

		const bool result = codec->decode(*this, stream, outResult);
		if (result)
			return true;

		stream.rewind();
	}
	return false;
}

bool Bitmap::encode(OutputStream& stream, const char* format) const
{
	for (IBitmapCodec* codec : mCodecs.mList)
	{
		if (!codec->canEncode(format))
			continue;

		const bool result = codec->encode(*this, stream);
		if (result)
			return true;

		stream.rewind();
	}
	return false;
}

uint8* Bitmap::convert(ColorFormat format, int& size, uint32* palette)
{
	// Convert into a different color format
	uint8* output = nullptr;
	const int pixels = mWidth * mHeight;
	size = pixels;
	switch (format)
	{
		// Convert RGBA -> RGB
		case ColorFormat::RGB24:
		{
			size *= 3;
			output = new uint8[size];
			for (int i = 0; i < pixels; ++i)
				memcpy(&output[i*3], &mData[i], 3);
			return output;
		}

		// Reduce to 16-bit
		case ColorFormat::RGB16:
		{
			size *= 2;
			output = new uint8[size];
			for (int i = 0; i < pixels; ++i)
			{
				uint16 color = 0;
				color += (mData[i] >> 3) & 0x001f;
				color += (mData[i] >> 5) & 0x07e0;
				color += (mData[i] >> 8) & 0xf800;
				*(uint16*)(&output[i*2]) = color;
			}
			return output;
		}

		// Create palette with 256 colors
		case ColorFormat::INDEXED_256_COLORS:
		{
			if (!palette)
				break;
			output = new uint8[size];
			convert2palette(output, 256, palette);
			return output;
		}

		// Create palette with 16 colors
		case ColorFormat::INDEXED_16_COLORS:
		{
			if (!palette)
				break;
			uint8* tmp = new uint8[size];
			convert2palette(tmp, 16, palette);
			size /= 2;
			output = new uint8[size];
			for (int i = 0; i < pixels; i += 2)
				output[i/2] = (tmp[i] << 4) + tmp[i+1];
			delete[] tmp;
			return output;
		}

		// No conversion
		default:
		{
			size *= 4;
			output = new uint8[size];
			memcpy(output, mData, size);
			return output;
		}
	}

	size = 0;
	return nullptr;
}

bool Bitmap::load(const WString& filename, LoadResult* outResult)
{
	// Load from input stream
	InputStream* stream = nullptr;
	{
		InputStream* fs = FTX::FileSystem->createInputStream(*filename);
		if (nullptr != fs)
		{
			if (fs->valid())
				stream = fs;
			else
				SAFE_DELETE(fs);
		}
	}

	if (nullptr == stream)
	{
		if (nullptr != outResult)
			outResult->mError = LoadResult::Error::FILE_NOT_FOUND;
		return false;
	}

	// Automatic file type recognition by file extension
	WString format;
	const int pos = filename.findChar(L'.', filename.length()-1, -1);
	if (pos > 0)
		format.makeSubString(filename, pos+1, -1);

	LoadResult loadResult;
	bool success = decode(*stream, loadResult, *format.toString());
	if (!success)
		success = decode(*stream, loadResult);
	if (nullptr != outResult)
		*outResult = loadResult;

	delete stream;
	return success;
}

bool Bitmap::save(const WString& filename)
{
	// TODO: First test if file can be created / written to

	// File extension is the format to save
	WString format;
	int pos = filename.findChar(L'.', filename.length()-1, -1);
	if (pos > 0)
		format.makeSubString(filename, pos+1, -1);

	DynOutputStream stream;
	bool result = encode(stream, *format.toString());
	if (!result)
		return false;

	// Save file
	MemOutputStream s0(stream.getPosition());
	result = stream.saveTo(s0);
	result = s0.saveToFile(filename.toString());
	return result;
}

void Bitmap::insert(int ax, int ay, const Bitmap& source)
{
	insert(ax, ay, source, Recti(0, 0, source.mWidth, source.mHeight));
}

void Bitmap::insert(int ax, int ay, const Bitmap& source, const Recti& rect)
{
	if (nullptr == source.mData)
		return;

	int px = rect.x;
	int py = rect.y;
	int sx = rect.width;
	int sy = rect.height;

	if (px < 0)  { sx += px;  ax -= px;  px = 0; }
	if (py < 0)  { sy += py;  ay -= py;  py = 0; }
	if (px + sx > source.mWidth)  sx = source.mWidth - px;
	if (py + sy > source.mHeight) sy = source.mHeight - py;
	if (sx <= 0 || sy <= 0)
		return;

	if (ax < 0)  { sx += ax;  px -= ax;  ax = 0; }
	if (ay < 0)  { sy += ay;  py -= ay;  ay = 0; }
	if (ax + sx > mWidth)  sx = mWidth - ax;
	if (ay + sy > mHeight) sy = mHeight - ay;
	if (sx <= 0 || sy <= 0)
		return;

	memcpyRect(&mData[ax+ay*mWidth], mWidth, &source.mData[px+py*source.mWidth], source.mWidth, sx, sy);
}

void Bitmap::insertBlend(int ax, int ay, const Bitmap& source)
{
	insertBlend(ax, ay, source, Recti(0, 0, source.mWidth, source.mHeight));
}

void Bitmap::insertBlend(int ax, int ay, const Bitmap& source, const Recti& rect)
{
	if (nullptr == source.mData)
		return;

	int px = rect.x;
	int py = rect.y;
	int sx = rect.width;
	int sy = rect.height;

	if (px < 0)  { sx += px;  ax -= px;  px = 0; }
	if (py < 0)  { sy += py;  ay -= py;  py = 0; }
	if (px + sx > source.mWidth)  sx = source.mWidth - px;
	if (py + sy > source.mHeight) sy = source.mHeight - py;
	if (sx <= 0 || sy <= 0)
		return;

	if (ax < 0)  { sx += ax;  px -= ax;  ax = 0; }
	if (ay < 0)  { sy += ay;  py -= ay;  ay = 0; }
	if (ax + sx > mWidth)  sx = mWidth - ax;
	if (ay + sy > mHeight) sy = mHeight - ay;
	if (sx <= 0 || sy <= 0)
		return;

	memcpyBlend(&mData[ax+ay*mWidth], mWidth, &source.mData[px+py*source.mWidth], source.mWidth, sx, sy);
}

void Bitmap::resize(int wid, int hgt)
{
	if (wid <= 0 || hgt <= 0)
	{
		clear();
		return;
	}
	uint32* mData2 = new uint32[wid*hgt];
	memcpyRect(mData2, wid, mData, mWidth, std::min(wid, mWidth), std::min(hgt, mHeight));
	delete[] mData;
	mData = mData2;
	mWidth = wid;
	mHeight = hgt;
}

void Bitmap::swapRedBlue()
{
	if (nullptr == mData)
		return;
	int wxh = mWidth * mHeight;
	for (int i = 0; i < wxh; ++i)
	{
		mData[i] = (mData[i] & 0xff00ff00)
				| ((mData[i] & 0x00ff0000) >> 16)
				| ((mData[i] & 0x000000ff) << 16);
	}
}

void Bitmap::mirrorHorizontal()
{
	if (nullptr == mData)
		return;
	uint32* mData2 = new uint32[mWidth*mHeight];
	for (int y = 0; y < mHeight; ++y)
		for (int x = 0; x < mWidth; ++x)
			mData2[x+y*mWidth] = mData[(mWidth-x-1)+y*mWidth];
	delete[] mData;
	mData = mData2;
}

void Bitmap::mirrorVertical()
{
	if (nullptr == mData)
		return;
	uint32* mData2 = new uint32[mWidth*mHeight];
	for (int y = 0; y < mHeight; ++y)
		memcpy(&mData2[y*mWidth], &mData[(mHeight-y-1)*mWidth], mWidth*4);
	delete[] mData;
	mData = mData2;
}

void Bitmap::blendBG(uint32 color)
{
	int size = mWidth * mHeight;
	float bg_value[3];
	for (int c = 0; c < 3; ++c)
		bg_value[c] = float((color >> (c*8)) & 0xff);

	for (int i = 0; i < size; ++i)
	{
		float alpha = float((mData[i] >> 24) & 0xff) / 255.0f;
		int output[3];
		for (int c = 0; c < 3; ++c)
		{
			int value = (mData[i] >> (c*8)) & 0xff;
			output[c] = int(float(value) * alpha + float(bg_value[c]) * (1.0f - alpha) + 0.5f);
		}
		mData[i] = 0xff000000 + output[0] + (output[1] << 8) + (output[2] << 16);
	}
}

void Bitmap::gaussianBlur(const Bitmap& source, float sigma)
{
	if (nullptr == source.mData || sigma <= 0.0f)
		return;
	if (&source != this)
		create(source.mWidth, source.mHeight);

	int size = (int)(3.0f * sigma + 0.5f);
	float* factors = new float[size*2+1];
	float* factors_alpha = new float[size*2+1];
	for (int j = 0; j <= size; ++j)
	{
		factors[size+j] = 0.0f;
		for (int i = -2; i <= 2; ++i)		// Evaluate with 5 samples for a more precise result
		{
			float s = ((float)j + (float)i * 0.2f) / sigma;
			factors[size+j] += exp(-0.5f * s * s);
		}
		factors[size-j] = factors[size+j];
	}

	// Two passes: horizontal and vertical
	Bitmap temp;
	temp.create(mWidth, mHeight);
	for (int step = 0; step < 2; ++step)
	{
		int maxz;
		uint8* src;
		uint8* dst;
		int swid = 4;
		if (step == 0)	{ maxz = mWidth;  src = (uint8*)source.mData; dst = (uint8*)temp.mData;            }
				  else	{ maxz = mHeight; src = (uint8*)temp.mData;   dst = (uint8*)mData;  swid *= mWidth; }

		int wxh = mWidth * mHeight;
		for (int i = 0; i < wxh; ++i)
		{
			int z = (step == 0) ? (i % mWidth) : (i / mWidth);
			int min_j = (z >= size)     ? -size : -z;
			int max_j = (z < maxz-size) ? +size : maxz-z-1;
			for (int j = min_j; j <= max_j; ++j)
				factors_alpha[size+j] = factors[size+j] * (float)(src[j*swid+3]) / 255.0f;
			for (int c = 0; c < 4; ++c)
			{
				float value = 0.0f;
				float sum = 0.0f;
				float* factor = (c < 3) ? &factors_alpha[size] : &factors[size];
				for (int j = min_j; j <= max_j; ++j)
				{
					value += factor[j] * (float)src[j*swid+c];
					sum += factor[j];
				}
				dst[c] = (uint8)(value / sum + 0.5f);
			}
			src += 4;
			dst += 4;
		}
	}
	delete[] factors;
	delete[] factors_alpha;
}

void Bitmap::gaussianBlur(const Bitmap& source, float sigma, int channel)
{
	if (nullptr == source.mData || sigma <= 0.0f || channel < 0 || channel > 3)
		return;
	if (&source != this)
		copy(source);

	int size = (int)(3.0f * sigma + 0.5f);
	float* factors = new float[size*2+1];
	for (int j = 0; j <= size; ++j)
	{
		factors[size+j] = 0.0f;
		for (int i = -2; i <= 2; ++i)		// Evaluate with 5 samples for a more precise result
		{
			float s = ((float)j + (float)i * 0.2f) / sigma;
			factors[size+j] += exp(-0.5f * s * s);
		}
		factors[size-j] = factors[size+j];
	}

	// Two passes: horizontal and vertical
	Bitmap bmp;
	bmp.create(mWidth, mHeight);
	for (int step = 0; step < 2; ++step)
	{
		int maxz;
		uint8* src;
		uint8* dst;
		int swid = 4;
		if (step == 0)	{ maxz = mWidth;  src = (uint8*)mData;     dst = (uint8*)bmp.mData;             }
				  else	{ maxz = mHeight; src = (uint8*)bmp.mData; dst = (uint8*)mData;  swid *= mWidth; }

		int wxh = mWidth * mHeight;
		for (int i = 0; i < wxh; ++i)
		{
			int z = (step == 0) ? (i % mWidth) : (i / mWidth);
			int min_j = (z >= size)     ? -size : -z;
			int max_j = (z < maxz-size) ? +size : maxz-z-1;
			float value = 0.0f;
			float sum = 0.0f;
			float* factor = &factors[size];
			for (int j = min_j; j <= max_j; ++j)
			{
				value += factor[j] * (float)src[j*swid+channel];
				sum += factor[j];
			}
			dst[channel] = (uint8)(value / sum + 0.5f);
			src += 4;
			dst += 4;
		}
	}
	delete[] factors;
}

void Bitmap::sampleDown(const Bitmap& source, bool roundup)
{
	// Reduce resolution to 50% and save the result here
	if (nullptr == source.mData)
		return;

	int sx = source.mWidth;
	int sy = source.mHeight;
	int nx = (sx == 1) ? 1 : roundup ? (sx+1)/2 : sx/2;
	int ny = (sy == 1) ? 1 : roundup ? (sy+1)/2 : sy/2;
	int max_x = (nx <= sx/2) ? nx : sx/2;
	uint32* newData = new uint32[nx*ny];
	for (int y = 0; y < ny; ++y)
	{
		uint32* src = &source.mData[y*2*sx];
		uint32* dst = &newData[y*nx];
		for (int x = 0; x < max_x; ++x)
			dst[x] = ((src[x*2] & 0xfefefefe) >> 1) + ((src[x*2+1] & 0xfefefefe) >> 1);
		if (nx*2-1 >= sx)
			dst[max_x] = src[max_x*2];
		if (y*2+1 < sy)
			src += sx;
		for (int x = 0; x < max_x; ++x)
		{
			uint32 color = ((src[x*2] & 0xfefefefe) >> 1) + ((src[x*2+1] & 0xfefefefe) >> 1);
			dst[x] = ((dst[x] & 0xfefefefe) >> 1) + ((color & 0xfefefefe) >> 1);
		}
		if (nx*2-1 >= sx)
			dst[max_x] = ((dst[max_x] & 0xfefefefe) >> 1) + ((src[max_x*2] & 0xfefefefe) >> 1);
	}
	delete[] mData;
	mData = newData;
	mWidth = nx;
	mHeight = ny;
}

void Bitmap::rescale(const Bitmap& source, int wid, int hgt)
{
	// Upscale or downscale bitmap
	if (nullptr == source.mData)
		return;

	bool rescale_x = (wid != source.mWidth);
	bool rescale_y = (hgt != source.mHeight);
	if (rescale_x == rescale_y)
	{
		if (rescale_x)
		{
			// Complete rescale
			Bitmap temp;
			temp.rescale(source, wid, source.mHeight);
			rescale(temp, wid, hgt);
		}
		else
		{
			// Size is the right one already
			if (&source != this)
				copy(source);
		}
		return;
	}

	if (&source == this)
	{
		Bitmap temp(source);
		rescale(temp, wid, hgt);
		return;
	}

	// Scale in horizontal or vertical direction
	create(wid, hgt);
	int swid = source.mWidth;
	int srcsize, dstsize;
	if (rescale_x)	{ srcsize = source.mWidth;  dstsize = mWidth;  }
			  else	{ srcsize = source.mHeight; dstsize = mHeight; }
	int mulz = rescale_x ? 1 : swid;
	int mulw = rescale_x ? swid : 1;
	uint32* dst = mData;
	float ratio = (float)srcsize / (float)dstsize;
	if (ratio < 1.0f)
	{
		// Upscale
		for (int y = 0; y < mHeight; ++y)
		{
			for (int x = 0; x < mWidth; ++x)
			{
				int z = rescale_x ? x : y;
				int w = rescale_x ? y : x;
				float fz = ((float)z + 0.5f) * ratio - 0.5f;
				if (fz < 0.0f)
					*dst = source.mData[w*mulw];
				else if (fz >= (float)(srcsize-1))
					*dst = source.mData[(srcsize-1)*mulz+w*mulw];
				else
				{
					int iz = (int)fz;
					fz -= (float)iz;
					int offset = iz*mulz + w*mulw;
					uint8* color1 = (uint8*)&source.mData[offset];
					uint8* color2 = (uint8*)&source.mData[offset+mulz];
					for (int c = 0; c < 4; ++c)
						((uint8*)dst)[c] = (uint8)((float)color1[c] * (1.0f - fz)
											   + (float)color2[c] * fz + 0.5f);
				}
				++dst;
			}
		}
	}
	else
	{
		// Downscale
		mulz *= 4;
		for (int y = 0; y < mHeight; ++y)
		{
			for (int x = 0; x < mWidth; ++x)
			{
				int z = rescale_x ? x : y;
				int w = rescale_x ? y : x;
				float f1 = (float)z * ratio;
				float f2 = (float)(z+1) * ratio;
				int i1 = (int)f1;  f1 -= (float)i1;
				int i2 = (int)f2;  f2 -= (float)i2;
				if (i2 >= srcsize)
					{ i2 = srcsize-1;  f2 = 1.0f; }
				uint8* src = (uint8*)&source.mData[w*mulw];
				for (int c = 0; c < 4; ++c)
				{
					float value = 0.0f;
					for (int i = i1+1; i < i2; ++i)
						value += (float)src[i*mulz+c];
					value += (float)src[i1*mulz+c] * (1.0f - f1);
					value += (float)src[i2*mulz+c] * f2;
					((uint8*)dst)[c] = (uint8)(value / ratio + 0.5f);
				}
				++dst;
			}
		}
	}
}

void Bitmap::rescale(int wid, int hgt)
{
	rescale(*this, wid, hgt);
}

void Bitmap::swap(Bitmap& other)
{
	std::swap(mData, other.mData);
	std::swap(mWidth, other.mWidth);
	std::swap(mHeight, other.mHeight);
}

inline void Bitmap::memcpyRect(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt)
{
	wid *= 4;			// 4 Bytes per pixel
	for (int y = 0; y < hgt; ++y)
		memcpy(&dst[y*dwid], &src[y*swid], wid);
}

inline void Bitmap::memcpyBlend(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt)
{
	for (int y = 0; y < hgt; ++y)
	{
		for (int x = 0; x < wid; ++x)
		{
			uint8* dst_ptr = (uint8*)(&dst[x+y*dwid]);
			uint8* src_ptr = (uint8*)(&src[x+y*swid]);
			float alpha = (float)src_ptr[3] / 255.0f;
			float alpha_dst = (float)dst_ptr[3] / 255.0f;
			float A = 1.0f - (1.0f - alpha) * (1.0f - alpha_dst);
			for (int c = 0; c < 3; ++c)
				dst_ptr[c] = (uint8)(((float)src_ptr[c] * alpha + (float)dst_ptr[c] * alpha_dst * (1.0f - alpha)) / A);
			dst_ptr[3] = (uint8)(A * 255.0f);
		}
	}
}

void Bitmap::convert2palette(uint8* output, int colors, uint32* palette)
{
	// Convert into a palette-based image
	memset(palette, 0, colors*4);
	struct octree_struct
	{
		int count;
		int red, green, blue;
		int index;
		bool is_leaf;
		octree_struct* child[8];
	};
	octree_struct octree_root;
	octree_root.count = 0;
	octree_root.red   = 0;
	octree_root.green = 0;
	octree_root.blue  = 0;
	octree_root.index = -1;
	octree_root.is_leaf = false;
	for (int i = 0; i < 8; ++i)
		octree_root.child[i] = 0;
	int leaf_count = 0;

	for (int y = 0; y < mHeight; ++y)
		for (int x = 0; x < mWidth; ++x)
		{
			uint32 color = mData[x+y*mWidth] & 0xffffff;		// Alphakanal ignorieren
			octree_struct* oct = &octree_root;
			for (int j = 0; j < 8; ++j)
			{
				int index = ((color >> 7) & 0x1)
					+ ((color >> 14) & 0x2)
					+ ((color >> 21) & 0x4);
				if (oct->child[index])
					oct = oct->child[index];
				else
				{
					oct->child[index] = new octree_struct;
					oct = oct->child[index];
					oct->count = 0;
					oct->red   = 0;
					oct->green = 0;
					oct->blue  = 0;
					oct->index = -1;
					oct->is_leaf = false;
					for (int i = 0; i < 8; ++i)
						oct->child[i] = 0;
				}
				color <<= 1;

				if ((j == 7) && (!oct->count))
				{
					oct->is_leaf = true;
					++leaf_count;
				}
				++oct->count;
				oct->red   += mData[x+y*mWidth] & 0xff;
				oct->green += (mData[x+y*mWidth] >> 8) & 0xff;
				oct->blue  += (mData[x+y*mWidth] >> 16) & 0xff;
			}
		}

	// Reduce colors
	octree_struct* stack[64];
	int stack_size;
	while (leaf_count > colors)
	{
		octree_struct* min_oct = 0;
		int min_count = 0x7fffffff;
		stack[0] = &octree_root;
		stack_size = 1;
		while (stack_size)
		{
			--stack_size;
			octree_struct* oct = stack[stack_size];
			bool all_leaves = true;
			for (int i = 0; i < 8; ++i)
				if (oct->child[i])
					if (!oct->child[i]->is_leaf)
					{
						stack[stack_size] = oct->child[i];
						++stack_size;
						all_leaves = false;
					}
			if (all_leaves)
				if (oct->count < min_count)
				{
					min_oct = oct;
					min_count = oct->count;
				}
		}

		for (int i = 0; i < 8; ++i)
			if (min_oct->child[i])
			{
				SAFE_DELETE(min_oct->child[i]);
				--leaf_count;
			}
		min_oct->is_leaf = true;
		++leaf_count;
	}

	// Create palette
	int pal_size = 0;
	stack[0] = &octree_root;
	stack_size = 1;
	while (stack_size)
	{
		--stack_size;
		octree_struct* oct = stack[stack_size];
		if (oct->is_leaf)
		{
			palette[pal_size] = (oct->red / oct->count)
				+ ((oct->green / oct->count) << 8)
				+ ((oct->blue / oct->count) << 16);
			oct->index = pal_size;
			++pal_size;
			continue;
		}
		for (int i = 0; i < 8; ++i)
			if (oct->child[i])
			{
				stack[stack_size] = oct->child[i];
				++stack_size;
			}
	}

	// Convert bitmap data
	for (int y = 0; y < mHeight; ++y)
	{
		for (int x = 0; x < mWidth; ++x)
		{
			uint32 color = mData[x+y*mWidth] & 0xffffff;	// Ignore alpha channel
			octree_struct* oct = &octree_root;
			while (!oct->is_leaf)
			{
				int index = ((color >> 7) & 0x1)
					+ ((color >> 14) & 0x2)
					+ ((color >> 21) & 0x4);
				oct = oct->child[index];
				color <<= 1;
			}
			output[x+y*mWidth] = (uint8)(oct->index);
		}
	}
}
