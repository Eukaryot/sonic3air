/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/helper/GameMenuControlsDisplay.h"
#include "sonic3air/menu/SharedResources.h"


void GameMenuControlsDisplay::clear()
{
	mControls.clear();
}

void GameMenuControlsDisplay::addControl(std::string_view displayText, bool alignRight, std::string_view spriteName)
{
	addControl(displayText, alignRight, spriteName, "");
}

void GameMenuControlsDisplay::addControl(std::string_view displayText, bool alignRight, std::string_view spriteName, std::string_view additionalSpriteName)
{
	const uint64 spriteKey = rmx::getMurmur2_64(spriteName);
	for (Control& control : mControls)
	{
		if (!control.mSpriteKeys.empty() && spriteKey == control.mSpriteKeys[0])
		{
			control.mDisplayText = displayText;
			return;
		}
	}

	Control& newControl = vectorAdd(mControls);
	newControl.mDisplayText = displayText;
	newControl.mAlignRight = alignRight;
	newControl.mSpriteKeys.push_back(spriteKey);

	if (!additionalSpriteName.empty())
		newControl.mSpriteKeys.push_back(rmx::getMurmur2_64(additionalSpriteName));
}

void GameMenuControlsDisplay::render(Drawer& drawer, float visibility)
{
	Vec2i pos(0, 216 + roundToInt((1.0f - visibility) * 16));

	// Background
	drawer.drawRect(Recti(pos.x, pos.y - 1, 400, 225 - pos.y), Color(0.0f, 0.0f, 0.0f));
	//	for (int k = 0; k < 6; ++k)
	//		drawer.drawRect(Recti(pos.x, pos.y - 7 + k, 400, 1), Color(0.0f, 0.0f, 0.0f, 0.1f * k - 0.05f));

	// Left-aligned entries
	pos.x += 12;
	for (const Control& control : mControls)
	{
		if (!control.mSpriteKeys.empty() && !control.mAlignRight)
		{
			drawControl(control, drawer, pos);
		}
	}

	// Right-aligned entries (in reverse order)
	pos.x = 400 - 8;
	for (auto it = mControls.crbegin(); it != mControls.crend(); ++it)
	{
		const Control& control = *it;
		if (!control.mSpriteKeys.empty() && control.mAlignRight)
		{
			drawControl(control, drawer, pos);
		}
	}
}

void GameMenuControlsDisplay::drawControl(const Control& control, Drawer& drawer, Vec2i& pos)
{
	Font& font = global::mOxyfontTinyRect;
	const int textWidth = font.getWidth(control.mDisplayText);

	if (control.mAlignRight)
	{
		pos.x -= textWidth;
		drawer.printText(font, Vec2i(pos.x, pos.y + 1), control.mDisplayText, 4);
		pos.x -= 15;

		for (uint64 spriteKey : control.mSpriteKeys)
		{
			drawer.drawSprite(pos + Vec2i(4, 0), spriteKey);
			pos.x -= 16;
		}
	}
	else
	{
		for (uint64 spriteKey : control.mSpriteKeys)
		{
			drawer.drawSprite(pos + Vec2i(4, 0), spriteKey);
			pos.x += 16;
		}

		drawer.printText(font, Vec2i(pos.x, pos.y + 1), control.mDisplayText, 4);
		pos.x += textWidth;
		pos.x += 15;
	}
}
