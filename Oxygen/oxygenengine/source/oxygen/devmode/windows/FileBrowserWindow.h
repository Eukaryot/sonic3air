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
#include "oxygen/drawing/DrawerTexture.h"


class FileBrowserWindow : public DevModeWindowBase
{
public:
	FileBrowserWindow();

	virtual void buildContent() override;

private:
	void updateFullPath();
	void refreshFileEntries();

	void drawFileBrowser();

	void drawAddressLine();
	void drawActionsMenu(bool openMenuNow);
	void drawConfirmDeletionPopup(bool openPopupNow);
	void drawRenamingPopup(bool openPopupNow);

	DrawerTexture& getFileIcon(const std::wstring& filename);

private:
	std::wstring mBasePath;
	std::wstring mFullPath;
	std::vector<std::wstring> mLocalPath;
	bool mRefreshFileEntries = false;

	std::vector<std::wstring> mDirectories;
	std::vector<std::wstring> mZipDirectories;
	std::vector<rmx::FileIO::FileEntry> mFileEntries;

	const std::wstring* mOpenActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* mOpenActionsForFile = nullptr;
	ImVec2 mActionsMenuPosition;

	DrawerTexture mFolderIconTexture;
	DrawerTexture mFileIconTexture;
	DrawerTexture mZipIconTexture;
};

#endif
