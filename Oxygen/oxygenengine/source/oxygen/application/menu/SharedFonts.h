/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiFontWrapper.h"


struct SharedFonts
{
	static inline loui::FontWrapper oxyFontRegular		 = loui::FontWrapper("oxyfont_regular");
	static inline loui::FontWrapper oxyFontRegularShadow = loui::FontWrapper("oxyfont_regular:shadow(1, 1)");
	static inline loui::FontWrapper oxyFontRegularShaded = loui::FontWrapper("oxyfont_regular:shadow(1, 1, 0.5, 0.8):outline:gradient");

	static inline loui::FontWrapper oxyFontSmall		 = loui::FontWrapper("oxyfont_small");
	static inline loui::FontWrapper oxyFontSmallShadow	 = loui::FontWrapper("oxyfont_small:shadow(1, 1)");

	static inline loui::FontWrapper smallFont			 = loui::FontWrapper("smallfont");
	static inline loui::FontWrapper smallFontOutline	 = loui::FontWrapper("smallfont:outline(0x00000080)");
};
