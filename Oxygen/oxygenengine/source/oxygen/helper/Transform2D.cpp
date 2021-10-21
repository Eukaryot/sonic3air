/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Transform2D.h"


bool Transform2D::hasRotationOrScale() const
{
	const float d11 = std::abs(std::abs(mMatrix[0]) - 1.0f);
	const float d12 = std::abs(mMatrix[1]);
	const float d21 = std::abs(mMatrix[2]);
	const float d22 = std::abs(std::abs(mMatrix[3]) - 1.0f);
	return (d11 + d12 + d21 + d22) > 0.001f;
}

void Transform2D::setIdentity()
{
	mMatrix.set(1.0f, 0.0f, 0.0f, 1.0f);
	mInverse.set(1.0f, 0.0f, 0.0f, 1.0f);
}

void Transform2D::setRotationByAngle(float angle)
{
	const float sin_ = std::sin(angle);
	const float cos_ = std::cos(angle);
	mMatrix[0] =  cos_;
	mMatrix[1] = -sin_;
	mMatrix[2] =  sin_;
	mMatrix[3] =  cos_;
	mInverse[0] =  cos_;
	mInverse[1] =  sin_;
	mInverse[2] = -sin_;
	mInverse[3] =  cos_;
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
