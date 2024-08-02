/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


template<typename TYPE> class TRect
{
public:
	union
	{
		struct
		{
			union					// Starting position (upper left corner)
			{
				struct { TYPE x, y; };
				struct { TYPE left, top; };
			};
			TYPE width, height;		// Rectangle size
		};
		TYPE mData[4];
	};

public:
	static TRect getIntersection(const TRect& other1, const TRect& other2)
	{
		const TYPE maxStartX = std::max(other1.x, other2.x);
		const TYPE maxStartY = std::max(other1.y, other2.y);
		const TYPE minEndX = std::min(other1.x + other1.width, other2.x + other2.width);
		const TYPE minEndY = std::min(other1.y + other1.height, other2.y + other2.height);
		return TRect(maxStartX, maxStartY, minEndX - maxStartX, minEndY - maxStartY);
	}

public:
	TRect() : x(0), y(0), width(0), height(0)	{}
	TRect(TYPE px, TYPE py, TYPE w, TYPE h)		{ set(px, py, w, h); }
	TRect(Vec2<TYPE> pos, Vec2<TYPE> size)		{ set(pos, size); }
	TRect(const TRect& rect)					{ set(rect); }
	TRect(const TRect& rect, float px, float py, float w, float h)  { partial(rect, px, py, w, h); }

	template<typename T> TRect(const TRect<T>& rect)	{ set(TYPE(rect.x), TYPE(rect.y), TYPE(rect.width), TYPE(rect.height)); }

	Vec2<TYPE> getPos() const    { return Vec2<TYPE>(x, y); }
	Vec2<TYPE> getSize() const   { return Vec2<TYPE>(width, height); }
	Vec2<TYPE> getCenter() const { return Vec2<TYPE>(x + width / TYPE(2), y + height / TYPE(2)); }
	Vec2<TYPE> getEndPos() const { return Vec2<TYPE>(x + width, y + height); }

	void set(TYPE px, TYPE py, TYPE w, TYPE h)  { x = px;  y = py;  width = w;  height = h; }
	void set(Vec2<TYPE> pos, Vec2<TYPE> size)	{ x = pos.x;  y = pos.y;  width = size.x;  height = size.y; }
	void set(const TRect& rect)			{ set(rect.x, rect.y, rect.width, rect.height); }
	void set(const TYPE* rect)			{ set(rect[0], rect[1], rect[2], rect[3]); }

	void setPos(TYPE px, TYPE py)		{ x = px;  y = py; }
	void setPos(Vec2<TYPE> pos)			{ x = pos.x;  y = pos.y; }
	void setEndPos(TYPE px, TYPE py)	{ width = px - x;  height = py - y; }
	void setEndPos(Vec2<TYPE> pos)		{ width = pos.x - x;  height = pos.y - y; }
	void addPos(TYPE px, TYPE py)		{ x += px;  y += py; }
	void addPos(Vec2<TYPE> pos)			{ x += pos.x;  y += pos.y; }
	void setSize(TYPE w, TYPE h)		{ width = w;  height = h; }
	void setSize(Vec2<TYPE> size)		{ width = size.x;  height = size.y; }
	void addSize(TYPE w, TYPE h)		{ width += w;  height += h; }
	void addSize(Vec2<TYPE> size)		{ width += size.x;  height += size.y; }

	void partial(float px, float py, float w, float h)
	{
		x += roundForInt<TYPE>(width * px);
		y += roundForInt<TYPE>(height * py);
		width  = roundForInt<TYPE>(width * w);
		height = roundForInt<TYPE>(height * h);
	}

	void partial(const TRect& other, float px, float py, float w, float h)
	{
		x = other.x + roundForInt<TYPE>(other.width * px);
		y = other.y + roundForInt<TYPE>(other.height * py);
		width  = roundForInt<TYPE>(other.width * w);
		height = roundForInt<TYPE>(other.height * h);
	}

	void getRect(TYPE* rect) const
	{
		rect[0] = x;
		rect[1] = y;
		rect[2] = width;
		rect[3] = height;
	}

	float getAspectRatio() const  { return (float)width / (float)height; }

	bool empty() const		{ return (width <= 0) || (height <= 0); }
	bool isEmpty() const	{ return (width <= 0) || (height <= 0); }
	bool nonEmpty() const	{ return (width > 0) && (height > 0); }

	bool equal(const TRect& other) const
	{
		for (int i = 0; i < 4; ++i)
			if (mData[i] != other.mData[i])
				return false;
		return true;
	}

	bool contains(TYPE px, TYPE py) const
	{
		if (px < x || px >= x + width)
			return false;
		if (py < y || py >= y + height)
			return false;
		return true;
	}

	bool contains(const Vec2<TYPE>& vec) const	{ return contains(vec.x, vec.y); }

	void extendToInclude(const Vec2<TYPE>& vec)
	{
		if (vec.x < x)			 { width  += (x - vec.x);  x = vec.x; }
		if (vec.y < y)			 { height += (y - vec.y);  y = vec.y; }
		if (vec.x >= x + width)	 { width  = vec.x - x + 1; }
		if (vec.y >= y + height) { height = vec.y - y + 1; }
	}

	Vec2<TYPE> getClosestPoint(const Vec2<TYPE>& vec) const
	{
		Vec2<TYPE> result;
		result.x = (vec.x <= x) ? x : (vec.x >= x + width)  ? (x + width)  : vec.x;
		result.y = (vec.y <= y) ? y : (vec.y >= y + height) ? (y + height) : vec.y;
		return result;
	}

	void intersect(const TRect& other)
	{
		*this = getIntersection(*this, other);
	}

	void intersect(const TRect& other1, const TRect& other2)
	{
		*this = getIntersection(other1, other2);
	}

	TYPE& operator[](size_t index)				{ return mData[index]; }
	const TYPE& operator[](size_t index) const	{ return mData[index]; }

	TRect& operator=(const TRect& other)
	{
		x = other.x;
		y = other.y;
		width = other.width;
		height = other.height;
		return *this;
	}

	bool operator==(const TRect& other) const	{ return equal(other); }
	bool operator!=(const TRect& other) const	{ return !equal(other); }

	TRect operator+(const Vec2<TYPE>& vec) const { return TRect(x + vec.x, y + vec.y, width, height); }
	TRect operator-(const Vec2<TYPE>& vec) const { return TRect(x - vec.x, y - vec.y, width, height); }

	TRect& operator+=(const Vec2<TYPE>& vec)  { x += vec.x; y += vec.y; return *this; }
	TRect& operator-=(const Vec2<TYPE>& vec)  { x -= vec.x; y -= vec.y; return *this; }
};


typedef TRect<int>   Recti;
typedef TRect<float> Rectf;
