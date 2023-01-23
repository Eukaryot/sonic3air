/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Shortcut for a for-loop over all components
#define FORi(_statement_) \
	{ for (int i = 0; i < 3; ++i) { _statement_; } }



template<typename TYPE> class Vec3
{
public:
	union
	{
		TYPE data[3];
		struct { TYPE x, y, z; };
	};

	enum class Initialization { NONE };
	static const Initialization Uninitialized = Initialization::NONE;

public:
	Vec3() : x(0), y(0), z(0)		{}
	explicit Vec3(Initialization)	{}
	explicit Vec3(TYPE value)		{ FORi(data[i] = value); }
	explicit Vec3(const TYPE* vec)	{ FORi(data[i] = vec[i]); }

	Vec3(const Vec3& source) : x(source.x), y(source.y), z(source.z) {}
	Vec3(TYPE x_, TYPE y_, TYPE z_) : x(x_), y(y_), z(z_) {}

	template<typename T> Vec3(const Vec3<T>& source)
	{
		FORi(data[i] = TYPE(source.data[i]));
	}

	void clear()
	{
		FORi(data[i] = 0);
	}

	void set(const TYPE* vec)			{ FORi(data[i] = vec[i]); }
	void set(const Vec3& source)		{ FORi(data[i] = source.data[i]); }
	void set(TYPE x_, TYPE y_, TYPE z_)	{ x = x_; y = y_; z = z_; }

	void copyTo(TYPE* dst) const		{ FORi(dst[i] = data[i]); }

	void neg()
	{
		FORi(data[i] = -data[i]);
	}

	void normalize()
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * data[i]));
		if (sum > 0)
		{
			sum = sqrt(sum);
			FORi(data[i] /= sum);
		}
	}

	Vec3 normalized() const
	{
		Vec3 v(*this);
		v.normalize();
		return v;
	}

	void add(const Vec3& source)						{ FORi(data[i] += source.data[i]); }
	void add(const Vec3& source, TYPE factor)			{ FORi(data[i] += source.data[i] * factor); }
	void sub(const Vec3& source)						{ FORi(data[i] -= source.data[i]); }
	void sub(const Vec3& source1, const Vec3& source2)	{ FORi(data[i] = source1.data[i] - source2.data[i]); }
	void mul(TYPE factor)								{ FORi(data[i] *= factor); }
	void div(TYPE divisor)								{ FORi(data[i] /= divisor); }

	TYPE length() const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * data[i]));
		return sqrt(sum);
	}

	TYPE sqrLen() const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * data[i]));
		return sum;
	}

	TYPE distance(const Vec3& other) const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] - other.data[i]) * (data[i] - other.data[i]));
		return sqrt(sum);
	}

	TYPE sqrDist(const Vec3& other) const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] - other.data[i]) * (data[i] - other.data[i]));
		return sum;
	}

	TYPE dot(const Vec3& other) const
	{
		// Dot product
		TYPE sum = 0;
		FORi(sum += (data[i] * other.data[i]));
		return sum;
	}

	void cross(const Vec3& source1, const Vec3& source2)
	{
		// Cross product
		const TYPE* src1 = source1.data;
		const TYPE* src2 = source2.data;
		x = (src1[1] * src2[2] - src1[2] * src2[1]);
		y = (src1[2] * src2[0] - src1[0] * src2[2]);
		z = (src1[0] * src2[1] - src1[1] * src2[0]);
	}

	void cross(const Vec3& source0, const Vec3& source1, const Vec3& source2)
	{
		// Cross product for triangles
		TYPE src1[3];
		TYPE src2[3];
		FORi(src1[i] = source1.data[i] - source0.data[i]);
		FORi(src2[i] = source2.data[i] - source0.data[i]);
		x = (src1[1] * src2[2] - src1[2] * src2[1]);
		y = (src1[2] * src2[0] - src1[0] * src2[2]);
		z = (src1[0] * src2[1] - src1[1] * src2[0]);
	}

	void mirror(const Vec3& axis)
	{
		// Mirror vector on an axis
		TYPE factor = 2 * dot(axis);
		FORi(data[i] = (factor * axis.data[i]) - data[i];);
	}

	void rotate(TYPE angle, const Vec3& axis)
	{
		// Rotation around arbitrary axis
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		TYPE _dot = dot(axis);
		TYPE tmp[3];
		for (int i = 0; i < 3; ++i)
		{
			tmp[i] = axis.data[i] * _dot * (1.0f - _cos);
			for (int j = 0; j < 3; ++j)
			{
				switch ((i-j+3) % 3)
				{
					case 0:  tmp[i] += (_cos * data[j]);  break;
					case 1:  tmp[i] += (axis.data[(i+1) % 3] * _sin * data[j]);  break;
					case 2:  tmp[i] -= (axis.data[(i+2) % 3] * _sin * data[j]);  break;
				}
			}
		}
		FORi(data[i] = tmp[i]);
	}

	void rotate(TYPE angle, int axis)
	{
		// Rotation around a unit vector (axis=0,1,2)
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		TYPE tmp[3];
		if (axis == 0)
		{
			tmp[0] = x;
			tmp[1] = (_cos * y) - (_sin * z);
			tmp[2] = (_sin * y) + (_cos * z);
		}
		else if (axis == 1)
		{
			tmp[0] = (_sin * z) + (_cos * x);
			tmp[1] = y;
			tmp[2] = (_cos * z) - (_sin * x);
		}
		else
		{
			tmp[0] = (_cos * x) - (_sin * y);
			tmp[1] = (_sin * x) + (_cos * y);
			tmp[2] = z;
		}
		FORi(data[i] = tmp[i]);
	}

	void interpolate(const Vec3& other, TYPE factor)
	{
		FORi(data[i] = data[i] * (1 - factor) + other.data[i] * factor);
	}

	void interpolate(const Vec3& source1, const Vec3& source2, TYPE factor)
	{
		FORi(data[i] = source1.data[i] * (1 - factor) + source2.data[i] * factor);
	}

	bool operator==(const Vec3& vec) const	{ FORi( if (data[i] != vec.data[i]) return false; ); return true;  }
	bool operator!=(const Vec3& vec) const	{ FORi( if (data[i] != vec.data[i]) return true; );  return false; }

	Vec3& operator=(const Vec3& source)		{ set(source); return *this; }

	Vec3 operator-() const					{ return Vec3(-x, -y, -z); }

	void operator+=(const Vec3& source)		{ FORi(data[i] += source.data[i]); }
	void operator-=(const Vec3& source)		{ FORi(data[i] -= source.data[i]); }
	void operator*=(const Vec3& source)		{ FORi(data[i] *= source.data[i]); }
	void operator*=(TYPE factor)			{ FORi(data[i] *= factor); }
	void operator/=(const Vec3& source)		{ FORi(data[i] /= source.data[i]); }
	void operator/=(TYPE divisor)			{ FORi(data[i] /= divisor); }

	Vec3 operator+(const Vec3& source) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] + source.data[i]);
		return result;
	}

	Vec3 operator-(const Vec3& source) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] - source.data[i]);
		return result;
	}

	Vec3 operator*(const Vec3& source) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] * source.data[i]);
		return result;
	}

	Vec3 operator*(const TYPE factor) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] * factor);
		return result;
	}

	Vec3 operator/(const Vec3& source) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] / source.data[i]);
		return result;
	}

	Vec3 operator/(const TYPE divisor) const
	{
		Vec3 result(Uninitialized);
		FORi(result.data[i] = data[i] / divisor);
		return result;
	}

	const TYPE& operator[](size_t d) const	{ return data[d]; }
	TYPE& operator[](size_t d)				{ return data[d]; }
	const TYPE* operator*() const			{ return data; }
	TYPE* operator*()						{ return data; }

public:
	static const Vec3 ZERO;
	static const Vec3 UNIT_X;
	static const Vec3 UNIT_Y;
	static const Vec3 UNIT_Z;
};


template<typename TYPE> const Vec3<TYPE> Vec3<TYPE>::ZERO(0, 0, 0);
template<typename TYPE> const Vec3<TYPE> Vec3<TYPE>::UNIT_X(1, 0, 0);
template<typename TYPE> const Vec3<TYPE> Vec3<TYPE>::UNIT_Y(0, 1, 0);
template<typename TYPE> const Vec3<TYPE> Vec3<TYPE>::UNIT_Z(0, 0, 1);

#undef FORi


// Concrete template instances
typedef Vec3<float> Vec3f;
typedef Vec3<int>   Vec3i;
