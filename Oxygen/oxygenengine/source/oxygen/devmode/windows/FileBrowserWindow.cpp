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
#include "oxygen/platform/PlatformFunctions.h"

#if defined(PLATFORM_ANDROID)
	#include "oxygen/platform/android/AndroidJavaInterface.h"
#endif

#if defined(PLATFORM_WINDOWS)
	#define OPEN_EXTERNAL_TEXT "Open in Explorer"
#elif defined(PLATFORM_MAC)
	#define OPEN_EXTERNAL_TEXT "Open in Finder"
#elif defined(PLATFORM_LINUX)
	#define OPEN_EXTERNAL_TEXT "Open externally"
#endif


namespace
{
	static const ImVec4 FILE_COLOR(0.6f, 0.8f, 1.0f, 1.0f);		// Only used if file name is meant to be highlighted
	static const ImVec4 FOLDER_COLOR(1.0f, 1.0f, 0.6f, 1.0f);

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
	ImGui::SetWindowSize(ImVec2(700.0f, 500.0f), ImGuiCond_FirstUseEver);

	if (mBasePath.empty())
	{
		mBasePath = Configuration::instance().mAppDataPath;
		FTX::FileSystem->normalizePath(mBasePath, true);
		mLocalPath.clear();
		mRefreshFileEntries = true;
	}

	drawFileBrowser();

#if defined(PLATFORM_ANDROID)
	{
		// Update file import
		AndroidJavaInterface::FileSelection& fileSelection = AndroidJavaInterface::instance().getFileSelection();
		switch (fileSelection.mDialogResult)
		{
			case AndroidJavaInterface::BinaryDialogResult::SUCCESS:
			{
				std::wstring_view filename = std::wstring_view(fileSelection.mPath).substr(fileSelection.mPath.find_last_of(L'/') + 1);
				FTX::FileSystem->saveFile(mBasePath + mLocalPath + WString(filename).toStdWString(), fileSelection.mFileContent);

				fileSelection = AndroidJavaInterface::FileSelection();	// Reset
				mRefreshFileEntries = true;
				break;
			}

			case AndroidJavaInterface::BinaryDialogResult::FAILED:
			{
				fileSelection = AndroidJavaInterface::FileSelection();	// Reset
				break;
			}
		}
	}
#endif
}

void FileBrowserWindow::refreshFileEntries()
{
	mDirectories.clear();
	mFileEntries.clear();
	FTX::FileSystem->listDirectories(mBasePath + mLocalPath, mDirectories);
	FTX::FileSystem->listFiles(mBasePath + mLocalPath, false, mFileEntries);

	// Filter out all ZIP directories
	for (size_t k = 0; k < mDirectories.size(); ++k)
	{
		const std::wstring& dirName = mDirectories[k];
		if (rmx::endsWith(dirName, L".zip") || rmx::endsWith(dirName, L".ZIP"))
		{
			const auto isFile = [&]()
			{
				for (const rmx::FileIO::FileEntry& fileEntry : mFileEntries)
				{
					if (fileEntry.mFilename == dirName)
						return true;
				}
				return false;
			};

			if (isFile())
			{
				mZipDirectories.push_back(dirName);
				mDirectories.erase(mDirectories.begin() + k);
				--k;
			}
		}
	}

	mOpenActionsForDirectory = nullptr;
	mOpenActionsForFile = nullptr;

	mRefreshFileEntries = false;
}

void FileBrowserWindow::drawFileBrowser()
{
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

	// "Refresh" button
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		mRefreshFileEntries = true;
	}

	ImGui::SameLine();
	ImGui::Text("  ");

#if defined(PLATFORM_ANDROID) || defined(DEBUG)
	// "Import file" button
	ImGui::SameLine();
	if (ImGui::Button("Import file..."))
	{
	#if defined(PLATFORM_ANDROID)
		AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
		javaInterface.openFileSelectionDialog();
	#endif
	}
