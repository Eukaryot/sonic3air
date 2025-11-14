/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/FileBrowserWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"


namespace
{
	void drawFileSize(uint64 fileSize)
	{
		if (fileSize < 1000)
		{
			ImGui::Text("%u Bytes", (int)fileSize);
		}
		else if (fileSize < 1000 * 1000)
		{
			ImGui::Text("%u,%03u Bytes", (int)(fileSize / 1000), (int)(fileSize % 1000));
		}
		else if (fileSize < 1000 * 1000 * 1000)
		{
			ImGui::Text("%u,%03u,%03u Bytes", (int)(fileSize / (1000 * 1000)), (int)(fileSize / 1000 % 1000), (int)(fileSize % 1000));
		}
		else
		{
			ImGui::Text("%u,%03u,%03u,%03u Bytes", (int)(fileSize / (1000 * 1000 * 1000)), (int)(fileSize / (1000 * 1000) % 1000), (int)(fileSize / 1000 % 1000), (int)(fileSize % 1000));
		}
	}
}


FileBrowserWindow::FileBrowserWindow() :
	DevModeWindowBase("File Browser", Category::MISC, 0)
{
}

void FileBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(100.0f, 200.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(600.0f, 500.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	if (mBasePath.empty())
	{
		mBasePath = Configuration::instance().mAppDataPath;
		FTX::FileSystem->normalizePath(mBasePath, true);
		mLocalPath.clear();
		mRefreshFileEntries = true;
	}

	// Address line
	ImGui::Text("[AppData] %s", *WString(mLocalPath).toUTF8());

	// "Up" button
	ImGui::BeginDisabled(mLocalPath.empty());
	if (ImGui::Button("Up"))
	{
		if (!mLocalPath.empty())
		{
			size_t pos = mLocalPath.find_last_of(L'/', mLocalPath.size() - 2);
			if (pos != std::wstring::npos)
			{
				mLocalPath.erase(pos + 1);
			}
			else
			{
				mLocalPath.clear();
			}
			mRefreshFileEntries = true;
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::Text("  ");
	ImGui::SameLine();
	if (ImGui::Button("Import"))
	{
		// TODO: Implement this, at least for Android
	}

	// Refresh directories & file entries if needed
	if (mRefreshFileEntries)
	{
		mDirectories.clear();
		mFileEntries.clear();
		FTX::FileSystem->listDirectories(mBasePath + mLocalPath, mDirectories);
		FTX::FileSystem->listFiles(mBasePath + mLocalPath, false, mFileEntries);

		mOpenActionsForDirectory = nullptr;
		mOpenActionsForFile = nullptr;

		mRefreshFileEntries = false;
	}

	static const ImVec4 FOLDER_COLOR(1.0f, 1.0f, 0.5f, 1.0f);

	const std::wstring* clickedDirectory = nullptr;
	const std::wstring* openActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* openActionsForFile = nullptr;

	int index = 0;
	if (ImGui::BeginTable("FileList", 3, ImGuiTableFlags_BordersH | ImGuiTableFlags_SizingStretchProp))
	{
		ImGuiHelpers::ScopedIndent si(10.0f);

		ImGui::TableSetupColumn("Name", 0, 250);
		ImGui::TableSetupColumn("Size", 0, 100);
		ImGui::TableSetupColumn("Actions", 0, 40);
		ImGui::TableHeadersRow();

		for (const std::wstring& directory : mDirectories)
		{
			ImGui::PushID(index);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::Selectable("##", false, 0);
			if (ImGui::IsItemClicked())
			{
				clickedDirectory = &directory;
			}
			ImGui::SameLine();
			ImGui::TextColored(FOLDER_COLOR, "%s", *WString(directory).toUTF8());

			ImGui::TableNextColumn();

			ImGui::TableNextColumn();
			if (ImGui::SmallButton("..."))
			{
				openActionsForDirectory = &directory;
				mActionsMenuPosition = ImGui::GetCursorScreenPos();
			}

			ImGui::PopID();
			++index;
		}

		for (const rmx::FileIO::FileEntry& entry : mFileEntries)
		{
			ImGui::PushID(index);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::Selectable("##", false, 0);
			ImGui::SameLine();
			ImGui::Text("%s", *WString(entry.mFilename).toUTF8());

			ImGui::TableNextColumn();
			drawFileSize((uint64)entry.mSize);

			ImGui::TableNextColumn();
			if (ImGui::SmallButton("..."))
			{
				openActionsForFile = &entry;
				mActionsMenuPosition = ImGui::GetCursorScreenPos();
			}

			ImGui::PopID();
			++index;
		}

		ImGui::EndTable();
	}

	if (nullptr != openActionsForDirectory)
	{
		mOpenActionsForDirectory = openActionsForDirectory;
		mOpenActionsForFile = nullptr;
		ImGui::OpenPopup("DirectoryActions");
	}
	else if (nullptr != openActionsForFile)
	{
		mOpenActionsForFile = openActionsForFile;
		mOpenActionsForDirectory = nullptr;
		ImGui::OpenPopup("FileActions");
	}
	else if (nullptr != clickedDirectory)
	{
		mLocalPath = mLocalPath + *clickedDirectory + L"/";
		mRefreshFileEntries = true;
	}

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("DirectoryActions"))
	{
		if (nullptr == mOpenActionsForDirectory)
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::TextColored(FOLDER_COLOR, "%s", *WString(*mOpenActionsForDirectory).toUTF8());
			const ImVec2 BUTTON_SIZE(100, 20);

			if (ImGui::Button("Rename", BUTTON_SIZE))
			{
				// TODO: Implement this
			}

			if (ImGui::Button("Delete", BUTTON_SIZE))
			{
				// TODO: Implement this
			}
		}

		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("FileActions"))
	{
		if (nullptr == mOpenActionsForFile)
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::Text("%s", *WString(mOpenActionsForFile->mFilename).toUTF8());
			const ImVec2 BUTTON_SIZE(100, 20);

			if (ImGui::Button("Rename", BUTTON_SIZE))
			{
				// TODO: Implement this
			}

			if (ImGui::Button("Delete", BUTTON_SIZE))
			{
				// TODO: Implement this
			}
		}

		ImGui::EndPopup();
	}
}

#endif
