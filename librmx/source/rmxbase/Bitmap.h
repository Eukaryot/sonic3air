/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Formats
#define BITMAP_FORMAT_MONOCHROME   1
#define BITMAP_FORMAT_16COLORS	   4
#define BITMAP_FORMAT_256COLORS	   8
#define BITMAP_FORMAT_RGB16		  16
#define BITMAP_FORMAT_RGB24		  24
#define BITMAP_FORMAT_RGBA32	  32


class API_EXPORT Bitmap
{
public:
	// Error codes
	enum class Error : uint8
	{
		OK = 0,
		FILE_NOT_FOUND,
		INVALID_FILE,
		NO_IMAGE_DATA,
		UNSUPPORTED,
		FILE_ERROR,
		ERROR = 0xff	// Unknown or other error
	};

	// Image data and properties
	uint32* mData;
	int mWidth, mHeight;
	mutable Error mError;

public:
	Bitmap();
	Bitmap(const Bitmap& bitmap);
	Bitmap(const String& filename);
	Bitmap(const WString& filename);
	~Bitmap();

	// Create as copy
	void copy(const Bitmap& source);
	void copy(const Bitmap& source, const Recti& rect);
	void copy(void* source, int wid, int hgt);

	// Create a bitmap
	void create(int wid, int hgt);
	void create(int wid, int hgt, uint32 color);

	// Clear bitmap
	void clear();
	void clear(uint32 color);
	void clear(const Color& color);
	void clearRGB(uint32 color);
	void clearAlpha(uint8 alpha);

	bool empty() const				{ return (nullptr == mData); }
	int getWidth() const			{ return mWidth; }
	int getHeight() const			{ return mHeight; }
	Vec2i getSize() const			{ return Vec2i(mWidth, mHeight); }
	int getPixelCount() const		{ return mWidth * mHeight; }
	float getAspectRatio() const	{ return (mHeight <= 0) ? 0.0f : (float)mWidth / (float)mHeight; }

	uint32* getData()				{ return mData; }
	const uint32* getData() const	{ return mData; }

	uint32 getPixel(int x, int y) const;
	uint32* getPixelPointer(int x, int y);
	const uint32* getPixelPointer(int x, int y) const;
	uint32 sampleLinear(float x, float y) const;

	void setPixel(int x, int y, unsigned int color);
	void setPixel(int x, int y, float red, float green, float blue, float alpha = 1.0f);

	bool decode(InputStream& stream, const char* format = nullptr);
	bool encode(OutputStream& stream, const char* format) const;

	uint8* convert(int format, int& size, uint32* palette = nullptr);

	bool load(const WString& filename);
	bool save(const WString& filename);

	void insert(int ax, int ay, const Bitmap& source);
	void insert(int ax, int ay, const Bitmap& source, const Recti& rect);
	void insertBlend(int ax, int ay, const Bitmap& source);
	void insertBlend(int ax, int ay, const Bitmap& source, const Recti& rect);

	void resize(int wid, int hgt);
	void swapRedBlue();
	void mirrorHorizontal();
	void mirrorVertical();
	void blendBG(uint32 color);

	// Operations with source bitmap (allowing source == *this)
	void gaussianBlur(const Bitmap& source, float sigma);
	void gaussianBlur(const Bitmap& source, float sigma, int channel);
	void sampleDown(const Bitmap& source, bool roundup = false);
	void rescale(const Bitmap& source, int wid, int hgt);
	void rescale(int wid, int hgt);

	void swap(Bitmap& other);

	// Operators
	uint32& operator[](size_t index)			 { return mData[index]; }
	const uint32& operator[](size_t index) const { return mData[index]; }
	Bitmap& operator=(const Bitmap& bmp)		 { copy(bmp); return *this; }

private:
	void privateInit();
	void memcpyRect(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt);
	void memcpyBlend(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt);
	void convert2palette(uint8* output, int colors, uint32* palette);

public:
	struct API_EXPORT CodecList
	{
		std::vector<class IBitmapCodec*> mList;
		template<class CLASS> void add()  { mList.push_back(new CLASS()); }
	};
	static CodecList mCodecs;
};



class IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const		  { return false; }
	virtual bool canEncode(const String& format) const		  { return false; }
	virtual bool decode(Bitmap& bitmap, InputStream& stream)  { return false; }
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream)  { return false; }
};

class API_EXPORT BitmapCodecBMP : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecPNG : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecJPG : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecICO : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};
