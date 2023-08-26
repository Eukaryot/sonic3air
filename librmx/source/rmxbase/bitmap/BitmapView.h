/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

class Bitmap;
template<typename T> class BitmapView;


// View to bitmap data with read and write access
template<typename T>
class BitmapViewMutable
{
friend class BitmapView<T>;

public:
	inline BitmapViewMutable() = default;
	inline BitmapViewMutable(const BitmapViewMutable& other) = default;
	inline BitmapViewMutable(const BitmapViewMutable& other, const Recti& innerRect) : mData(other.mData), mSize(other.mSize), mStride(other.mStride) { makePartialRect(innerRect); }
	inline BitmapViewMutable(Bitmap& bitmap) : mData(bitmap.getData()), mSize(bitmap.getSize()), mStride(bitmap.getWidth()) {}
	inline BitmapViewMutable(Bitmap& bitmap, const Recti& innerRect) : mData(bitmap.getData()), mSize(bitmap.getSize()), mStride(bitmap.getWidth()) { makePartialRect(innerRect); }
	inline BitmapViewMutable(T* data, Vec2i size) : mData(data), mSize(size), mStride(size.x) {}
	inline BitmapViewMutable(T* data, Vec2i size, const Recti& innerRect) : mData(data), mSize(size), mStride(size.x) { makePartialRect(innerRect); }

	inline void operator=(const BitmapViewMutable& other)  { mData = other.mData; mSize = other.mSize; mStride = other.mStride; }

	inline Vec2i getSize() const		{ return mSize; }
	inline bool isEmpty() const			{ return mSize.x <= 0 || mSize.y <= 0; }
	inline int getPixelCount() const	{ return mSize.x * mSize.y; }

	inline T* getData() const						{ return mData; }
	inline T  getPixel(int x, int y) const			{ return mData[x + y * mStride]; }
	inline T  getPixel(Vec2i pos) const				{ return mData[pos.x + pos.y * mWidth]; }
	inline T* getPixelPointer(int x, int y) const	{ return &mData[x + y * mStride]; }
	inline T* getPixelPointer(Vec2i pos) const		{ return &mData[pos.x + pos.y * mStride]; }
	inline T* getLinePointer(int y) const			{ return &mData[y * mStride]; }

	inline void setPixel(int x, int y, T value) const	{ mData[x + y * mStride] = value; }

	inline void makePartialRect(Recti rect)
	{
		rect.intersect(Recti(Vec2i(), mSize));
		mData += rect.x + rect.y * mStride;
		mSize = rect.getSize();
	}

private:
	T* mData = nullptr;
	Vec2i mSize;
	size_t mStride = 0;		// Difference in pixels (not bytes) between lines
};


// Read-only view to bitmap data
template<typename T>
class BitmapView
{
public:
	inline BitmapView() = default;
	inline BitmapView(const BitmapView& other) = default;
	inline BitmapView(const BitmapView& other, const Recti& innerRect) : mData(other.mData), mSize(other.mSize), mStride(other.mStride) { makePartialRect(innerRect); }
	inline BitmapView(const BitmapViewMutable<T>& other) : mData(other.mData), mSize(other.mSize), mStride(other.mStride) {}
	inline BitmapView(const Bitmap& bitmap) : mData(bitmap.getData()), mSize(bitmap.getSize()), mStride(bitmap.getWidth()) {}
	inline BitmapView(const Bitmap& bitmap, const Recti& innerRect) : mData(bitmap.getData()), mSize(bitmap.getSize()), mStride(bitmap.getWidth()) { makePartialRect(innerRect); }
	inline BitmapView(const T* data, Vec2i size) : mData(data), mSize(size), mStride(size.x) {}
	inline BitmapView(const T* data, Vec2i size, const Recti& innerRect) : mData(data), mSize(size), mStride(size.x) { makePartialRect(innerRect); }

	inline void operator=(const BitmapView& other)  { mData = other.mData; mSize = other.mSize; mStride = other.mStride; }

	inline Vec2i getSize() const		{ return mSize; }
	inline bool isEmpty() const			{ return mSize.x <= 0 || mSize.y <= 0; }
	inline int getPixelCount() const	{ return mSize.x * mSize.y; }

	inline const T* getData() const						{ return mData; }
	inline       T  getPixel(int x, int y) const		{ return mData[x + y * mStride]; }
	inline       T  getPixel(Vec2i pos) const			{ return mData[pos.x + pos.y * mWidth]; }
	inline const T* getPixelPointer(int x, int y) const { return &mData[x + y * mStride]; }
	inline const T* getPixelPointer(Vec2i pos) const	{ return &mData[pos.x + pos.y * mStride]; }
	inline const T* getLinePointer(int y) const			{ return &mData[y * mStride]; }

	inline void makePartialRect(Recti rect)
	{
		rect.intersect(Recti(Vec2i(), mSize));
		mData += rect.x + rect.y * mStride;
		mSize = rect.getSize();
	}

private:
	const T* mData = nullptr;
	Vec2i mSize;
	size_t mStride = 0;		// Difference in pixels (not bytes) between lines
};
