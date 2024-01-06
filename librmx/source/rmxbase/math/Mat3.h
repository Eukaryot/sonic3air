/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Shortcut for a for-loop over all components
#define FORi(_statement_) \
	{ for (int i = 0; i < 3; ++i) { _statement_; } }

#define FORi2(_statement_) \
	{ for (int i = 0; i < 9; ++i) { _statement_; } }

#define FORij(_statement_) \
	{ for (int i = 0; i < 3; ++i) { for (int j = 0; j < 3; ++j) { _statement_; } } }



template<typename TYPE> class Mat3
{
public:
	union
	{
		TYPE data[9];
		struct { TYPE m11, m12, m13, m21, m22, m23, m31, m32, m33; };
	};

public:
	Mat3()							{ setIdentity(); }
	explicit Mat3(bool undefined)	{}
	explicit Mat3(TYPE value)		{ FORi2(data[i] = value); }

	Mat3(const TYPE* mat)			{ FORi2(data[i] = mat[i]); }
	Mat3(const Mat3& source)		{ FORi2(data[i] = source.data[i]); }

	void set(const TYPE* mat)		{ FORi2(data[i] = mat[i]); }
	void set(const Mat3& source)	{ FORi2(data[i] = source.data[i]); }

	void setZero()
	{
		FORi2(data[i] = 0);
	}

	void setIdentity()
	{
		FORi2(data[i] = 0);
		for (int i = 0; i < 3; ++i)
			data[i*(3+1)] = 1;
	}

	void transpose()
	{
		for (int i = 0; i < 3-1; ++i)
			for (int j = i+1; j < 3; ++j)
			{
				TYPE temp = data[i+j*3];
				data[i+j*3] = data[j+i*3];
				data[j+i*3] = temp;
			}
	}

	void transpose(const Mat3& source)
	{
		if (&source == this)
		{
			transpose();
			return;
		}
		FORij(data[i*3+j] = source.data[i+j*3]);
	}

	void add(const Mat3& source)	{ FORi2(data[i] += source.data[i]); }
	void sub(const Mat3& source)	{ FORi2(data[i] -= source.data[i]); }
	void mul(TYPE factor)			{ FORi2(data[i] *= factor); }

	Vec3<TYPE> mul(const Vec3<TYPE>& vec) const
	{
		Vec3<TYPE> output;
		FORij(output.data[i] += data[i*3+j] * vec.data[j]);
		return output;
	}

	Mat3 mul(const Mat3& source) const
	{
		Mat3 result((TYPE)0);
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				for (int k = 0; k < 3; ++k)
					result.data[j+i*3] += data[k+i*3] * source.data[j+k*3];
		return result;
	}

	void mul(const Mat3& source1, const Mat3& source2)
	{
		setZero();
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				for (int k = 0; k < 3; ++k)
					data[j+i*3] += source1.data[k+i*3] * source2.data[j+k*3];
	}

	Vec3<TYPE> getRow(int num) const
	{
		Vec3<TYPE> output;
		FORi(output.data[i] = data[i*3+num]);
		return output;
	}

	Vec3<TYPE> getLine(int num) const
	{
		Vec3<TYPE> output;
		FORi(output.data[i] = data[num*3+i]);
		return output;
	}

	void setRow(const Vec3<TYPE>& input, int num)
	{
		FORi(data[i*3+num] = input.data[i]);
	}

	void setLine(const Vec3<TYPE>& input, int num)
	{
		FORi(data[num*3+i] = input.data[i]);
	}

	void setScale(TYPE scale)
	{
		setZero();
		FORi(data[i*(3+1)] = scale);
	}

	void setScale(const Vec3<TYPE>& scale)
	{
		setZero();
		FORi(data[i*(3+1)] = scale.data[i]);
	}

	void setRotation2D(TYPE angle)
	{
		setIdentity();
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		data[0] = _cos;	data[1] = -_sin;
		data[3] = _sin;	data[3+1] = _cos;
	}

	void setRotation3D(TYPE angle, const Vec3<TYPE>& axis_)
	{
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
		setIdentity();
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		if (axis == 0)
		{
			data[3+1]   = _cos;
			data[3+2]   = -_sin;
			data[3*2+1] = +_sin;
			data[3*2+2] = _cos;
		}
		else if (axis == 1)
		{
			data[0]       = _cos;
			data[2]       = +_sin;
			data[3*2]   = -_sin;
			data[3*2+2] = _cos;
		}
		else
		{
			data[0]     = _cos;
			data[1]     = -_sin;
			data[3]   = +_sin;
			data[3+1] = _cos;
		}
	}

	void setTransformation(const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setIdentity();
		for (int i = 0; i < 3; ++i)
		{
			data[i*3+0] = dir0.data[i];
			data[i*3+1] = dir1.data[i];
			data[i*3+2] = dir2.data[i];
		}
	}

	void setTransformationInv(const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setIdentity();
		for (int i = 0; i < 3; ++i)
		{
			data[i]       = dir0.data[i];
			data[3+i]   = dir1.data[i];
			data[2*3+i] = dir2.data[i];
		}
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
		Vec3<TYPE> out2;
		out2.cross(out0, out1);
		if (out2.sqrLen() == 0)
		{
			out1 = Vec3<TYPE>::UNIT_Z;
			out2.cross(out0, out1);
			if (out2.sqrLen() == 0)
			{
				out1 = Vec3<TYPE>::UNIT_Y;
				out2.cross(out0, out1);
			}
		}
		out2.normalize();
		out1.cross(out2, out0);
		for (int i = 0; i < 3; ++i)
		{
			data[i*3+0] = out0.data[i];
			data[i*3+1] = out1.data[i];
			data[i*3+2] = out2.data[i];
		}
	}

	void setEulerAngles(const Vec3<TYPE>& angles)
	{
		setEulerAngles(angles[0], angles[1], angles[2]);
	}

	void setEulerAngles(TYPE alpha, TYPE beta, TYPE gamma)
	{
		setIdentity();
		Vec3<TYPE> out0 = Vec3<TYPE>::UNIT_X;
		Vec3<TYPE> out1 = Vec3<TYPE>::UNIT_Y;
		Vec3<TYPE> out2 = Vec3<TYPE>::UNIT_Z;
		out0.rotate(alpha, 2);
		out1.rotate(alpha, 2);
		out1.rotate(beta, out0);
		out2.rotate(beta, out0);
		out0.rotate(gamma, out1);
		out2.rotate(gamma, out1);
		for (int i = 0; i < 3; ++i)
		{
			data[i*3+0] = out0.data[i];
			data[i*3+1] = out1.data[i];
			data[i*3+2] = out2.data[i];
		}
	}

	void getEulerAngles(Vec3<TYPE>& angles) const
	{
		getEulerAngles(angles[0], angles[1], angles[2]);
	}

	void getEulerAngles(TYPE& alpha, TYPE& beta, TYPE& gamma) const
	{
		Vec3<TYPE> in1(data[0], data[3+0], data[3*2+0]);
		Vec3<TYPE> in2(data[1], data[3+1], data[3*2+1]);
		alpha = atan2(in2.data[1], in2.data[0]) - (TYPE)PI_DOUBLE / 2;
		if (alpha < 0)
			alpha += (TYPE)PI_DOUBLE * 2;
		Vec3<TYPE> gam0(cos(alpha), sin(alpha), 0);
		alpha = rad2deg(alpha);
		beta = rad2deg(asin(in2[2]));
		TYPE dot = gam0.dot(in1);
		dot = (dot < -1) ? -1 : ((dot > +1) ? +1 : dot);
		gamma = rad2deg(acos(dot));
		Vec3<TYPE> tmp;
		tmp.cross(gam0, in1);
		if (tmp.dot(in2) < 0)
			gamma = -gamma;
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
#undef FORi2
#undef FORij


typedef Mat3<float> Mat3f;
