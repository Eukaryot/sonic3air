/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>

class Drawer;


class GameMenuControlsDisplay
{
public:
	void clear();
	void addControl(std::string_view displayText, bool alignRight, std::string_view spriteName);
	void addControl(std::string_view displayText, bool alignRight, std::string_view spriteName, std::string_view additionalSpriteName);

	void render(Drawer& drawer, float visibility = 1.0f);

private:
	struct Control
	{
		std::string mDisplayText;
		bool mAlignRight = false;
		std::vector<uint64> mSpriteKeys;
	};

	std::vector<Control> mControls;
};

