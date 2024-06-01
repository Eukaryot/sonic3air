/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace math
{

	class Plane
	{
	public:
		enum class Side
		{
			NO_SIDE,
			POSITIVE_SIDE,
			NEGATIVE_SIDE,
			BOTH_SIDE
		};

	public:
		Plane()
			: mNormal(0,0,1), mDistance(0)
		{}

		Plane(const Plane& plane)
			: mNormal(plane.mNormal), mDistance(plane.mDistance)
		{}

		Plane(float a, float b, float c, float d)
			: mNormal(a,b,c), mDistance(d)
		{}

		Plane(const Vec3f& normal, const Vec3f& point)
		{
			redefine(normal, point);
		}

		Plane(const Vec3f& point0, const Vec3f& point1, const Vec3f& point2)
		{
			redefine(point0, point1, point2);
		}

		inline const Vec3f& getNormal() const	{ return mNormal; }
		inline float getDistance() const		{ return mDistance; }

		void redefine(const Vec3f& point0, const Vec3f& point1, const Vec3f& point2)
		{
			mNormal = Vec3f::crossProduct(point1 - point0, point2 - point0);
			redefine(mNormal.normalized(), point0);
		}

		void redefine(const Vec3f& normal, const Vec3f& point)
		{
			mNormal = normal;
			mDistance = -mNormal.dot(point);
		}

		float getDistance(const Vec3f& point) const
		{
			return mNormal.dot(point) + mDistance;
		}

		Side getSide(const Vec3f& point) const
		{
			const float fDistance = getDistance(point);
			if (fDistance < 0.0)
				return Side::NEGATIVE_SIDE;
			else if (fDistance > 0.0)
				return Side::POSITIVE_SIDE;
			else
				return Side::NO_SIDE;
		}

		float normalize()
		{
			float fLength = mNormal.length();
			if (fLength > 0.0f)
			{
				float fInvLength = 1.0f / fLength;
				mNormal *= fInvLength;
				mDistance *= fInvLength;
			}

			return fLength;
		}

		bool operator==(const Plane& rhs) const
		{
			return (rhs.mDistance == mDistance && rhs.mNormal == mNormal);
		}

		bool operator!=(const Plane& rhs) const
		{
			return (rhs.mDistance != mDistance || rhs.mNormal != mNormal);
		}

	private:
		Vec3f mNormal;
		float mDistance;
	};

}
