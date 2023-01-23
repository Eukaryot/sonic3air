/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputManager.h"


class DynamicSprites
{
public:
	void updateSpriteRedirects();

private:
	InputManager::InputType mLastInputType = InputManager::InputType::NONE;
	uint32 mLastMappingsChangeCounter = 0;
	uint32 mLastSpriteCacheChangeCounter = 0;
};
