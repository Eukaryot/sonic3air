/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Box2
*		Axis-aligned 2D box. In contrast to TRect, it stores the endpoint instead of width/height.
*/

#pragma once


template<typename TYPE> class TBox2
{
public:
	Vec2<TYPE> mMin;
	Vec2<TYPE> mMax;

public:
	TBox2() {}
	TBox2(const TBox2& box) : mMin(box.mMin), mMax(box.mMax) {}

	template<typename T> TBox2(const TBox2<T>& box) : mMin(Vec2<TYPE>(box.mMin)), mMax(Vec2<TYPE>(box.mMax)) {}

	Vec2<TYPE> getSize() const		{ return (mMax - mMin); }
	void setSize(TYPE w, TYPE h)	{ mMax = mMin + Vec2<TYPE>(w, h); }

	float getAspectRatio() const	{ return (float)(mMax.x - mMin.x) / (float)(mMax.y - mMin.y); }

	bool empty() const		{ return (mMax.x <= mMin.x) || (mMax.y <= mMin.y); }
	bool nonEmpty() const	{ return !empty(); }

	bool equal(const TBox2& other) const	{ return (mMin == other.mMin) && (mMax == other.mMax); }

	bool contains(TYPE px, TYPE py) const
	{
		return (px >= mMin.x && px < mMax.x && py >= mMin.y && py < mMax.y);
	}

	bool contains(const Vec2<TYPE>& vec) const
	{
		return contains(vec.x, vec.y);
	}

	void intersect(const TBox2& other)
	{
		intersect(other, TBox2(*this));
	}

	void intersect(const TBox2& other1, const TBox2& other2)
	{
		mMin.x = std::max(other1.mMin.x, other2.mMin.x);
		mMin.y = std::max(other1.mMin.y, other2.mMin.y);
		mMax.x = std::min(other1.mMax.x, other2.mMax.x);
		mMax.y = std::min(other1.mMax.y, other2.mMax.y);
	}

	TBox2& operator=(const TBox2& other)
	{
		mMin = other.mMin;
		mMax = other.mMax;
		return *this;
	}

	bool operator==(const TBox2& other) const	{ return equal(other); }
	bool operator!=(const TBox2& other) const	{ return !equal(other); }
};


typedef TBox2<int>   Box2i;
typedef TBox2<float> Box2f;
