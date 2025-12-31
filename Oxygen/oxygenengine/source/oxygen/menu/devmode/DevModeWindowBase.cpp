/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/DevModeWindowBase.h"

#if defined(SUPPORT_IMGUI)

#include "imgui_internal.h"


namespace
{
	void scrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button)
	{
		// Based on https://github.com/ocornut/imgui/issues/3379
		using namespace ImGui;

		ImGuiContext& g = *GetCurrentContext();
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##scrolldraggingoverlay");
		KeepAliveID(id);

		// Passing 0 to ItemHoverable means it doesn't set HoveredId, which is what we want.
		if (g.ActiveId == 0 && ItemHoverable(window->Rect(), 0, g.CurrentItemFlags) && IsMouseClicked(mouse_button, ImGuiInputFlags_None, id))
			SetActiveID(id, window);
		if (g.ActiveId == id && !g.IO.MouseDown[mouse_button])
			ClearActiveID();

		if (g.ActiveId == id && delta.x != 0.0f)
			SetScrollX(window, window->Scroll.x - delta.x);
		if (g.ActiveId == id && delta.y != 0.0f)
			SetScrollY(window, window->Scroll.y - delta.y);
	}
}


void DevModeWindowBase::allowDragScrolling()
{
	// This should be called just before "ImGui::End" and "ImGui::EndChild" (at least if the window or child actually has some empty area)
	if (Configuration::instance().mDevMode.mScrollByDragging)
	{
		scrollWhenDraggingOnVoid(ImGui::GetIO().MouseDelta, ImGuiMouseButton_Left);
	}
}


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

	mUIScale = Configuration::instance().mDevMode.mUIScale;

	// Check if window is outside the visible screen
	//  -> Note that this check is done before the "ImGui::Begin" call so we can set the corrected window position in advance,
	//     avoiding glitches where the window frame still uses the non-corrected position, and only the content uses the corrected one
	ImGuiWindow* window = ImGui::FindWindowByName(mTitle.c_str());
	if (nullptr != window)
	{
		const Vec2f maxPos = Vec2f(FTX::screenSize()) - Vec2f(40.0f, 20.0f);
		const ImVec2 originalScreenPos = window->Pos;
		const ImVec2 screenPos(std::min(originalScreenPos.x, maxPos.x), clamp(originalScreenPos.y, 0.0f, maxPos.y));
		if (screenPos.x != originalScreenPos.x || screenPos.y != originalScreenPos.y)
		{
			ImGui::SetNextWindowPos(screenPos);
		}
	}

	if (!ImGui::Begin(mTitle.c_str(), mCanBeClosed ? &mIsWindowOpen : nullptr, mImGuiWindowFlags))
	{
		ImGui::End();
		return false;
	}

	buildContent();

	allowDragScrolling();

	ImGui::End();
	return true;
}

#endif
