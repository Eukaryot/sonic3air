/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace math
{

	bool intersectRayWithPlane(const Ray& ray, const Plane& plane, Vec3f* outIntersectionPoint, float* outIntersectionT)
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

	bool intersectLineWithPlane(const Line& line, const Plane& plane, Vec3f* outIntersectionPoint, float* outIntersectionT)
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

		// Calculate offset t along line where intersection happens
		const float t = -distance / prox;

		// We have an intersection
		if (outIntersectionPoint)
			*outIntersectionPoint = line.getPoint(t);
		if (outIntersectionT)
			*outIntersectionT = t;
		return true;
	}

	bool nearestPointBetweenLines(const Line& line1, const Line& line2, Vec3f* outPoint, float* outT)
	{
		// Vector orthogonal to each line, this is also a direction vector along the connection of the closest points
		Vec3f connectionNormal;
		connectionNormal.cross(line1.getDirection(), line2.getDirection());

		if (connectionNormal.sqrLen() == 0.0f)
		{
			// The lines are parallel
			return false;
		}

		// Now we construct a plane that contains line2 AND the connection vector.
		//  This plane will also contain the closest point on line1.
		Vec3f planeNormal;
		planeNormal.cross(connectionNormal, line2.getDirection());
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