#endif

	// Address line
	ImGui::Text("[AppData] %s", *WString(mLocalPath).toUTF8());

	// Refresh directories & file entries if needed
	if (mRefreshFileEntries)
	{
		refreshFileEntries();
	}

	const std::wstring* clickedDirectory = nullptr;
	const std::wstring* openActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* openActionsForFile = nullptr;

	int index = 0;
	if (ImGui::BeginTable("FileList", 3, ImGuiTableFlags_BordersH | ImGuiTableFlags_SizingStretchProp))
	{
		ImGuiHelpers::ScopedIndent si(10.0f);

		ImGui::TableSetupColumn("Name", 0, 250);
		ImGui::TableSetupColumn("Size", 0, 100);
		ImGui::TableSetupColumn("Actions", 0, 50);
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
			if (ImGui::SmallButton(" ... "))
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
			if (ImGui::SmallButton(" ... "))
			{
				openActionsForFile = &entry;
				mActionsMenuPosition = ImGui::GetCursorScreenPos();
			}

			ImGui::PopID();
			++index;
		}

		ImGui::EndTable();
	}

	bool openActionsMenu = false;

	if (nullptr != openActionsForDirectory)
	{
		mOpenActionsForDirectory = openActionsForDirectory;
		mOpenActionsForFile = nullptr;
		openActionsMenu = true;
	}
	else if (nullptr != openActionsForFile)
	{
		mOpenActionsForFile = openActionsForFile;
		mOpenActionsForDirectory = nullptr;
		openActionsMenu = true;
	}
	else if (nullptr != clickedDirectory)
	{
		mLocalPath = mLocalPath + *clickedDirectory + L"/";
		mRefreshFileEntries = true;
	}

	drawActionsMenu(openActionsMenu);
}

void FileBrowserWindow::drawActionsMenu(bool openMenuNow)
{
	bool openConfirmDeletionPopup = false;
	bool openRenamingPopup = false;

	if (openMenuNow)
	{
		if (nullptr != mOpenActionsForDirectory)
		{
			ImGui::OpenPopup("FileBrowser_DirectoryActions");
		}
		else if (nullptr != mOpenActionsForFile)
		{
			ImGui::OpenPopup("FileBrowser_FileActions");
		}
	}

	const ImVec2 BUTTON_SIZE(130 * mUIScale, 0);

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("FileBrowser_DirectoryActions"))
	{
		if (nullptr == mOpenActionsForDirectory)
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::TextColored(FOLDER_COLOR, "%s", *WString(*mOpenActionsForDirectory).toUTF8());

			ImGui::Separator();

			if (ImGui::Button("Rename", BUTTON_SIZE))
			{
				openRenamingPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Delete", BUTTON_SIZE))
			{
				openConfirmDeletionPopup = true;
				ImGui::CloseCurrentPopup();
			}

		#if defined(OPEN_EXTERNAL_TEXT)		// Only for platforms that support it
			ImGui::Separator();
			if (ImGui::Button(OPEN_EXTERNAL_TEXT, BUTTON_SIZE))
			{
				PlatformFunctions::openDirectoryExternal(mBasePath + mLocalPath + *mOpenActionsForDirectory);
				ImGui::CloseCurrentPopup();
			}
		#endif
		}

		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("FileBrowser_FileActions"))
	{
		if (nullptr == mOpenActionsForFile)
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::Text("%s", *WString(mOpenActionsForFile->mFilename).toUTF8());

			if (rmx::endsWith(mOpenActionsForFile->mFilename, L".zip") || rmx::endsWith(mOpenActionsForFile->mFilename, L".ZIP"))
			{
				if (ImGui::Button("Open zip file", BUTTON_SIZE))
				{
					mLocalPath = mLocalPath + mOpenActionsForFile->mFilename + L"/";
					mRefreshFileEntries = true;
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();

			if (ImGui::Button("Rename", BUTTON_SIZE))
			{
				openRenamingPopup = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Button("Delete", BUTTON_SIZE))
			{
				openConfirmDeletionPopup = true;
				ImGui::CloseCurrentPopup();
			}

		#if defined(PLATFORM_ANDROID)
			ImGui::Separator();

			if (ImGui::Button("Export file...", BUTTON_SIZE))
			{
				std::vector<uint8> contents;
				if (FTX::FileSystem->readFile(mOpenActionsForFile->mPath + mOpenActionsForFile->mFilename, contents))
				{
					AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
					javaInterface.openFileExportDialog(mOpenActionsForFile->mFilename, contents);
				}
				ImGui::CloseCurrentPopup();
			}
		#endif
		}

		ImGui::EndPopup();
	}

	drawConfirmDeletionPopup(openConfirmDeletionPopup);
	drawRenamingPopup(openRenamingPopup);
}

