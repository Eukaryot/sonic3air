/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

class ModsMenu;
class Mod;


struct ModsMenuRenderContext : public GameMenuEntry::RenderContext
{
	struct SpeechBalloon
	{
		std::string_view mText;
		Vec2i mBasePosition;
	};

	Recti mVisualRect;
	bool mIsSelected = false;
	Color mBaseColor;
	SpeechBalloon mSpeechBalloon;
};


class ModMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("ModMenuEntry");

public:
	ModMenuEntry();
	ModMenuEntry& initEntry(const Mod& mod, uint32 data);

	void renderEntry(RenderContext& renderContext_) override;

private:
	const Mod* mMod = nullptr;
};
