/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct Transform2D
{
public:
	inline Transform2D() : mMatrix(1.0f, 0.0f, 0.0f, 1.0f), mInverse(1.0f, 0.0f, 0.0f, 1.0f) {}

	inline bool isIdentity() const  { return (mMatrix == Vec4f(1.0f, 0.0f, 0.0f, 1.0f)); }
	bool hasNontrivialRotationOrScale() const;	// Checks for scale and rotation that is not just a multiple of 90°

	void setIdentity();
	void setRotationByAngle(float angle);
	void setRotationAndScale(float angle, Vec2f scale);
	void setByMatrix(float a, float b, float c, float d);

	void applyScale(float scale);
	void flipX();
	void flipY();

	Vec2f transformVector(const Vec2f input) const;

public:
	Vec4f mMatrix;
	Vec4f mInverse;
};
