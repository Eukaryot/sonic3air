/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/mods/ModsMenuEntries.h"
#include "sonic3air/menu/mods/ModResources.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/modding/Mod.h"


ModMenuEntry::ModMenuEntry()
{
	mMenuEntryType = MENU_ENTRY_TYPE;
}

ModMenuEntry& ModMenuEntry::initEntry(const Mod& mod, ModResources& modResources, uint32 data)
{
	mMod = &mod;
	mModResources = &modResources;
	mText = mod.mDisplayName;
	mData = data;
	return *this;
}

void ModMenuEntry::renderEntry(RenderContext& renderContext_)
{
	ModsMenuRenderContext& renderContext = renderContext_.as<ModsMenuRenderContext>();
	Drawer& drawer = *renderContext.mDrawer;

	if (renderContext.mIsSelected && renderContext.mInMovementMode)
	{
		// Draw light blue background
		Recti bgRect = renderContext.mVisualRect;
		bgRect.addPos(-20, -5);
		drawer.drawRect(bgRect, Color(0.25f, 0.75f, 1.0f, 0.5f * renderContext.mBaseColor.a));
	}

	DrawerTexture& texture = renderContext.mIsActiveModsTab ? mModResources->mSmallIcon : mModResources->mSmallIconGray;
	if (texture.isValid())
	{
		const Recti iconRect(renderContext.mVisualRect.x, renderContext.mVisualRect.y - 4, 16, 16);
		drawer.drawRect(iconRect, texture, Color(1.0f, 1.0f, 1.0f, renderContext.mBaseColor.a));
	}

	Color textColor = renderContext.mBaseColor;
	{
		// Check for errors / warnings
		int errorLevel = 0;
		std::string_view message;
		if (mMod->mState == Mod::State::FAILED)
		{
			errorLevel = 2;
			message = mMod->mFailedMessage;
		}
		else if (nullptr != mHighestRemark)
		{
			errorLevel = mHighestRemark->mIsError ? 2 : 1;
			message = mHighestRemark->mText;
		}

		if (errorLevel > 0)
		{
			if (errorLevel == 1)
				textColor.set(1.0f, renderContext.mIsSelected ? 1.0f : 0.75f, 0.0f, renderContext.mBaseColor.a);
			else
				textColor.set(1.0f, renderContext.mIsSelected ? 0.5f : 0.0f, 0.0f, renderContext.mBaseColor.a);

			// Draw warning icon
			static const uint64 spriteKeys[2] =
			{
				rmx::constMurmur2_64("small_warning_icon_yellow"),
				rmx::constMurmur2_64("small_warning_icon_red")
			};
			drawer.drawSprite(Vec2i(renderContext.mVisualRect.x + 212, renderContext.mVisualRect.y + 4), spriteKeys[errorLevel-1], Color(1.0f, 1.0f, 1.0f, renderContext.mBaseColor.a));

			if (renderContext.mIsSelected)
			{
				renderContext.mSpeechBalloon.mText = message;
				renderContext.mSpeechBalloon.mBasePosition.set(renderContext.mVisualRect.x + 212, renderContext.mVisualRect.y + 4);
			}
		}
	}

	drawer.printText(global::mOxyfontSmall, renderContext.mVisualRect + Vec2i(24, 0), mMod->mDisplayName, 1, textColor);

	if (renderContext.mIsSelected && renderContext.mInMovementMode)
	{
		// Draw arrows
		if (renderContext.mIsActiveModsTab)
		{
			if (renderContext.mNumModsInTab >= 2)
				drawer.printText(global::mOxyfontSmall, renderContext.mVisualRect - Vec2i(12, 1), L"\u21f3", 1, renderContext.mBaseColor);
			drawer.printText(global::mOxyfontSmall, renderContext.mVisualRect + Vec2i(225, 0), L"\u25ba", 1, renderContext.mBaseColor);
		}
		else
		{
			drawer.printText(global::mOxyfontSmall, renderContext.mVisualRect - Vec2i(10, 0), L"\u25c4", 1, renderContext.mBaseColor);
		}
	}
}

void ModMenuEntry::refreshAfterRemarksChange()
{
	mHighestRemark = nullptr;
	for (Remark& remark : mRemarks)
	{
		if (nullptr == mHighestRemark || (remark.mIsError && !mHighestRemark->mIsError))
			mHighestRemark = &remark;
	}
}
