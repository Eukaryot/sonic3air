/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

class OptionsMenu;


struct OptionsMenuRenderContext : public GameMenuEntry::RenderContext
{
	OptionsMenu* mOptionsMenu = nullptr;
	bool mIsSelected = false;
	float mTabAlpha = 1.0f;
	bool mIsModsTab = false;
};


class TitleMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("TitleMenuEntry");

public:
	TitleMenuEntry();
	TitleMenuEntry& initEntry(const std::string& text);

	void renderEntry(RenderContext& renderContext_) override;
};


class SectionMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("SectionMenuEntry");

public:
	SectionMenuEntry();
	SectionMenuEntry& initEntry(const std::string& text);

	void renderEntry(RenderContext& renderContext_) override;
};


class OptionsMenuEntry : public GameMenuEntry
{
public:
	OptionsMenuEntry& setUseSmallFont(bool useSmallFont);

	void renderEntry(RenderContext& renderContext_) override;

private:
	bool mUseSmallFont = false;
};


class UpdateCheckMenuEntry : public OptionsMenuEntry
{
public:
	void renderEntry(RenderContext& renderContext) override;

private:
	bool mTextUpdateLink = false;
};
