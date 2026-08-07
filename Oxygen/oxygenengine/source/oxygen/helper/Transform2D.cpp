/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Transform2D.h"


namespace
{
	void getSinCos(float angle, float& sine, float& cosine)
	{
		// Make sure to use precise values when close to a multiple of 90°
		const float multiple = angle / (PI_FLOAT * 0.5f);
		if (std::abs(multiple - roundToFloat(multiple)) < 0.001f)
		{
			// Make it an int value between 0 and 3, representing 0° to 270°
			const int multipleInt = roundToInt(multiple) & 3;
			sine   = (float)std::min(multipleInt, 2 - multipleInt);		// 0, 1, 0, -1
			cosine = (float)std::max(1 - multipleInt, multipleInt - 3);	// 1, 0, -1, 0
		}
		else
		{
			sine   = std::sin(angle);
			cosine = std::cos(angle);
		}
	}
}


bool Transform2D::hasNontrivialRotationOrScale() const
{
	// Check if any component is anything different than -1.0f, 0.0f, or 1.0f
	return (mMatrix[0] != roundToFloat(mMatrix[0]) ||
			mMatrix[1] != roundToFloat(mMatrix[1]) ||
			mMatrix[2] != roundToFloat(mMatrix[2]) ||
			mMatrix[3] != roundToFloat(mMatrix[3]));
}

void Transform2D::setIdentity()
{
	mMatrix.set(1.0f, 0.0f, 0.0f, 1.0f);
	mInverse.set(1.0f, 0.0f, 0.0f, 1.0f);
}

void Transform2D::setRotationByAngle(float angle)
{
	float sine;
	float cosine;
	::getSinCos(angle, sine, cosine);
	mMatrix[0] =  cosine;
	mMatrix[1] = -sine;
	mMatrix[2] =  sine;
	mMatrix[3] =  cosine;
	mInverse[0] =  cosine;
	mInverse[1] =  sine;
	mInverse[2] = -sine;
	mInverse[3] =  cosine;
}

void Transform2D::setRotationAndScale(float angle, Vec2f scale)
{
	setRotationByAngle(angle);
	mMatrix[0] *= scale.x;
	mMatrix[1] *= scale.y;
	mMatrix[2] *= scale.x;
	mMatrix[3] *= scale.y;
	mInverse[0] /= scale.x;
	mInverse[1] /= scale.x;
	mInverse[2] /= scale.y;
	mInverse[3] /= scale.y;
}

void Transform2D::setByMatrix(float a, float b, float c, float d)
{
	mMatrix[0] = a;
	mMatrix[1] = b;
	mMatrix[2] = c;
	mMatrix[3] = d;

	// Calculate the inverse
	const float determinant = a * d - b * c;
	mInverse[0] =  d / determinant;
	mInverse[1] = -b / determinant;
	mInverse[2] = -c / determinant;
	mInverse[3] =  a / determinant;
}

void Transform2D::applyScale(float scale)
{
	const float inverseScale = 1.0f / scale;
	mMatrix[0] *= scale;
	mMatrix[1] *= scale;
	mMatrix[2] *= scale;
	mMatrix[3] *= scale;
	mInverse[0] *= inverseScale;
	mInverse[1] *= inverseScale;
	mInverse[2] *= inverseScale;
	mInverse[3] *= inverseScale;
}

void Transform2D::flipX()
{
	mMatrix[0] = -mMatrix[0];
	mMatrix[2] = -mMatrix[2];
	mInverse[0] = -mInverse[0];
	mInverse[1] = -mInverse[1];
}

void Transform2D::flipY()
{
	mMatrix[1] = -mMatrix[1];
	mMatrix[3] = -mMatrix[3];
	mInverse[2] = -mInverse[2];
	mInverse[3] = -mInverse[3];
}

Vec2f Transform2D::transformVector(const Vec2f input) const
{
	return Vec2f(mMatrix[0] * input.x + mMatrix[1] * input.y, mMatrix[2] * input.x + mMatrix[3] * input.y);
}
