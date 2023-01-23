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
	{ for (int i = 0; i < 2; ++i) { _statement_; } }



template<typename TYPE> class Vec2
{
public:
	union
	{
		TYPE data[2];
		struct { TYPE x, y; };
	};

	enum class Initialization { NONE };
	static const Initialization Uninitialized = Initialization::NONE;

public:
	Vec2() : x(0), y(0)				{}
	explicit Vec2(Initialization)	{}
	explicit Vec2(TYPE value)		{ FORi(data[i] = value); }
	explicit Vec2(const TYPE* vec)	{ FORi(data[i] = vec[i]); }

	Vec2(const Vec2& source) : x(source.x), y(source.y) {}
	Vec2(TYPE x_, TYPE y_) : x(x_), y(y_) {}

	template<typename T> Vec2(const Vec2<T>& source)
	{
		FORi(data[i] = TYPE(source.data[i]));
	}

	void clear()	{ FORi(data[i] = 0); }

	void set(const TYPE* vec)		{ FORi(data[i] = vec[i]); }
	void set(const Vec2& source)	{ FORi(data[i] = source.data[i]); }
	void set(TYPE x_, TYPE y_)		{ x = x_; y = y_; }

	void copyTo(TYPE* dst) const	{ FORi(dst[i] = data[i]); }

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

	Vec2 normalized() const
	{
		Vec2 v(*this);
		v.normalize();
		return v;
	}

	void add(const Vec2& source)						{ FORi(data[i] += source.data[i]); }
	void add(const Vec2& source, TYPE factor)			{ FORi(data[i] += source.data[i] * factor); }
	void sub(const Vec2& source)						{ FORi(data[i] -= source.data[i]); }
	void sub(const Vec2& source1, const Vec2& source2)	{ FORi(data[i] = source1.data[i] - source2.data[i]); }
	void mul(TYPE factor)								{ FORi(data[i] *= factor); }
	void div(TYPE divisor)								{ FORi(data[i] /= divisor); }

	float length() const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * data[i]));
		return sqrtf((float)sum);
	}

	TYPE sqrLen() const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * data[i]));
		return sum;
	}

	float distance(const Vec2& other) const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] - other.data[i]) * (data[i] - other.data[i]));
		return sqrtf((float)sum);
	}

	TYPE sqrDist(const Vec2& other) const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] - other.data[i]) * (data[i] - other.data[i]));
		return sum;
	}

	TYPE dot(const Vec2& other) const
	{
		TYPE sum = 0;
		FORi(sum += (data[i] * other.data[i]));
		return sum;
	}

	void rotate(TYPE angle)
	{
		// 2D rotation
		angle = deg2rad(angle);
		float _cos = cosf(angle);
		float _sin = sinf(angle);
		set(_cos * x - _sin * y, _sin * x + _cos * y);
	}

	void interpolate(const Vec2& other, float factor)
	{
		interpolate(*this, other, factor);
	}

	void interpolate(const Vec2& source1, const Vec2& source2, float factor)
	{
		FORi(data[i] = roundForInt<TYPE>((float)source1.data[i] * (1.0f - factor) + (float)source2.data[i] * factor));
	}

	bool operator==(const Vec2& vec) const	{ FORi( if (data[i] != vec.data[i]) return false; ); return true;  }
	bool operator!=(const Vec2& vec) const	{ FORi( if (data[i] != vec.data[i]) return true; );  return false; }

	Vec2& operator=(const Vec2& source)		{ set(source); return *this; }

	Vec2 operator-() const					{ return Vec2(-x, -y); }

	void operator+=(const Vec2& source)		{ FORi(data[i] += source.data[i]); }
	void operator-=(const Vec2& source)		{ FORi(data[i] -= source.data[i]); }
	void operator*=(const Vec2& source)		{ FORi(data[i] *= source.data[i]); }
	void operator*=(TYPE factor)			{ FORi(data[i] *= factor); }
	void operator/=(const Vec2& source)		{ FORi(data[i] /= source.data[i]); }
	void operator/=(TYPE divisor)			{ FORi(data[i] /= divisor); }

	Vec2 operator+(const Vec2& source) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] + source.data[i]);
		return result;
	}

	Vec2 operator-(const Vec2& source) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] - source.data[i]);
		return result;
	}

	Vec2 operator*(const Vec2& source) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] * source.data[i]);
		return result;
	}

	Vec2 operator*(const TYPE factor) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] * factor);
		return result;
	}

	Vec2 operator/(const Vec2& source) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] / source.data[i]);
		return result;
	}

	Vec2 operator/(const TYPE divisor) const
	{
		Vec2 result(Uninitialized);
		FORi(result.data[i] = data[i] / divisor);
		return result;
	}

	const TYPE& operator[](size_t d) const	{ return data[d]; }
	TYPE& operator[](size_t d)				{ return data[d]; }
	const TYPE* operator*() const			{ return data; }
	TYPE* operator*()						{ return data; }

public:
	static const Vec2 ZERO;
	static const Vec2 UNIT_X;
	static const Vec2 UNIT_Y;
};


template<typename TYPE> const Vec2<TYPE> Vec2<TYPE>::ZERO(0, 0);
template<typename TYPE> const Vec2<TYPE> Vec2<TYPE>::UNIT_X(1, 0);
template<typename TYPE> const Vec2<TYPE> Vec2<TYPE>::UNIT_Y(0, 1);

#undef FORi


// Concrete template instances
typedef Vec2<float> Vec2f;
typedef Vec2<int>   Vec2i;
