/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace math
{
	namespace detail
	{
		bool rayBoxIntersectionInAxis(float& outMinT, float& outMaxT, float rayStart, float rayDir, float boxMin, float boxMax)
		{
			if (rayDir != 0.0f)
			{
				outMinT = (boxMin - rayStart) / rayDir;
				outMaxT = (boxMax - rayStart) / rayDir;
				if (outMinT > outMaxT)
					std::swap(outMinT, outMaxT);
				if (outMaxT < 0.0f)
					return false;
				outMinT = std::max(outMinT, 0.0f);
				return true;
			}
			else if (rayStart >= boxMin && rayStart <= boxMax)
			{
				outMinT = -FLT_MAX;
				outMaxT = FLT_MAX;
				return true;
			}
			else
			{
				return false;
			}
		}
	}


	bool intersectRayWithPlane(const Ray& ray, const Plane& plane, Vec3f* outIntersectionPoint, float* outIntersectionT, bool onlyFromFront)
	{
		// Distance of ray's origin to plane along plane normal
		const float distance = plane.getDistance(ray.getOrigin());

		// Length of ray's direction vector projected to plane normal (= "speed" of ray towards plane)
		const float prox = plane.getNormal().dot(ray.getDirection());

		// Ray is parallel or inside plane?
		if (prox == 0.0f)
		{
			// Inside if dot1 is zero
			if (distance == 0.0f)
			{
				if (outIntersectionPoint)
					*outIntersectionPoint = ray.getOrigin();
				if (outIntersectionT)
					*outIntersectionT = 0.0f;
				return true;
			}

			// They are parallel
			return false;
		}

		// Check if plane is hit from front or back side
		if (onlyFromFront && prox > 0.0f)
			return false;

		// Calculate offset t along ray where intersection happens
		const float t = -distance / prox;

		// Intersection point is not on ray, but "behind" it?
		if (t < 0.0f)
		{
			return false;
		}

		// We have an intersection
		if (outIntersectionPoint)
			*outIntersectionPoint = ray.getPoint(t);
		if (outIntersectionT)
			*outIntersectionT = t;
		return true;
	}

	bool intersectLineWithPlane(const Line& line, const Plane& plane, Vec3f* outIntersectionPoint, float* outIntersectionT, bool onlyFromFront)
	{
		// Distance of line's origin to plane along plane normal
		const float distance = plane.getDistance(line.getOrigin());

		// Length of line's direction vector projected to plane normal (= "speed" of line towards plane)
		const float prox = plane.getNormal().dot(line.getDirection());

		// Line is parallel or inside plane?
		if (prox == 0.0f)
		{
			// Inside if dot1 is zero
			if (distance == 0.0f)
			{
				if (outIntersectionPoint)
					*outIntersectionPoint = line.getOrigin();
				if (outIntersectionT)
					*outIntersectionT = 0.0f;
				return true;
			}

			// They are parallel
			return false;
		}

		// Check if plane is hit from front or back side
		if (onlyFromFront && prox > 0.0f)
			return false;

		// Calculate offset t along line where intersection happens
		const float t = -distance / prox;

		// We have an intersection
		if (outIntersectionPoint)
			*outIntersectionPoint = line.getPoint(t);
		if (outIntersectionT)
			*outIntersectionT = t;
		return true;
	}

	bool intersectRayWithBox(const math::Ray& ray, const Box3f& box, float* outIntersectionT0, float* outIntersectionT1)
	{
		const Vec3f rayStart = ray.getOrigin();
		const Vec3f rayDirection = ray.getDirection();

		// Check intersection in x-direction
		float t0, t1;
		if (!detail::rayBoxIntersectionInAxis(t0, t1, rayStart.x, rayDirection.x, box.mMin.x, box.mMax.x))
			return false;

		// Check intersection in y-direction
		float s0, s1;
		if (!detail::rayBoxIntersectionInAxis(s0, s1, rayStart.y, rayDirection.y, box.mMin.y, box.mMax.y))
			return false;

		t0 = std::max(t0, s0);
		t1 = std::min(t1, s1);
		if (t1 <= t0)
			return false;

		// Check intersection in z-direction
		if (!detail::rayBoxIntersectionInAxis(s0, s1, rayStart.z, rayDirection.z, box.mMin.z, box.mMax.z))
			return false;

		t0 = std::max(t0, s0);
		t1 = std::min(t1, s1);
		if (t1 <= t0)
			return false;

		// There is an intersection, namely in the interval [t0, t1]
		if (nullptr != outIntersectionT0)
			*outIntersectionT0 = t0;
		if (nullptr != outIntersectionT1)
			*outIntersectionT1 = t1;
		return true;
	}

	bool nearestPointBetweenLines(const Line& line1, const Line& line2, Vec3f* outPoint, float* outT)
	{
		// Vector orthogonal to each line, this is also a direction vector along the connection of the closest points
		Vec3f connectionNormal = Vec3f::crossProduct(line1.getDirection(), line2.getDirection());

		if (connectionNormal.sqrLen() == 0.0f)
		{
			// The lines are parallel
			return false;
		}

		// Now we construct a plane that contains line2 AND the connection vector.
		//  This plane will also contain the closest point on line1.
		Vec3f planeNormal = Vec3f::crossProduct(connectionNormal, line2.getDirection());
		const Plane plane(planeNormal, line2.getOrigin());

		// the resulting point is on the recently constructed plane, and on line1, so we just need to intersect them
		return intersectLineWithPlane(line1, plane, outPoint, outT);
	}

	Vec3f nearestPointOnLine(const Vec3f& point, const Line& line)
	{
		// Construct plane containing point with line's direction vector as normal
		const Plane plane(line.getDirection(), point);

		// Then intersect line with that plane
		Vec3f outPoint;
		if (!intersectLineWithPlane(line, plane, &outPoint))
			assert(false);

		return outPoint;
	}

}
