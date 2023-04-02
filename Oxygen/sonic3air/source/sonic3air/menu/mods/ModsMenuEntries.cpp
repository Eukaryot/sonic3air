/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/mods/ModsMenuEntries.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/modding/Mod.h"


ModMenuEntry::ModMenuEntry()
{
	mMenuEntryType = MENU_ENTRY_TYPE;
}

ModMenuEntry& ModMenuEntry::initEntry(const Mod& mod, uint32 data)
{
	mMod = &mod;
	mText = mod.mDisplayName;
	mData = data;
	return *this;
}

void ModMenuEntry::renderEntry(RenderContext& renderContext_)
{
	ModsMenuRenderContext& renderContext = renderContext_.as<ModsMenuRenderContext>();
	Drawer& drawer = *renderContext.mDrawer;

	Color textColor = renderContext.mBaseColor;
	if (mMod->mState == Mod::State::FAILED)
	{
		textColor.set(1.0f, renderContext.mIsSelected ? 0.5f : 0.0f, 0.0f, renderContext.mBaseColor.a);

		static const uint64 spriteKey = rmx::getMurmur2_64("small_warning_icon_red");
		drawer.drawSprite(Vec2i(renderContext.mVisualRect.x + 212, renderContext.mVisualRect.y + 4), spriteKey, Color(1.0f, 1.0f, 1.0f, renderContext.mBaseColor.a));

		if (renderContext.mIsSelected)
		{
			renderContext.mSpeechBalloon.mText = mMod->mFailedMessage;
			renderContext.mSpeechBalloon.mBasePosition.set(renderContext.mVisualRect.x + 212, renderContext.mVisualRect.y + 4);
		}
	}

	drawer.printText(global::mOxyfontSmall, renderContext.mVisualRect + Vec2i(24, 0), mMod->mDisplayName, 1, textColor);
}
