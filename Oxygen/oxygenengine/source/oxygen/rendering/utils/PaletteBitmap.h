/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class PaletteBitmap
{
public:
	inline PaletteBitmap() {}
	PaletteBitmap(const PaletteBitmap& toCopy);
	~PaletteBitmap();

	inline bool empty() const			{ return (nullptr == mData); }
	inline int getWidth() const			{ return mWidth; }
	inline int getHeight() const		{ return mHeight; }
	inline Vec2i getSize() const		{ return Vec2i(mWidth, mHeight); }
	inline int getPixelCount() const	{ return mWidth * mHeight; }
	inline float getAspectRatio() const	{ return (mHeight <= 0) ? 0.0f : (float)mWidth / (float)mHeight; }

	inline uint8* getData()				{ return mData; }
	inline const uint8* getData() const	{ return mData; }

	inline uint8 getPixel(int x, int y) const				{ return mData[x + y * mWidth]; }
	inline uint8* getPixelPointer(int x, int y)				{ return &mData[x + y * mWidth]; }
	inline const uint8* getPixelPointer(int x, int y) const { return &mData[x + y * mWidth]; }

	void create(uint32 width, uint32 height);
	void create(uint32 width, uint32 height, uint8 fillValue);
	void copy(const PaletteBitmap& source);
	void copy(const PaletteBitmap& source, const Recti& rect);
	void copyRect(const PaletteBitmap& source, const Recti& rect, const Vec2i& destination = Vec2i());
	void swap(PaletteBitmap& other);
	void clear(uint8 color);

	void shiftAllIndices(int8 indexShift);
	void overwriteUnusedPaletteEntries(Color* palette);

	bool loadBMP(const std::vector<uint8>& bmpContent, Color* outPalette = nullptr);
	bool saveBMP(std::vector<uint8>& bmpContent, const Color* palette);

private:
	void memcpyRect(uint8* dst, int dwid, uint8* src, int swid, int wid, int hgt);

public:
	uint8* mData = nullptr;
	uint32 mWidth = 0;
	uint32 mHeight = 0;
};
