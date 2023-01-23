/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace math
{

	class Ray : public Line
	{
	public:
		Ray() : Line() {}
		Ray(const Vec3f& origin, const Vec3f& direction) : Line(origin, direction) {}
	};

}
