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
#include "oxygen/helper/FileHelper.h"
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
	FileHelper::loadTexture(mFolderIconTexture, L"data/images/menu/filebrowser/icon_folder_32.png");
	FileHelper::loadTexture(mFileIconTexture, L"data/images/menu/filebrowser/icon_file_32.png");
	FileHelper::loadTexture(mZipIconTexture, L"data/images/menu/filebrowser/icon_zip_32.png");
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
		updateFullPath();
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
				FTX::FileSystem->saveFile(mFullPath + WString(filename).toStdWString(), fileSelection.mFileContent);

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

void FileBrowserWindow::updateFullPath()
{
	mFullPath = mBasePath;
	for (const std::wstring& dir : mLocalPath)
	{
		mFullPath += dir + L"/";
	}
	mRefreshFileEntries = true;
}

void FileBrowserWindow::refreshFileEntries()
{
	mDirectories.clear();
	mFileEntries.clear();
	FTX::FileSystem->listDirectories(mFullPath, mDirectories);
	FTX::FileSystem->listFiles(mFullPath, false, mFileEntries);

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
	ImGui::Text("  ");
	ImGui::SameLine();

	// "Refresh" button
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
	drawAddressLine();

	ImGui::Spacing();

	// Refresh directories & file entries if needed
	if (mRefreshFileEntries)
	{
		refreshFileEntries();
	}

	const std::wstring* clickedDirectory = nullptr;
	const std::wstring* openActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* openActionsForFile = nullptr;

	ImGui::PushStyleVarY(ImGuiStyleVar_CellPadding, 5.0f * mUIScale);
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

	int index = 0;
	if (ImGui::BeginTable("FileList", 3, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
	{
		ImGuiHelpers::ScopedIndent si(10.0f);

		ImGui::TableSetupColumn("Name", 0, 250);
		ImGui::TableSetupColumn("Size", 0, 100);
		ImGui::TableSetupColumn("Actions", 0, 50);
		ImGui::TableHeadersRow();

		// Directories
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
			ImGui::Image(ImGuiHelpers::getTextureRef(mFolderIconTexture), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::TextColored(FOLDER_COLOR, "%s", rmx::convertToUTF8(directory).c_str());

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

		// Files
		for (const rmx::FileIO::FileEntry& entry : mFileEntries)
		{
			ImGui::PushID(index);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::Selectable("##", false, 0);
			ImGui::SameLine();
			ImGui::Image(ImGuiHelpers::getTextureRef(getFileIcon(entry.mFilename)), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::Text("%s", rmx::convertToUTF8(entry.mFilename).c_str());

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
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();

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
		mLocalPath.push_back(*clickedDirectory);
		updateFullPath();
	}

	drawActionsMenu(openActionsMenu);
}

void FileBrowserWindow::drawAddressLine()
{
	// Draw background frame
	const float rectWidth = ImGui::GetWindowWidth() - 32 * mUIScale;
	const ImVec2 rectStart = ImGui::GetCursorScreenPos();
	const ImVec2 rectEnd = rectStart + ImVec2(rectWidth, 29 * mUIScale);
	ImGui::GetWindowDrawList()->AddRectFilled(rectStart, rectEnd, 0xff402020, 5 * mUIScale);
	ImGui::GetWindowDrawList()->AddRect(rectStart, rectEnd, 0xffc08080, 5 * mUIScale);

	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(4, 4) * mUIScale);

	// "Up" button
	ImGui::BeginDisabled(mLocalPath.empty());
	if (ImGui::Button("Up"))
	{
		if (!mLocalPath.empty())
		{
			mLocalPath.pop_back();
			updateFullPath();
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10 * mUIScale);

	ImGui::PushClipRect(ImVec2(ImGui::GetCursorScreenPos().x - 1, rectStart.y), rectEnd, true);

	// Estimate total width of the entries
	float width = ImGui::CalcTextSize("AppData").x + 11 * mUIScale;
	for (size_t index = 0; index < mLocalPath.size(); ++index)
	{
		width += ImGui::CalcTextSize(rmx::convertToUTF8(mLocalPath[index]).c_str()).x;
		width += 26 * mUIScale;
	}
	// Just for debugging
	//ImGui::GetWindowDrawList()->AddRect(rectStart, rectStart + ImVec2(width, 29 * mUIScale), 0xff00ff00, 5 * mUIScale);

	const float px1 = ImGui::GetCursorPosX();
	const float px2 = (rectEnd.x - ImGui::GetWindowPos().x) - width;
	ImGui::SetCursorPosX(std::min(px1, px2));

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.5f, 0.5f));
	for (size_t index = 0; index <= mLocalPath.size(); ++index)
	{
		if (index == 0)
		{
		}
		else
		{
			ImGui::SameLineRelSpace(0.5f);
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), ">");
			ImGui::SameLineRelSpace(0.5f);
		}

		if (ImGui::Button((index == 0) ? "AppData" : (rmx::convertToUTF8(mLocalPath[index - 1]) + "##" + std::to_string(index)).c_str()))
		{
			mLocalPath.resize(index);
			updateFullPath();
			break;
		}
	}
	ImGui::PopStyleColor(1);

	ImGui::PopClipRect();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
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
			ImGui::Image(ImGuiHelpers::getTextureRef(mFolderIconTexture), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::TextColored(FOLDER_COLOR, "%s", rmx::convertToUTF8(*mOpenActionsForDirectory).c_str());

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
				PlatformFunctions::openDirectoryExternal(mFullPath + *mOpenActionsForDirectory);
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
			ImGui::Image(ImGuiHelpers::getTextureRef(getFileIcon(mOpenActionsForFile->mFilename)), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::Text("%s", rmx::convertToUTF8(mOpenActionsForFile->mFilename).c_str());

			if (rmx::endsWith(mOpenActionsForFile->mFilename, L".zip") || rmx::endsWith(mOpenActionsForFile->mFilename, L".ZIP"))
			{
				if (ImGui::Button("Open zip file", BUTTON_SIZE))
				{
					mLocalPath.push_back(mOpenActionsForFile->mFilename);
					updateFullPath();
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
				ImGui::TextColored(FILE_COLOR, "%s", rmx::convertToUTF8(mOpenActionsForFile->mFilename).c_str());
			}
			else
			{
				ImGui::TextColored(FOLDER_COLOR, "%s", rmx::convertToUTF8(*mOpenActionsForDirectory).c_str());
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
		const std::wstring currentPath = mFullPath;
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
				ImGui::TextColored(FILE_COLOR, "%s", rmx::convertToUTF8(mOpenActionsForFile->mFilename).c_str());
			}
			else
			{
				ImGui::TextColored(FOLDER_COLOR, "%s", rmx::convertToUTF8(*mOpenActionsForDirectory).c_str());
			}
			ImGui::SameLineNoSpace();
			ImGui::Text(" to:");

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
		const std::wstring currentPath = mFullPath;
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

DrawerTexture& FileBrowserWindow::getFileIcon(const std::wstring& filename)
{
	if (rmx::endsWith(filename, L".zip"))
	{
		return mZipIconTexture;
	}
	return mFileIconTexture;
}

#endif
