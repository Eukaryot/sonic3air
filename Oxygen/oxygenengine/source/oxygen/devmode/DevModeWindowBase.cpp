/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/DevModeWindowBase.h"

#if defined(SUPPORT_IMGUI)


DevModeWindowBase::DevModeWindowBase(std::string_view title, Category category, ImGuiWindowFlags windowFlags) :
	mTitle(title),
	mCategory(category),
	mImGuiWindowFlags(windowFlags)
{
}

void DevModeWindowBase::setIsWindowOpen(bool open)
{
	if (mIsWindowOpen == open)
		return;

	mIsWindowOpen = open;
	mLastWindowOpen = open;
	onChangedIsWindowOpen(open);
}

bool DevModeWindowBase::buildWindow()
{
	// Check if window was closed by ImGui itself (when clicking the close button)
	if (mIsWindowOpen != mLastWindowOpen)
	{
		mLastWindowOpen = mIsWindowOpen;
		onChangedIsWindowOpen(mIsWindowOpen);
	}

	if (!mIsWindowOpen)
		return false;

	if (!ImGui::Begin(mTitle.c_str(), mCanBeClosed ? &mIsWindowOpen : nullptr, mImGuiWindowFlags))
	{
		ImGui::End();
		return false;
	}

	// Check if window is outside the visible screen
	// -> We only check the right / bottom side, as ImGui already handles the others
	{
		const Vec2f maxPos = Vec2f(FTX::screenSize()) - Vec2f(10.0f);
		const ImVec2 originalScreenPos = ImGui::GetWindowPos();
		const ImVec2 screenPos(std::min(originalScreenPos.x, maxPos.x), std::min(originalScreenPos.y, maxPos.y));
		if (screenPos.x != originalScreenPos.x || screenPos.y != originalScreenPos.y)
		{
			ImGui::SetWindowPos(screenPos);
		}
	}

	buildContent();

	ImGui::End();
	return true;
}

float DevModeWindowBase::getUIScale() const
{
	return Configuration::instance().mDevMode.mUIScale;
}

#endif
