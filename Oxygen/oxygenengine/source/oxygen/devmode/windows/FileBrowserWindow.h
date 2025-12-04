/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/devmode/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/DevModeWindowBase.h"


class FileBrowserWindow : public DevModeWindowBase
{
public:
	FileBrowserWindow();

	virtual void buildContent() override;

private:
	std::wstring mBasePath;
	std::wstring mLocalPath;
	bool mRefreshFileEntries = false;
	std::vector<std::wstring> mDirectories;
	std::vector<rmx::FileIO::FileEntry> mFileEntries;

	const std::wstring* mOpenActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* mOpenActionsForFile = nullptr;
	ImVec2 mActionsMenuPosition;
};

#endif
