/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/Drawer.h"


class DrawerHelper
{
public:
	static void drawBorderedRect(Drawer& drawer, const Recti& rect, int borderWidth, const Color& innerColor, const Color& borderColor)
	{
		const Recti innerRect(rect.x + borderWidth, rect.y + borderWidth, rect.width - borderWidth * 2, rect.height - borderWidth * 2);
		drawer.drawRect(innerRect, innerColor);

		// Upper and lower border (full width)
		drawer.drawRect(Recti(rect.x, rect.y,                         rect.width, borderWidth), borderColor);
		drawer.drawRect(Recti(rect.x, innerRect.y + innerRect.height, rect.width, borderWidth), borderColor);

		// Left and right border (inner height)
		drawer.drawRect(Recti(rect.x,                        innerRect.y, borderWidth, innerRect.height), borderColor);
		drawer.drawRect(Recti(innerRect.x + innerRect.width, innerRect.y, borderWidth, innerRect.height), borderColor);
	}
};
