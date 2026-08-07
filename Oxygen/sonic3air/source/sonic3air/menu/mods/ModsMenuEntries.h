/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

class ModsMenu;
class Mod;
struct ModResources;


struct ModsMenuRenderContext : public GameMenuEntry::RenderContext
{
	struct SpeechBalloon
	{
		std::string_view mText;
		Vec2i mBasePosition;
	};

	Recti mVisualRect;
	Color mBaseColor;
	bool mIsActiveModsTab = false;
	bool mInMovementMode = false;
	size_t mNumModsInTab = 0;
	SpeechBalloon mSpeechBalloon;
};


class ModMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("ModMenuEntry");

public:
	struct Remark
	{
		bool mIsError = false;
		std::string mText;
	};
	std::vector<Remark> mRemarks;

public:
	ModMenuEntry();
	ModMenuEntry& initEntry(const Mod& mod, ModResources& modResources, uint32 data);

	void renderEntry(RenderContext& renderContext_) override;

	const Mod& getMod() const  { return *mMod; }
	void refreshAfterRemarksChange();

private:
	const Mod* mMod = nullptr;
	ModResources* mModResources = nullptr;
	Remark* mHighestRemark = nullptr;
};
