/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Line
*		A line in 3D space.
*/

#pragma once


namespace math
{

	class API_EXPORT Line
	{
	public:
		static Line fromOriginAndDirection(const Vec3f& origin, const Vec3f& direction)	{ return Line(origin, direction); }
		static Line fromTwoPoints(const Vec3f& origin, const Vec3f& target)				{ return Line(origin, target - origin); }

		Line() : mDirection(0,0,1) {}

		void setOrigin(const Vec3f& origin)		{ mOrigin = origin; }
		const Vec3f& getOrigin(void) const		{ return mOrigin; }

		void setDirection(const Vec3f& dir)		{ mDirection = dir; }
		const Vec3f& getDirection(void) const	{ return mDirection; }

		Vec3f getPoint(float t) const
		{
			return mOrigin + (mDirection * t);
		}

	protected:
		Line(const Vec3f& origin, const Vec3f& direction) : mOrigin(origin), mDirection(direction) {}

	protected:
		Vec3f mOrigin;
		Vec3f mDirection;
	};

}
