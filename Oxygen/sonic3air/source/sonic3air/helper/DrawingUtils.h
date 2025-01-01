/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Drawer;
class Font;


class DrawingUtils
{
public:
	static void drawSpeechBalloon(Drawer& drawer, Font& font, std::string_view text, const Recti& attachmentRect, const Recti& fenceRect, const Color& color);
};
