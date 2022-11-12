/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


// Shortcut for a for-loop over all components
#define FORi(_statement_) \
	{ for (int i = 0; i < 4; ++i) { _statement_; } }

#define FORi2(_statement_) \
	{ for (int i = 0; i < 16; ++i) { _statement_; } }

#define FORij(_statement_) \
	{ for (int i = 0; i < 4; ++i) { for (int j = 0; j < 4; ++j) { _statement_; } } }



template<typename TYPE> class Mat4
{
public:
	union
	{
		TYPE data[16];
		struct { TYPE m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44; };
	};

public:
	Mat4()							{ setIdentity(); }
	explicit Mat4(bool undefined)	{}
	explicit Mat4(TYPE value)		{ FORi2(data[i] = value); }

	Mat4(const TYPE* mat)			{ FORi2(data[i] = mat[i]); }
	Mat4(const Mat4& source)		{ FORi2(data[i] = source.data[i]); }

	void set(const TYPE* mat)	{ FORi2(data[i] = mat[i]); }
	void set(const Mat4& source)	{ FORi2(data[i] = source.data[i]); }

	void setZero()
	{
		FORi2(data[i] = 0);
	}

	void setIdentity()
	{
		FORi2(data[i] = 0);
		for (int i = 0; i < 4; ++i)
			data[i*(4+1)] = 1;
	}

	void transpose()
	{
		for (int i = 0; i < 4-1; ++i)
			for (int j = i+1; j < 4; ++j)
			{
				TYPE temp = data[i+j*4];
				data[i+j*4] = data[j+i*4];
				data[j+i*4] = temp;
			}
	}

	void transpose(const Mat4& source)
	{
		if (&source == this)
		{
			transpose();
			return;
		}
		FORij(data[i*4+j] = source.data[i+j*4]);
	}

	inline void add(const Mat4& source)	{ FORi2(data[i] += source.data[i]); }
	inline void sub(const Mat4& source)	{ FORi2(data[i] -= source.data[i]); }
	inline void mul(TYPE factor)		{ FORi2(data[i] *= factor); }

	Vec4<TYPE> mul(const Vec4<TYPE>& vec) const
	{
		Vec4<TYPE> output;
		FORij(output.data[i] += data[i*4+j] * vec.data[j]);
		return output;
	}

	Vec3<TYPE> mul(const Vec3<TYPE>& vec, TYPE w) const
	{
		Vec3<TYPE> output(false);
		for (int i = 0; i < 4-1; ++i)
		{
			output.data[i] = data[(i+1)*4-1] * w;
			for (int j = 0; j < 4-1; ++j)
				output.data[i] += data[i*4+j] * vec.data[j];
		}
		return output;
	}

	Mat4 mul(const Mat4& source) const
	{
		Mat4 result((TYPE)0);
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				for (int k = 0; k < 4; ++k)
					result.data[j+i*4] += data[k+i*4] * source.data[j+k*4];
		return result;
	}

	void mul(const Mat4& source1, const Mat4& source2)
	{
		setZero();
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				for (int k = 0; k < 4; ++k)
					data[j+i*4] += source1.data[k+i*4] * source2.data[j+k*4];
	}

	Vec4<TYPE> getRow(int num) const
	{
		Vec4<TYPE> output;
		FORi(output.data[i] = data[i*4+num]);
		return output;
	}

	Vec4<TYPE> getLine(int num) const
	{
		Vec4<TYPE> output;
		FORi(output.data[i] = data[num*4+i]);
		return output;
	}

	void setRow(const Vec4<TYPE>& input, int num)
	{
		FORi(data[i*4+num] = input.data[i]);
	}

	void setLine(const Vec4<TYPE>& input, int num)
	{
		FORi(data[num*4+i] = input.data[i]);
	}

	void setTranslation(const Vec3<TYPE>& translate)
	{
		setIdentity();
		for (int i = 0; i < 4-1; ++i)
			data[(i+1)*4-1] = translate.data[i];
	}

	void setScale(TYPE scale)
	{
		setZero();
		for (int i = 0; i < 4-1; ++i)
			data[i*(4+1)] = scale;
		data[4*4-1] = 1;
	}

	void setScale(const Vec4<TYPE>& scale)
	{
		setZero();
		FORi(data[i*(4+1)] = scale.data[i]);
	}

	void setScale(const Vec3<TYPE>& scale)
	{
		setZero();
		for (int i = 0; i < 4-1; ++i)
			data[i*(4+1)] = scale.data[i];
		data[4*4-1] = 1;
	}

	void setRotation2D(TYPE angle)
	{
		setIdentity();
		angle = deg2rad(angle);
		TYPE _cos = cos(angle);
		TYPE _sin = sin(angle);
		data[0] = _cos;	data[1] = -_sin;
		data[4] = _sin;	data[4+1] = _cos;
	}

	void setRotation3D(TYPE angle, const Vec3<TYPE>& axis_)
	{
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
		for (int i = 0; i < 4-1; ++i)
		{
			data[i*4+0] = dir0.data[i];
			data[i*4+1] = dir1.data[i];
			data[i*4+2] = dir2.data[i];
			data[i*4+3] = translate.data[i];
		}
	}

	void setTransformation(const Vec3<TYPE>& translate, const Mat3<TYPE>& mat)
	{
		setIdentity();
		for (int i = 0; i < 4-1; ++i)
		{
			data[i*4+0] = mat.data[i*(4-1)+0];
			data[i*4+1] = mat.data[i*(4-1)+1];
			data[i*4+2] = mat.data[i*(4-1)+2];
			data[i*4+3] = translate.data[i];
		}
	}

	void setTransformationInv(const Vec3<TYPE>& translate, const Vec3<TYPE>& dir0, const Vec3<TYPE>& dir1, const Vec3<TYPE>& dir2)
	{
		setIdentity();
		for (int i = 0; i < 4-1; ++i)
		{
			data[i]       = dir0.data[i];
			data[4+i]   = dir1.data[i];
			data[2*4+i] = dir2.data[i];
		}
		data[3]       = -translate.dot(dir0);
		data[4+3]   = -translate.dot(dir1);
		data[2*4+3] = -translate.dot(dir2);
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
		Vec3<TYPE> out2;
		out2.cross(out0, out1);
		if (out2.sqrLen() == 0)
		{
			out1 = Vec3f::UNIT_Z;
			out2.cross(out0, out1);
			if (out2.sqrLen() == 0)
			{
				out1 = Vec3f::UNIT_Y;
				out2.cross(out0, out1);
			}
		}
		out2.normalize();
		out1.cross(out2, out0);
		for (int i = 0; i < 4; ++i)
		{
			data[i*4+0] = out0.data[i];
			data[i*4+1] = out1.data[i];
			data[i*4+2] = out2.data[i];
		}
	}

	void setEulerAngles(const Vec3<TYPE>& angles)
	{
		setEulerAngles(angles[0], angles[1], angles[2]);
	}

	void setEulerAngles(TYPE alpha, TYPE beta, TYPE gamma)
	{
		setIdentity();
		Vec3<TYPE> out0 = Vec3f::UNIT_X;
		Vec3<TYPE> out1 = Vec3f::UNIT_Y;
		Vec3<TYPE> out2 = Vec3f::UNIT_Z;
		out0.rotate(alpha, 2);
		out1.rotate(alpha, 2);
		out1.rotate(beta, out0);
		out2.rotate(beta, out0);
		out0.rotate(gamma, out1);
		out2.rotate(gamma, out1);
		for (int i = 0; i < 4; ++i)
		{
			data[i*4+0] = out0.data[i];
			data[i*4+1] = out1.data[i];
			data[i*4+2] = out2.data[i];
		}
	}

	void getEulerAngles(Vec3<TYPE>& angles) const
	{
		getEulerAngles(angles[0], angles[1], angles[2]);
	}

	void getEulerAngles(TYPE& alpha, TYPE& beta, TYPE& gamma) const
	{
		Vec3<TYPE> in1(data[0], data[4+0], data[4*2+0]);
		Vec3<TYPE> in2(data[1], data[4+1], data[4*2+1]);
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
#undef FORi2
#undef FORij


typedef Mat4<float> Mat4f;
