/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/Renderer.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/simulation/LogDisplay.h"


bool Renderer::isUsingSpriteMask(const std::vector<Geometry*>& geometries) const
{
	for (const Geometry* geometry : geometries)
	{
		if (geometry->getType() == Geometry::Type::SPRITE && geometry->as<SpriteGeometry>().mSpriteInfo.getType() == RenderItem::Type::SPRITE_MASK)
			return true;
	}
	return false;
}

void Renderer::startRendering()
{
	mRenderingStartTicks = SDL_GetTicks();
	mRenderingRunningCount = 0;
	mLoggedLimitWarning = false;
}

bool Renderer::progressRendering()
{
	// Check if rendering is taking too long
	++mRenderingRunningCount;
	if ((mRenderingRunningCount % 100) == 0)
	{
		constexpr uint32 LIMIT_MILLISECONDS = 100;
		const uint32 numTicks = SDL_GetTicks() - mRenderingStartTicks;
		if (numTicks >= LIMIT_MILLISECONDS)
		{
			if (!mLoggedLimitWarning)
			{
				LogDisplay::instance().setLogDisplay("Warning: Exceeded the render time limit of "  + std::to_string(LIMIT_MILLISECONDS) + " ms, further render items will be ignored");
				mLoggedLimitWarning = true;
			}
			return false;
		}
	}
	return true;
}
