/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/DrawingUtils.h"

#include "oxygen/helper/Utils.h"


namespace
{
	void drawSpeechBalloon(Drawer& drawer, const Recti& innerRect, const Color& color, int arrowDirection, int arrowX)
	{
		static const uint64 key00 = rmx::getMurmur2_64(std::string_view("speechballoon_00"));
		static const uint64 key01 = rmx::getMurmur2_64(std::string_view("speechballoon_01"));
		static const uint64 key02 = rmx::getMurmur2_64(std::string_view("speechballoon_02"));
		static const uint64 key10 = rmx::getMurmur2_64(std::string_view("speechballoon_10"));
		static const uint64 key11 = rmx::getMurmur2_64(std::string_view("speechballoon_11"));
		static const uint64 key12 = rmx::getMurmur2_64(std::string_view("speechballoon_12"));
		static const uint64 key20 = rmx::getMurmur2_64(std::string_view("speechballoon_20"));
		static const uint64 key21 = rmx::getMurmur2_64(std::string_view("speechballoon_21"));
		static const uint64 key22 = rmx::getMurmur2_64(std::string_view("speechballoon_22"));
		static const uint64 arrowUp = rmx::getMurmur2_64(std::string_view("speechballoon_arrow_up"));
		static const uint64 arrowDown = rmx::getMurmur2_64(std::string_view("speechballoon_arrow_down"));

		constexpr int ARROW_WIDTH = 12;

		arrowX -= innerRect.x;
		if (arrowDirection != 0)
		{
			arrowX = clamp(arrowX, ARROW_WIDTH / 2, innerRect.width - ARROW_WIDTH / 2);
		}

		const int x0 = innerRect.x;
		const int x1 = innerRect.x + innerRect.width;
		const int y0 = innerRect.y;
		const int y1 = innerRect.y + innerRect.height;

		const int left = arrowX - ARROW_WIDTH / 2;
		const int right = left + ARROW_WIDTH;

		// Corners
		drawer.drawSprite(Vec2i(x0, y0), key00, color);
		drawer.drawSprite(Vec2i(x0, y1), key02, color);
		drawer.drawSprite(Vec2i(x1, y0), key20, color);
		drawer.drawSprite(Vec2i(x1, y1), key22, color);

		// Left / right border
		drawer.drawSpriteRect(Recti(x0-5, y0, 5, innerRect.height), key01, color);
		drawer.drawSpriteRect(Recti(x1,   y0, 5, innerRect.height), key21, color);

		// Upper border
		if (arrowDirection < 0 && innerRect.width >= ARROW_WIDTH)
		{
			drawer.drawSpriteRect(Recti(x0, y0-5, left, 5), key10, color);
			drawer.drawSpriteRect(Recti(x0 + right, y0-5, innerRect.width - right, 5), key10, color);
			drawer.drawSprite(Vec2i(x0 + arrowX, y0), arrowUp, color);
		}
		else
		{
			drawer.drawSpriteRect(Recti(x0, y0-5, innerRect.width, 5), key10, color);
		}

		// Lower border
		if (arrowDirection > 0 && innerRect.width >= ARROW_WIDTH)
		{
			drawer.drawSpriteRect(Recti(x0, y1, left, 5), key12, color);
			drawer.drawSpriteRect(Recti(x0 + right, y1, innerRect.width - right, 5), key12, color);
			drawer.drawSprite(Vec2i(x0 + arrowX, y1), arrowDown, color);
		}
		else
		{
			drawer.drawSpriteRect(Recti(x0, y1, innerRect.width, 5), key12, color);
		}

		// Inner
		drawer.drawSpriteRect(innerRect, key11, color);
	}
}


void DrawingUtils::drawSpeechBalloon(Drawer& drawer, Font& font, std::string_view text, const Recti& attachmentRect, const Recti& fenceRect, const Color& color)
{
	std::vector<std::string_view> lines;
	utils::splitTextIntoLines(lines, text, font, fenceRect.width);

	const Vec2i attachmentCenter = attachmentRect.getCenter();
	const int lineHeight = font.getLineHeight() + 1;

	Recti textRect;
	textRect.width = 0;
	for (const std::string_view& line : lines)
		textRect.width = std::max(textRect.width, font.getWidth(line));
	textRect.height = lineHeight * (int)lines.size() - 3;

	textRect.x = attachmentCenter.x - textRect.width / 2;
	if (textRect.x < fenceRect.x)
	{
		textRect.x = fenceRect.x;
	}
	else if (textRect.x + textRect.width > fenceRect.x + fenceRect.width)
	{
		textRect.x = fenceRect.x + fenceRect.width - textRect.width;
	}

	if (attachmentCenter.y < fenceRect.getCenter().y)
	{
		textRect.y = attachmentRect.y + attachmentRect.height + 3;
	}
	else
	{
		textRect.y = attachmentRect.y - textRect.height - 17;
	}

	const Recti innerRect(textRect.x - 3, textRect.y - 2, textRect.width + 6, textRect.height + 4);

	if (attachmentCenter.y < fenceRect.getCenter().y)
	{
		::drawSpeechBalloon(drawer, innerRect, color, -1, attachmentCenter.x - 1);
	}
	else
	{
		::drawSpeechBalloon(drawer, innerRect, color, +1, attachmentCenter.x - 1);
	}

	Recti rct = textRect;
	for (const std::string_view& line : lines)
	{
		drawer.printText(font, rct, line, 1);
		rct.y += lineHeight;
	}
}
