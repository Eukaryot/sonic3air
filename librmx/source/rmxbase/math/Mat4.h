/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

template<typename TYPE> class Vec3;
template<typename TYPE> class Vec4;
template<typename TYPE> class Mat3;


// Shortcut for a for-loop over all components
#define FORi(_statement_) \
	{ for (int i = 0; i < COLS; ++i) { _statement_; } }

#define FOR_ALL(_statement_) \
	{ for (int i = 0; i < COLS * ROWS; ++i) { _statement_; } }



template<typename TYPE> class Mat4
{
public:
	union
	{
		// Stored as a row-major matrix:
		//  (  [0]  [1]  [2]  [3] )    <->    ( m11 m12 m13 m14 )
		//  (  [4]  [5]  [6]  [7] )    <->    ( m21 m22 m23 m24 )
		//  (  [8]  [9] [10] [11] )    <->    ( m31 m32 m33 m34 )
		//  ( [12] [13] [14] [15] )    <->    ( m41 m42 m43 m44 )

		TYPE data[16];
		struct { TYPE m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44; };
	};

	enum class Initialization { NONE };
	static const Initialization Uninitialized = Initialization::NONE;

	static constexpr int COLS = 4;
	static constexpr int ROWS = 4;

public:
	Mat4()								{ setIdentity(); }
	explicit Mat4(Initialization)		{}
	explicit Mat4(TYPE value)			{ FOR_ALL(data[i] = value); }

	Mat4(const Mat4& source)			{ FOR_ALL(data[i] = source.data[i]); }
	explicit Mat4(const TYPE* mat)		{ FOR_ALL(data[i] = mat[i]); }

	explicit Mat4(const Mat3<TYPE>& source)
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

	void set(const Mat4& source)		{ FOR_ALL(data[i] = source.data[i]); }
	void set(const Mat3<TYPE>& source)	{ *this = Mat4(source); }
	void set(const TYPE* mat)			{ FOR_ALL(data[i] = mat[i]); }

	void setZero()
	{
		FOR_ALL(data[i] = 0);
	}

	void setIdentity()
	{
		FOR_ALL(data[i] = 0);
		for (int i = 0; i < ROWS; ++i)
		{
			set(i, i, 1);
		}
	}

	TYPE get(int col, int row) const		{ return data[col + row * COLS]; }
	TYPE& access(int col, int row)			{ return data[col + row * COLS]; }
	void set(int col, int row, TYPE value)	{ data[col + row * COLS] = value; }

	Vec4<TYPE> getRow(int num) const
	{
		Vec4<TYPE> output;
		FORi(output.data[i] = get(i, num));
		return output;
	}

	Vec4<TYPE> getColumn(int num) const
	{
		Vec4<TYPE> output;
		FORi(output.data[i] = get(num, i));
		return output;
	}

	void setRow(int num, const Vec4<TYPE>& input)
	{
		FORi(set(i, num, input.data[i]));
	}

	void setRow(int num, const Vec3<TYPE>& input)
	{
		for (int i = 0; i < 3; ++i)
		{
			set(i, num, input.data[i]);
		}
	}

	void setColumn(int num, const Vec4<TYPE>& input)
	{
		FORi(set(num, i, input.data[i]));
	}

	void setColumn(int num, const Vec3<TYPE>& input)
	{
		for (int i = 0; i < 3; ++i)
		{
			set(num, i, input.data[i]);
		}
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

	Mat4 transposed() const
	{
		Mat4 result(Uninitialized);
		for (int col = 0; col < COLS; ++col)
		{
			for (int row = 0; row < ROWS; ++row)
			{
				result.set(col, row, get(row, col));
			}
		}
		return result;
	}

	inline void add(const Mat4& source)	{ FOR_ALL(data[i] += source.data[i]); }
	inline void sub(const Mat4& source)	{ FOR_ALL(data[i] -= source.data[i]); }
	inline void mul(TYPE factor)		{ FOR_ALL(data[i] *= factor); }

	Vec4<TYPE> mul(const Vec4<TYPE>& vec) const
	{
		Vec4<TYPE> output(Vec4<TYPE>::Uninitialized);
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

	Vec3<TYPE> mul(const Vec3<TYPE>& vec, TYPE w) const
	{
		Vec3<TYPE> output(Vec3<TYPE>::Uninitialized);
		for (int row = 0; row < 3; ++row)
		{
			TYPE value = get(3, row) * w;
			for (int col = 0; col < 3; ++col)
			{
				value += get(col, row) * vec.data[col];
			}
			output.data[row] = value;
		}
		return output;
	}

	Mat4 mul(const Mat4& source) const
	{
		Mat4 result(Uninitialized);
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

	void setTranslation(const Vec3<TYPE>& translate)
	{
		setIdentity();
		for (int row = 0; row < 3; ++row)
		{
			set(3, row, translate.data[row]);
		}
	}

	void setScale(TYPE scale)
	{
		setZero();
		for (int i = 0; i < 3; ++i)
		{
			set(i, i, scale);
		}
		set(3, 3, (TYPE)1);
	}

	void setScale(const Vec3<TYPE>& scale)
	{
		setZero();
		for (int i = 0; i < 3; ++i)
		{
			set(i, i, scale);
		}
		set(3, 3, (TYPE)1);
	}

	void setScale(const Vec4<TYPE>& scale)
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
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		data[0] = _cos;	data[1] = -_sin;
		data[4] = _sin;	data[4+1] = _cos;
	}

	void setRotation3D(TYPE angle, const Vec3<TYPE>& axis_)
	{
		// TODO: This needs review / testing
		setIdentity();
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		Vec3<TYPE> axis = axis_;
		axis.normalize();
		data[0]  = axis.data[0] * axis.data[0] * (1.0f - _cos) + _cos;
		data[1]  = axis.data[0] * axis.data[1] * (1.0f - _cos) - axis.data[2] * _sin;
		data[2]  = axis.data[0] * axis.data[2] * (1.0f - _cos) + axis.data[1] * _sin;
		data[4]  = axis.data[0] * axis.data[1] * (1.0f - _cos) + axis.data[2] * _sin;
		data[5]  = axis.data[1] * axis.data[1] * (1.0f - _cos) + _cos;
		data[6]  = axis.data[1] * axis.data[2] * (1.0f - _cos) - axis.data[0] * _sin;
		data[8]  = axis.data[0] * axis.data[2] * (1.0f - _cos) - axis.data[1] * _sin;
		data[9]  = axis.data[1] * axis.data[2] * (1.0f - _cos) + axis.data[0] * _sin;
		data[10] = axis.data[2] * axis.data[2] * (1.0f - _cos) + _cos;
	}

	void setRotation3D(TYPE angle, int axis)
	{
		// TODO: This needs review / testing
		setIdentity();
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		if (axis == 0)
		{
			data[4+1]   = _cos;
			data[4+2]   = -_sin;
			data[4*2+1] = +_sin;
			data[4*2+2] = _cos;
		}
		else if (axis == 1)
		{
			data[0]     = _cos;
			data[2]     = +_sin;
			data[4*2]   = -_sin;
			data[4*2+2] = _cos;
		}
		else
		{
			data[0]   = _cos;
			data[1]   = -_sin;
			data[4]   = +_sin;
			data[4+1] = _cos;
		}
	}

	void setTransformation(const Vec3<TYPE>& translate, const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setIdentity();
		for (int row = 0; row < 3; ++row)
		{
			set(0, row, dir0.data[row]);
			set(1, row, dir1.data[row]);
			set(2, row, dir2.data[row]);
			set(3, row, translate.data[row]);
		}
	}

	void setTransformation(const Vec3<TYPE>& translate, const Mat3<TYPE>& mat)
	{
		*this = Mat4(mat);
		for (int row = 0; row < 3; ++row)
		{
			set(3, row, translate.data[row]);
		}
	}

	void setTransformationInv(const Vec3<TYPE>& translate, const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setIdentity();
		for (int col = 0; col < 3; ++col)
		{
			set(col, 0, dir0.data[col]);
			set(col, 0, dir1.data[col]);
			set(col, 0, dir2.data[col]);
		}
		set(3, 0, -translate.dot(dir0));
		set(3, 1, -translate.dot(dir1));
		set(3, 2, -translate.dot(dir2));
	}

	void setTransformationInv(const Vec3<TYPE>& translate, const Mat3<TYPE>& mat)
	{
		setTransformationInv(translate, mat.getRow(0), mat.getRow(1), mat.getRow(2));
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
		Vec3<TYPE> out0(dir0);
		out0.normalize();
		Vec3<TYPE> out1(dir1);
		Vec3<TYPE> out2 = Vec3<TYPE>::crossProduct(out0, out1);
		if (out2.sqrLen() == 0)
		{
			out1 = Vec3f::UNIT_Z;
			out2 = Vec3<TYPE>::crossProduct(out0, out1);
			if (out2.sqrLen() == 0)
			{
				out1 = Vec3f::UNIT_Y;
				out2 = Vec3<TYPE>::crossProduct(out0, out1);
			}
		}
		out2.normalize();
		out1 = Vec3<TYPE>::crossProduct(out2, out0);
		for (int i = 0; i < 4; ++i)
		{
			data[i*4+0] = out0.data[i];
			data[i*4+1] = out1.data[i];
			data[i*4+2] = out2.data[i];
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

		setIdentity();
		setTransformation(localX, localY, localZ);
	}

	void setEulerAngles(const Vec3<TYPE>& angles)
	{
		setEulerAngles(angles[0], angles[1], angles[2]);
	}

	Mat4& operator=(const Mat4& source)
	{
		set(source);
		return *this;
	}

	Mat4 operator*(const Mat4& mat) const				{ return mul(mat); }
	Vec4<TYPE> operator*(const Vec4<TYPE>& vec) const	{ return mul(vec); }
	Vec3<TYPE> operator*(const Vec3<TYPE>& vec) const	{ return mul(vec, 0.0f); }

	inline const TYPE& operator[](size_t d) const	{ return data[d]; }
	inline TYPE& operator[](size_t d)				{ return data[d]; }
	inline const TYPE* operator*() const			{ return data; }
	inline TYPE* operator*()						{ return data; }

public:
	static const Mat4 IDENTITY;
	static const Mat4 ZERO;
};

template<typename TYPE> const Mat4<TYPE> Mat4<TYPE>::IDENTITY;
template<typename TYPE> const Mat4<TYPE> Mat4<TYPE>::ZERO(0);


#undef FORi
#undef FOR_ALL


typedef Mat4<float> Mat4f;
