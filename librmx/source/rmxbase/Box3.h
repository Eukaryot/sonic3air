/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Box3
*		Axis-aligned 3D box.
*/

#pragma once


template<typename TYPE> class TBox3
{
public:
	Vec3<TYPE> mMin;
	Vec3<TYPE> mMax;

public:
	TBox3() {}
	TBox3(const TBox3& box) : mMin(box.mMin), mMax(box.mMax) {}

	template<typename T> TBox3(const TBox3<T>& box) : mMin(Vec3<TYPE>(box.mMin)), mMax(Vec3<TYPE>(box.mMax)) {}

	Vec3<TYPE> getSize() const				{ return (mMax - mMin); }
	void setSize(TYPE sx, TYPE sy, TYPE sz)	{ mMax = mMin + Vec3<TYPE>(sx, sy, sz); }

	bool empty() const	{ return (mMax.x <= mMin.x) || (mMax.y <= mMin.y); }
	bool nonEmpty() const	{ return !empty(); }

	bool equal(const TBox3& other) const	{ return (mMin == other.mMin) && (mMax == other.mMax); }

	bool contains(TYPE px, TYPE py, TYPE pz) const
	{
		return (px >= mMin.x && px < mMax.x && py >= mMin.y && py < mMax.y && pz >= mMin.z && pz < mMax.z);
	}

	bool contains(const Vec3<TYPE>& vec) const
	{
		return contains(vec.x, vec.y, vec.z);
	}

	void intersect(const TBox3& other)
	{
		intersect(other, TBox3(*this));
	}

	void intersect(const TBox3& other1, const TBox3& other2)
	{
		mMin.x = std::max(other1.mMin.x, other2.mMin.x);
		mMin.y = std::max(other1.mMin.y, other2.mMin.y);
		mMin.z = std::max(other1.mMin.z, other2.mMin.z);
		mMax.x = std::min(other1.mMax.x, other2.mMax.x);
		mMax.y = std::min(other1.mMax.y, other2.mMax.y);
		mMax.z = std::min(other1.mMax.z, other2.mMax.z);
	}

	TBox3& operator=(const TBox3& other)
	{
		mMin = other.mMin;
		mMax = other.mMax;
		return *this;
	}

	bool operator==(const TBox3& other) const	{ return equal(other); }
	bool operator!=(const TBox3& other) const	{ return !equal(other); }
};


typedef TBox3<int>   Box3i;
typedef TBox3<float> Box3f;
