/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/resources/SpriteCache.h"


class RenderResources : public SingleInstance<RenderResources>
{
public:
	void loadSpriteCache(bool fullReload = false);

public:
	SpriteCache mSpriteCache;
};
