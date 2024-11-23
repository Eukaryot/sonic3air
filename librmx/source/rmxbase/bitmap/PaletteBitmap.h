/*
*	rmx Library
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class API_EXPORT PaletteBitmap
{
public:
	inline PaletteBitmap() {}
	PaletteBitmap(const PaletteBitmap& toCopy);
	~PaletteBitmap();

	inline bool empty() const			{ return (nullptr == mData); }
	inline bool isEmpty() const			{ return (nullptr == mData); }
	inline bool nonEmpty() const		{ return (nullptr != mData); }

	inline int getWidth() const			{ return mWidth; }
	inline int getHeight() const		{ return mHeight; }
	inline Vec2i getSize() const		{ return Vec2i(mWidth, mHeight); }
	inline int getPixelCount() const	{ return mWidth * mHeight; }
	inline float getAspectRatio() const	{ return (mHeight <= 0) ? 0.0f : (float)mWidth / (float)mHeight; }

	inline uint8* getData()				{ return mData; }
	inline const uint8* getData() const	{ return mData; }

	inline bool isValidPosition(int x, int y) const  { return (uint32)x < (uint32)mWidth && (uint32)y < (uint32)mHeight; }	// Unsigned comparison essentially implies that x and y are >= 0

	// Pixel access
	inline uint8 getPixel(int x, int y) const				{ return mData[x + y * mWidth]; }
	inline uint8* getPixelPointer(int x, int y)				{ return &mData[x + y * mWidth]; }
	inline const uint8* getPixelPointer(int x, int y) const { return &mData[x + y * mWidth]; }

	void setPixel(int x, int y, uint8 color);

	void create(int width, int height);
	void create(int width, int height, uint8 color);
	inline void create(Vec2i size)				{ create(size.x, size.y); }
	inline void create(Vec2i size, uint8 color) { create(size.x, size.y, color); }

	void copy(const PaletteBitmap& source);
	void copy(const PaletteBitmap& source, const Recti& rect);
	void copyRect(const PaletteBitmap& source, const Recti& rect, const Vec2i& destination = Vec2i());
	void swap(PaletteBitmap& other);
	void clear(uint8 color);

	void shiftAllIndices(int8 indexShift);
	void overwriteUnusedPaletteEntries(uint32* palette, uint32 unusedPaletteColor);

	bool loadBMP(const std::vector<uint8>& bmpContent, std::vector<uint32>* outPalette = nullptr);	// Expecting palette colors to use ABGR32 format
	bool saveBMP(std::vector<uint8>& bmpContent, const uint32* palette) const;

	void convertToRGBA(Bitmap& output, const uint32* palette, size_t paletteSize) const;

	// Operators
	inline uint8& operator[](size_t index)							{ return mData[index]; }
	inline const uint8& operator[](size_t index) const				{ return mData[index]; }
	inline PaletteBitmap& operator=(const PaletteBitmap& toCopy)	{ copy(toCopy); return *this; }

private:
	void memcpyRect(uint8* dst, int dwid, uint8* src, int swid, int wid, int hgt);

private:
	uint8* mData = nullptr;
	int mWidth = 0;
	int mHeight = 0;
};
