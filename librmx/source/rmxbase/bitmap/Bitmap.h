/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class API_EXPORT Bitmap
{
public:
	struct LoadResult
	{
		// Error codes
		enum class Error : uint8
		{
			OK = 0,			// No error
			FILE_NOT_FOUND,
			INVALID_FILE,
			NO_IMAGE_DATA,
			UNSUPPORTED,
			FILE_ERROR,
			ERROR = 0xff	// Unknown or other error
		};
		Error mError = Error::OK;
	};

	enum class ColorFormat		// Only used in "convert" method
	{
		INDEXED_16_COLORS	= 4,
		INDEXED_256_COLORS	= 8,
		RGB16				= 16,
		RGB24				= 24,
		RGBA32				= 32
	};

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
	void createReusingMemory(int wid, int hgt, int& reservedSize);
	void createReusingMemory(int wid, int hgt, int& reservedSize, uint32 color);

	inline void create(Vec2i size)												 { return create(size.x, size.y); }
	inline void create(Vec2i size, uint32 color)								 { return create(size.x, size.y, color); }
	inline void createReusingMemory(Vec2i size, int& reservedSize)				 { return createReusingMemory(size.x, size.y, reservedSize); }
	inline void createReusingMemory(Vec2i size, int& reservedSize, uint32 color) { return createReusingMemory(size.x, size.y, reservedSize, color); }

	// Clear bitmap
	void clear();
	void clear(uint32 color);
	void clear(const Color& color);
	void clearRGB(uint32 color);
	void clearAlpha(uint8 alpha);

	inline bool empty() const				{ return (nullptr == mData); }
	inline bool isEmpty() const				{ return (nullptr == mData); }
	inline bool nonEmpty() const			{ return (nullptr != mData); }

	inline int getWidth() const				{ return mWidth; }
	inline int getHeight() const			{ return mHeight; }
	inline Vec2i getSize() const			{ return Vec2i(mWidth, mHeight); }
	inline int getPixelCount() const		{ return mWidth * mHeight; }
	inline float getAspectRatio() const		{ return (mHeight <= 0) ? 0.0f : (float)mWidth / (float)mHeight; }

	inline uint32* getData()				{ return mData; }
	inline const uint32* getData() const	{ return mData; }

	inline bool isValidPosition(int x, int y) const  { return (uint32)x < (uint32)mWidth && (uint32)y < (uint32)mHeight; }	// Unsigned comparison essentially implies that x and y are >= 0

	// Pixel access
	inline uint32 getPixel(int x, int y) const				 { return mData[x + y * mWidth]; }
	inline uint32 getPixel(Vec2i pos) const					 { return mData[pos.x + pos.y * mWidth]; }
	inline uint32* getPixelPointer(int x, int y)			 { return &mData[x + y * mWidth]; }
	inline uint32* getPixelPointer(Vec2i pos)				 { return &mData[pos.x + pos.y * mWidth]; }
	inline const uint32* getPixelPointer(int x, int y) const { return &mData[x + y * mWidth]; }
	inline const uint32* getPixelPointer(Vec2i pos) const	 { return &mData[pos.x + pos.y * mWidth]; }

	uint32 getPixelSafe(int x, int y) const;
	uint32* getPixelPointerSafe(int x, int y);
	const uint32* getPixelPointerSafe(int x, int y) const;

	inline uint32 getPixelSafe(Vec2i pos) const					{ return getPixelSafe(pos.x, pos.y); }
	inline uint32* getPixelPointerSafe(Vec2i pos)				{ return getPixelPointerSafe(pos.x, pos.y); }
	inline const uint32* getPixelPointerSafe(Vec2i pos) const	{ return getPixelPointerSafe(pos.x, pos.y); }

	uint32 sampleLinear(float x, float y) const;
	inline uint32 sampleLinear(Vec2f pos) const  { return sampleLinear(pos.x, pos.y); }

	void setPixel(int x, int y, uint32 color);
	void setPixel(int x, int y, float red, float green, float blue, float alpha = 1.0f);
	inline void setPixel(Vec2i pos, uint32 color)											{ setPixel(pos.x, pos.y, color); }
	inline void setPixel(Vec2i pos, float red, float green, float blue, float alpha = 1.0f)	{ setPixel(pos.x, pos.y, red, green, blue, alpha); }

	bool decode(InputStream& stream, LoadResult& outResult, const char* format = nullptr);
	bool encode(OutputStream& stream, const char* format) const;

	uint8* convert(ColorFormat format, int& size, uint32* palette = nullptr);

	bool load(const WString& filename, LoadResult* outResult = nullptr);
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
	inline uint32& operator[](size_t index)				{ return mData[index]; }
	inline const uint32& operator[](size_t index) const	{ return mData[index]; }
	inline Bitmap& operator=(const Bitmap& bmp)			{ copy(bmp); return *this; }

private:
	void memcpyRect(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt);
	void memcpyBlend(uint32* dst, int dwid, uint32* src, int swid, int wid, int hgt);
	void convert2palette(uint8* output, int colors, uint32* palette);

private:
	uint32* mData = nullptr;	// Pixels in ABGR32 format, i.e. each pixel is encoded as 0xAABBGGRR
	int mWidth = 0;
	int mHeight = 0;
};


namespace rmx
{
	class API_EXPORT IBitmapCodec
	{
	public:
		virtual bool canDecode(const String& format) const  { return false; }
		virtual bool canEncode(const String& format) const  { return false; }
		virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult)  { return false; }
		virtual bool encode(const Bitmap& bitmap, OutputStream& stream)  { return false; }
	};

	class API_EXPORT BitmapCodecList
	{
	public:
		std::vector<IBitmapCodec*> mList;

	public:
		~BitmapCodecList()
		{
			for (IBitmapCodec* codec : mList)
				delete codec;
			mList.clear();
		}

		template<class CLASS> void add()  { mList.push_back(new CLASS()); }

	public:
		static BitmapCodecList mCodecs;
	};
}
