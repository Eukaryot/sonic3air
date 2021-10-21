/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/CheatSheetOverlay.h"
#include "oxygen/application/EngineMain.h"


CheatSheetOverlay::CheatSheetOverlay()
{
}

CheatSheetOverlay::~CheatSheetOverlay()
{
}

void CheatSheetOverlay::initialize()
{
}

void CheatSheetOverlay::deinitialize()
{
}

void CheatSheetOverlay::update(float timeElapsed)
{
	if (mShouldBeShown)
	{
		mVisibility = std::min(mVisibility + timeElapsed / 0.1f, 1.0f);
	}
	else
	{
		mVisibility = std::max(mVisibility - timeElapsed / 0.1f, 0.0f);
	}
}

void CheatSheetOverlay::render()
{
	GuiBase::render();

	if (mVisibility <= 0.0f)
		return;
	const float alpha = mVisibility;
	const Color alphaWhite(1.0f, 1.0f, 1.0f, alpha);

	Drawer& drawer = EngineMain::instance().getDrawer();
	Font& font = EngineMain::getDelegate().getDebugFont(10);

	static const std::vector<const char*> texts =
	{
		// Always available
		"Alt+Enter", "Toggle fullscreen",
		"Alt+F/G",   "Change upscaling method",
		"Alt+H",     "Change frame sync method",
		"Alt+B",     "Change background blur",
		"Alt+P",     "Change performance display",
		"F2",        "Save game recording (for debugging)",
		"F3",        "Rescan connected game controllers",
		"F4",        "Exchange player 1/2 controls",

		// Dev mode only
		"F5",        "Save state",
		"F7",        "Reload last state",
		"F8",        "Load state",
		"F10",       "Reload resources",
		"F11",       "Reload scripts",
		"0..9",      "Debug keys (can be queried in scripts)",
		",",         "Show plane B content",
		".",         "Show plane A content",
		"-",         "Show VRAM content",
		"Tab",       "Dump shown plane, VRAM or palette",
		"Alt+1..8",  "Toggle layer rendering",
		"Alt+M",     "Toggle palette view",
		"Alt+R",     "Change render method",
		"Alt+T",     "Toggle abstracted level rendering",
		"Alt+V",     "Toggle debug visualization",
		"Alt+C",     "Change between debug visualizations",
	};
	const size_t NUM_TEXTS_NONDEV = 8;
	const size_t NUM_TEXTS = EngineMain::getDelegate().useDeveloperFeatures() ? (texts.size() / 2) : NUM_TEXTS_NONDEV;

	mRect.setSize(330, (float)(58 + NUM_TEXTS * 18));
	mRect.setPos(((float)FTX::screenWidth() - mRect.width) * 0.95f, ((float)FTX::screenHeight() - mRect.height) * (1.0f - alpha * 0.1f));
	drawer.drawRect(mRect, Color(0.1f, 0.1f, 0.1f, alpha * 0.6f));
	
	Recti rct(roundToInt(mRect.x) + 20, roundToInt(mRect.y) + 16, 40, 20);
	drawer.printText(font, rct, "Hotkeys overview - show/hide with F1", 1, Color(0.5f, 1.0f, 1.0f, alpha));
	rct.y += 26;

	for (size_t i = 0; i < NUM_TEXTS; ++i)
	{
		if (i == NUM_TEXTS_NONDEV)
			rct.y += 8;
		drawer.printText(font, rct, texts[i*2], 1, alphaWhite);
		drawer.printText(font, rct + Vec2i(65, 0), texts[i*2+1], 1, alphaWhite);
		rct.y += 18;
	}
}
