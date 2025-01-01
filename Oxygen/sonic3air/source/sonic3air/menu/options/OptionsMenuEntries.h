/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

class OptionsMenu;
class Mod;


struct OptionsMenuRenderContext : public GameMenuEntry::RenderContext
{
	OptionsMenu* mOptionsMenu = nullptr;
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


class ModTitleMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("ModTitleMenuEntry");

public:
	ModTitleMenuEntry();
	ModTitleMenuEntry& initEntry(const Mod& mod);

	void renderEntry(RenderContext& renderContext_) override;

	inline const Mod& getMod() const  { return *mMod; }

private:
	const Mod* mMod = nullptr;
	float mIndent = 0.0f;
};


class LabelMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("LabelMenuEntry");

public:
	LabelMenuEntry();
	LabelMenuEntry& initEntry(const std::string& text, const Color& color);

	void renderEntry(RenderContext& renderContext_) override;

protected:
	Color mColor = Color::WHITE;
};


class OptionsMenuEntry : public GameMenuEntry
{
public:
	OptionsMenuEntry& setUseSmallFont(bool useSmallFont);

	virtual void renderEntry(RenderContext& renderContext_) override;

	virtual void triggerButton() {}
	virtual bool shouldBeShown() { return true; }	// Note that this is only one of multiple conditions for whether the options entry is shown

protected:
	void renderInternal(RenderContext& renderContext_, const Color& normalColor, const Color& selectedColor);

protected:
	bool mUseSmallFont = false;
};


class AdvancedOptionMenuEntry : public OptionsMenuEntry
{
public:
	AdvancedOptionMenuEntry();
	AdvancedOptionMenuEntry& setDefaultValue(uint32 defaultValue) { mDefaultValue = defaultValue; return *this; }

	void renderEntry(RenderContext& renderContext_) override;

private:
	uint32 mDefaultValue = 0;
};


class UpdateCheckMenuEntry : public OptionsMenuEntry
{
public:
	void renderEntry(RenderContext& renderContext) override;

private:
	bool mTextUpdateLink = false;
};


class SoundtrackMenuEntry : public OptionsMenuEntry
{
public:
	void renderEntry(RenderContext& renderContext) override;
};


class SoundtrackDownloadMenuEntry : public OptionsMenuEntry
{
public:
	void renderEntry(RenderContext& renderContext) override;
	void triggerButton() override;
	bool shouldBeShown() override;
};
