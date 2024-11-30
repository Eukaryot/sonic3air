/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/DevModeWindowBase.h"

#if defined(SUPPORT_IMGUI)


DevModeWindowBase::DevModeWindowBase(std::string_view title, Category category, ImGuiWindowFlags windowFlags) :
	mTitle(title),
	mCategory(category),
	mImGuiWindowFlags(windowFlags)
{
}

bool DevModeWindowBase::buildWindow()
{
	if (!mIsWindowOpen)
		return false;

	if (!ImGui::Begin(mTitle.c_str(), &mIsWindowOpen, mImGuiWindowFlags))
	{
		ImGui::End();
		return false;
	}

	buildContent();

	ImGui::End();
	return true;
}

#endif
