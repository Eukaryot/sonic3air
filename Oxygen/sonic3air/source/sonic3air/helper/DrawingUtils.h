/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Drawer;


class DrawingUtils
{
public:
	static void drawSpeechBalloon(Drawer& drawer, const Recti& innerRect, const Color& color);
	static void drawSpeechBalloonArrowUp(Drawer& drawer, const Recti& innerRect, const Color& color, int arrowOffset);
	static void drawSpeechBalloonArrowDown(Drawer& drawer, const Recti& innerRect, const Color& color, int arrowOffset);
};