void FileBrowserWindow::drawConfirmDeletionPopup(bool openPopupNow)
{
	bool performDeletion = false;

	if (openPopupNow)
	{
		ImGui::OpenPopup("FileBrowser_ConfirmDeletion");
	}

	if (ImGui::BeginPopupModal("FileBrowser_ConfirmDeletion", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (nullptr != mOpenActionsForFile || nullptr != mOpenActionsForDirectory)
		{
			ImGui::Text(nullptr != mOpenActionsForFile ? "Really delete file " : "Really delete folder ");
			ImGui::SameLineNoSpace();
			if (nullptr != mOpenActionsForFile)
			{
				ImGui::TextColored(FILE_COLOR, "%s", *WString(mOpenActionsForFile->mFilename).toUTF8());
			}
			else
			{
				ImGui::TextColored(FOLDER_COLOR, "%s", *WString(*mOpenActionsForDirectory).toUTF8());
			}
			ImGui::SameLineNoSpace();
			ImGui::Text("?");

			ImGui::Text("This operation can't be undone!");
			ImGui::Separator();

			// Center the buttons
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max<float>(ImGui::GetContentRegionAvail().x - 208 * mUIScale, 0) / 2);

			// Yes button
			if (ImGui::RedButton("Yes", ImVec2(100 * mUIScale, 0)))
			{
				performDeletion = true;
				ImGui::CloseCurrentPopup();
			}

			// No button
			ImGui::SameLine();
			if (ImGui::Button("No", ImVec2(100 * mUIScale, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
		}
		else
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (performDeletion)
	{
		const std::wstring currentPath = mBasePath + mLocalPath;
		if (nullptr != mOpenActionsForFile)
		{
			FTX::FileSystem->removeFile(currentPath + mOpenActionsForFile->mFilename);
		}
		else if (nullptr != mOpenActionsForDirectory)
		{
			FTX::FileSystem->removeDirectory(currentPath + *mOpenActionsForDirectory);
		}

		mRefreshFileEntries = true;
	}
}

void FileBrowserWindow::drawRenamingPopup(bool openPopupNow)
{
	static ImGuiHelpers::WideInputString newNameInput;
	bool performRenaming = false;

	if (openPopupNow)
	{
		ImGui::OpenPopup("FileBrowser_Renaming");
		if (nullptr != mOpenActionsForFile)
		{
			newNameInput.set(mOpenActionsForFile->mFilename);
		}
		else if (nullptr != mOpenActionsForDirectory)
		{
			newNameInput.set(*mOpenActionsForDirectory);
		}
	}

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("FileBrowser_Renaming"))
	{
		if (nullptr != mOpenActionsForFile || nullptr != mOpenActionsForDirectory)
		{
			ImGui::Text(nullptr != mOpenActionsForFile ? "Rename file " : "Rename folder ");
			ImGui::SameLineNoSpace();
			if (nullptr != mOpenActionsForFile)
			{
				ImGui::TextColored(FILE_COLOR, "%s", *WString(mOpenActionsForFile->mFilename).toUTF8());
			}
			else
			{
				ImGui::TextColored(FOLDER_COLOR, "%s", *WString(*mOpenActionsForDirectory).toUTF8());
			}
			ImGui::SameLineNoSpace();
			ImGui::Text("to:");

			if (openPopupNow)
			{
				ImGui::SetKeyboardFocusHere();
			}
			if (ImGui::InputText("##NewName", newNameInput.mInternalUTF8, sizeof(newNameInput.mInternalUTF8), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll))
			{
				// TODO: Check for invalid characters for the file name (including slashes and hacks like "..")
				newNameInput.refreshFromInternal();
			}

			ImGui::Separator();

			// Center the buttons
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max<float>(ImGui::GetContentRegionAvail().x - 208 * mUIScale, 0) / 2);

			// Buttons
			if (ImGui::Button("Rename", ImVec2(100 * mUIScale, 0)))
			{
				performRenaming = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100 * mUIScale, 0)))
			{
				ImGui::CloseCurrentPopup();
			}
		}
		else
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (performRenaming)
	{
		const std::wstring currentPath = mBasePath + mLocalPath;
		if (nullptr != mOpenActionsForFile)
		{
			const std::wstring oldName = mOpenActionsForFile->mFilename;
			const std::wstring newName = newNameInput.get().toStdWString();
			if (oldName != newName)
			{
				FTX::FileSystem->renameFile(currentPath + oldName, currentPath + newName);
			}
		}
		else if (nullptr != mOpenActionsForDirectory)
		{
			const std::wstring oldName = *mOpenActionsForDirectory;
			const std::wstring newName = newNameInput.get().toStdWString();
			if (oldName != newName)
			{
				FTX::FileSystem->renameDirectory(currentPath + oldName, currentPath + newName);
			}
		}

		mRefreshFileEntries = true;
	}
}

#endif
