/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

template<typename TYPE> class Vec3;
template<typename TYPE> class Mat4;


// Shortcut for a for-loop over all components
#define FORi(_statement_) \
	{ for (int i = 0; i < COLS; ++i) { _statement_; } }

#define FOR_ALL(_statement_) \
	{ for (int i = 0; i < COLS * ROWS; ++i) { _statement_; } }


template<typename TYPE> class Mat3
{
public:
	union
	{
		// Stored as a row-major matrix:
		//  ( [0] [1] [2] )    <->    ( m11 m12 m13 )
		//  ( [3] [4] [5] )    <->    ( m21 m22 m23 )
		//  ( [6] [7] [8] )    <->    ( m31 m32 m33 )

		TYPE data[9];
		struct { TYPE m11, m12, m13, m21, m22, m23, m31, m32, m33; };
	};

	enum class Initialization { NONE };
	static const Initialization Uninitialized = Initialization::NONE;

	static constexpr int COLS = 3;
	static constexpr int ROWS = 3;

public:
	Mat3()							{ setIdentity(); }
	explicit Mat3(Initialization)	{}
	explicit Mat3(TYPE value)		{ FOR_ALL(data[i] = value); }

	Mat3(const Mat3& source)		{ FOR_ALL(data[i] = source.data[i]); }
	explicit Mat3(const TYPE* mat)	{ FOR_ALL(data[i] = mat[i]); }

	explicit Mat3(const Mat4<TYPE>& source)
	{
		setIdentity();
		for (int col = 0; col < 3; ++col)
		{
			for (int row = 0; row < 3; ++row)
			{
				set(col, row, source.get(col, row));
			}
		}
	}

	void set(const Mat3& source)	{ FOR_ALL(data[i] = source.data[i]); }
	void set(const TYPE* mat)		{ FOR_ALL(data[i] = mat[i]); }

	void setZero()
	{
		FOR_ALL(data[i] = 0);
	}

	void setIdentity()
	{
		setZero();
		for (int i = 0; i < ROWS; ++i)
		{
			set(i, i, 1);
		}
	}

	TYPE get(int col, int row) const		{ return data[col + row * COLS]; }
	TYPE& access(int col, int row)			{ return data[col + row * COLS]; }
	void set(int col, int row, TYPE value)	{ data[col + row * COLS] = value; }

	Vec3<TYPE> getRow(int num) const
	{
		Vec3<TYPE> output;
		FORi(output.data[i] = get(i, num));
		return output;
	}

	Vec3<TYPE> getColumn(int num) const
	{
		Vec3<TYPE> output;
		FORi(output.data[i] = get(num, i));
		return output;
	}

	void setRow(int num, const Vec3<TYPE>& input)
	{
		FORi(set(i, num, input.data[i]));
	}

	void setColumn(int num, const Vec3<TYPE>& input)
	{
		FORi(set(num, i, input.data[i]));
	}

	void transpose()
	{
		for (int col = 0; col < COLS - 1; ++col)
		{
			for (int row = col + 1; row < ROWS; ++row)
			{
				std::swap(access(col, row), access(row, col));
			}
		}
	}

	Mat3 transposed() const
	{
		Mat3 result(Uninitialized);
		for (int col = 0; col < COLS; ++col)
		{
			for (int row = 0; row < ROWS; ++row)
			{
				result.set(col, row, get(row, col));
			}
		}
		return result;
	}

	void add(const Mat3& source)	{ FOR_ALL(data[i] += source.data[i]); }
	void sub(const Mat3& source)	{ FOR_ALL(data[i] -= source.data[i]); }
	void mul(TYPE factor)			{ FOR_ALL(data[i] *= factor); }

	Vec3<TYPE> mul(const Vec3<TYPE>& vec) const
	{
		Vec3<TYPE> output(Vec3<TYPE>::Uninitialized);
		for (int row = 0; row < ROWS; ++row)
		{
			TYPE value = 0;
			for (int col = 0; col < COLS; ++col)
			{
				value += get(col, row) * vec.data[col];
			}
			output.data[row] = value;
		}
		return output;
	}

	Mat3 mul(const Mat3& source) const
	{
		Mat3 result(Uninitialized);
		for (int row = 0; row < ROWS; ++row)
		{
			for (int col = 0; col < COLS; ++col)
			{
				TYPE value = 0;
				for (int k = 0; k < COLS; ++k)
				{
					value += get(k, row) * source.get(col, k);
				}
				result.set(col, row, value);
			}
		}
		return result;
	}

	void setScale(TYPE scale)
	{
		setZero();
		for (int i = 0; i < COLS; ++i)
		{
			set(i, i, scale);
		}
	}

	void setScale(const Vec3<TYPE>& scale)
	{
		setZero();
		for (int i = 0; i < COLS; ++i)
		{
			set(i, i, scale);
		}
	}

	void setRotation2D(TYPE angle)
	{
		// TODO: This needs review / testing
		setIdentity();
		angle = deg2rad(angle);
		const TYPE _cos = cos(angle);
		const TYPE _sin = sin(angle);
		data[0] = _cos;	data[1] = -_sin;
		data[3] = _sin;	data[3+1] = _cos;
	}

	void setRotation3D(TYPE angle, const Vec3<TYPE>& axis_)
	{
		// TODO: This needs review / testing
		angle = deg2rad(angle);
		const TYPE _cos = cos(angle);
		const TYPE _sin = sin(angle);
		Vec3<TYPE> axis = axis_.normalized();
		data[0] = axis.data[0] * axis.data[0] * (1.0f - _cos) + _cos;
		data[1] = axis.data[0] * axis.data[1] * (1.0f - _cos) - axis.data[2] * _sin;
		data[2] = axis.data[0] * axis.data[2] * (1.0f - _cos) + axis.data[1] * _sin;
		data[3] = axis.data[0] * axis.data[1] * (1.0f - _cos) + axis.data[2] * _sin;
		data[4] = axis.data[1] * axis.data[1] * (1.0f - _cos) + _cos;
		data[5] = axis.data[1] * axis.data[2] * (1.0f - _cos) - axis.data[0] * _sin;
		data[6] = axis.data[0] * axis.data[2] * (1.0f - _cos) - axis.data[1] * _sin;
		data[7] = axis.data[1] * axis.data[2] * (1.0f - _cos) + axis.data[0] * _sin;
		data[8] = axis.data[2] * axis.data[2] * (1.0f - _cos) + _cos;
	}

	void setRotation3D(TYPE angle, int axis)
	{
		// TODO: This needs review / testing
		setIdentity();
		angle = deg2rad(angle);
		const TYPE _cos = cos(angle);
		const TYPE _sin = sin(angle);
		if (axis == 0)
		{
			data[3+1]   = _cos;
			data[3+2]   = -_sin;
			data[3*2+1] = +_sin;
			data[3*2+2] = _cos;
		}
		else if (axis == 1)
		{
			data[0]     = _cos;
			data[2]     = +_sin;
			data[3*2]   = -_sin;
			data[3*2+2] = _cos;
		}
		else
		{
			data[0]   = _cos;
			data[1]   = -_sin;
			data[3]   = +_sin;
			data[3+1] = _cos;
		}
	}

	void setTransformation(const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setColumn(0, dir0);
		setColumn(1, dir1);
		setColumn(2, dir2);
	}

	void setTransformationInv(const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setRow(0, dir0);
		setRow(1, dir1);
		setRow(2, dir2);
	}

	void setOrthonormal(const Vec3<TYPE>& dir0)
	{
		Vec3<TYPE> zero;
		setOrthonormal(dir0, zero);
	}

	void setOrthonormal(const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1)
	{
		setIdentity();
		if (dir0.sqrLen() == 0)
			return;
		Vec3<TYPE> localX(dir0);
		localX.normalize();
		Vec3<TYPE> localY(dir1);
		Vec3<TYPE> localZ = Vec3<TYPE>::crossProduct(localX, localY);
		if (localZ.sqrLen() == 0)
		{
			localY = Vec3<TYPE>::UNIT_Z;
			localZ = Vec3<TYPE>::crossProduct(localX, localY);
			if (localZ.sqrLen() == 0)
			{
				localY = Vec3<TYPE>::UNIT_Y;
				localZ = Vec3<TYPE>::crossProduct(localX, localY);
			}
		}
		localZ.normalize();
		localY = Vec3<TYPE>::crossProduct(localZ, localX);
		for (int i = 0; i < 3; ++i)
		{
			data[i*COLS+0] = localX.data[i];
			data[i*COLS+1] = localY.data[i];
			data[i*COLS+2] = localZ.data[i];
		}
	}

	void getEulerAngles(TYPE& yaw, TYPE& pitch, TYPE& roll) const
	{
		// Coordinate system used (right-handed):
		//  - forward = positive y-axis
		//  - right   = positive x-axis
		//  - up      = positive z-axis

		// Euler angles:
		//  - yaw   (range 0 to 360)    = rotation around the global up axis (z)
		//  - pitch (range -90 to 90)   = rotation around the local right axis (x)
		//  - roll  (range -180 to 180) = rotation around the local forward axis (y)

		Vec3<TYPE> right = getColumn(0);	// Local x-axis
		Vec3<TYPE> forward = getColumn(1);	// Local y-axis

		// Get yaw from forward vector
		yaw = std::atan2(forward.y, forward.x) - (TYPE)PI_DOUBLE / 2;
		if (yaw < 0)
			yaw += (TYPE)PI_DOUBLE * 2;
		const Vec3<TYPE> flatForward(std::cos(yaw), std::sin(yaw), 0);
		yaw = rad2deg(yaw);

		// Get pitch also from forward vector
		pitch = rad2deg(std::asin(forward.z));

		// Get roll from right vector
		const TYPE dot = clamp(flatForward.dot(right), (TYPE)-1, (TYPE)1);
		roll = rad2deg(std::acos(dot));
		Vec3<TYPE> tmp = Vec3<TYPE>::crossProduct(flatForward, right);
		if (tmp.dot(forward) < 0)
			roll = -roll;
	}

	Vec3<TYPE> getEulerAngles() const
	{
		Vec3<TYPE> angles;
		getEulerAngles(angles[0], angles[1], angles[2]);
		return angles;
	}

	void setEulerAngles(TYPE yaw, TYPE pitch, TYPE roll)
	{
		Vec3<TYPE> localX = Vec3<TYPE>::UNIT_X;
		Vec3<TYPE> localY = Vec3<TYPE>::UNIT_Y;
		Vec3<TYPE> localZ = Vec3<TYPE>::UNIT_Z;
		localX.rotate(yaw, 2);
		localY.rotate(yaw, 2);
		localY.rotate(pitch, localX);
		localZ.rotate(pitch, localX);
		localX.rotate(roll, localY);
		localZ.rotate(roll, localY);

		setTransformation(localX, localY, localZ);
	}

	void setEulerAngles(const Vec3<TYPE>& angles)
	{
		setEulerAngles(angles[0], angles[1], angles[2]);
	}

	Mat3& operator=(const Mat3& source)
	{
		set(source);
		return *this;
	}

	Mat3 operator*(const Mat3& mat) const				{ return mul(mat); }
	Vec3<TYPE> operator*(const Vec3<TYPE>& vec) const	{ return mul(vec); }

	inline const TYPE& operator[](size_t d) const	{ return data[d]; }
	inline TYPE& operator[](size_t d)				{ return data[d]; }
	inline const TYPE* operator*() const			{ return data; }
	inline TYPE* operator*()						{ return data; }

public:
	static const Mat3 IDENTITY;
	static const Mat3 ZERO;
};

template<typename TYPE> const Mat3<TYPE> Mat3<TYPE>::IDENTITY;
template<typename TYPE> const Mat3<TYPE> Mat3<TYPE>::ZERO(0);


#undef FORi
#undef FOR_ALL


typedef Mat3<float> Mat3f;
