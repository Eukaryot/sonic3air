/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/RenderResources.h"


void RenderResources::loadSpriteCache(bool fullReload)
{
	if (fullReload)
		mSpriteCache.clear();
	mSpriteCache.loadAllSpriteDefinitions();
}
