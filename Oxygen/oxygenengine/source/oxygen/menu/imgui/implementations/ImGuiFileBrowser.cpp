/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/imgui/implementations/ImGuiFileBrowser.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/menu/imgui/ImGuiIntegration.h"
#include "oxygen/menu/imgui/ImGuiHelpers.h"
#include "oxygen/menu/devmode/DevModeMainWindow.h"	// Only used for "DevModeWindowBase::allowDragScrolling"
#include "oxygen/platform/PlatformFlags.h"
#include "oxygen/platform/PlatformFunctions.h"

#if defined(PLATFORM_ANDROID)
	#include "oxygen/platform/android/AndroidJavaInterface.h"
#elif defined(PLATFORM_WEB)
	#include <emscripten.h>
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
#if defined(PLATFORM_WEB)
	ImGuiFileBrowser* sActiveFileBrowser = nullptr;
#endif

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

	void moveWindowToFitOnScreen(ImVec2& position, float uiScale)
	{
		const float diffX = (ImGui::GetWindowPos().x + ImGui::GetWindowSize().x) - FTX::screenWidth();
		const float diffY = (ImGui::GetWindowPos().y + ImGui::GetWindowSize().y) - FTX::screenHeight();
		if (diffX > 0.0f)
			position.x -= std::min(diffX, 1000.0f * FTX::getTimeDifference() * uiScale);
		if (diffY > 0.0f)
			position.y -= std::min(diffY, 1000.0f * FTX::getTimeDifference() * uiScale);
	}

	bool isZipFileName(std::wstring_view filename)
	{
		return rmx::endsWithCaseInsensitive(filename, L".zip");
	}

	void mountModZipFile(std::wstring_view filepath)
	{
		if (isZipFileName(filepath) && rmx::startsWith(filepath, ModManager::instance().getModsBasePath()))
		{
			const std::wstring modsLocalFilePath = std::wstring(filepath.substr(ModManager::instance().getModsBasePath().length()));
			ModManager::instance().addZipFileProvider(modsLocalFilePath);
		}
	}

	void unmountModZipFile(std::wstring_view filepath)
	{
		if (isZipFileName(filepath) && rmx::startsWith(filepath, ModManager::instance().getModsBasePath()))
		{
			const std::wstring modsLocalFilePath = std::wstring(filepath.substr(ModManager::instance().getModsBasePath().length()));
			ModManager::instance().tryRemoveZipFileProvider(modsLocalFilePath);
		}
	}

	int filterFileNameInput(ImGuiInputTextCallbackData* data)
	{
		static const char FORBIDDEN_FILENAME_CHARACTERS[] = "/\\*?:|<>\"";

		// Check for forbidden characters
		for (char character : FORBIDDEN_FILENAME_CHARACTERS)
		{
			if (data->EventChar == character)
				return 1;
		}
		return 0;
	}
}

ImGuiFileBrowser::ImGuiFileBrowser()
{
	FileHelper::loadTexture(mFolderIconTexture, L"data/images/menu/filebrowser/icon_folder_32.png");
	FileHelper::loadTexture(mFileIconTexture, L"data/images/menu/filebrowser/icon_file_32.png");
	FileHelper::loadTexture(mZipIconTexture, L"data/images/menu/filebrowser/icon_zip_32.png");
}

void ImGuiFileBrowser::buildImGuiContent()
{
#if defined(PLATFORM_WEB)
	sActiveFileBrowser = this;
#endif
	if (ImGui::Begin("Fullscreen File Browser", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::SetWindowPos(ImVec2((float)FTX::screenWidth() * 0.05f, (float)FTX::screenHeight() * 0.01f), ImGuiCond_Always);
		ImGui::SetWindowSize(ImVec2((float)FTX::screenWidth() * 0.9f, (float)FTX::screenHeight() * 0.98f), ImGuiCond_Always);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("File Browser");

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 220 * mUIScale);
		const ImVec2 settingsPopupPos = ImGui::GetCursorScreenPos() + ImVec2(-120, 23) * mUIScale;
		if (ImGui::Button("Settings"))
		{
			ImGui::OpenPopup("FileBrowser_Settings");
		}

		ImGui::SameLineRelSpace(3.0f);
		if (ImGui::RedButton("Return to Options"))
		{
			mRemoveContentProvider = true;
		}

	#if defined(PLATFORM_HAS_MOUSE)
		ImGui::Text("Please use your mouse to interact with this window.");
	#elif defined(PLATFORM_HAS_TOUCH_INPUT)
		ImGui::Text("Please use touch input to interact with this window.");
	#else
		ImGui::Text("Please use a mouse or touch input to interact with this window.");
	#endif

		ImGui::Spacing();
		ImGui::Separator();

		ImGui::SetNextWindowPos(settingsPopupPos);
		if (ImGui::BeginPopup("FileBrowser_Settings"))
		{
			if (ImGui::DragFloat("UI Scale", &Configuration::instance().mDevMode.mUIScale, 0.003f, 0.5f, 4.0f, "<   %.1f   >"))
			{
				ImGuiIntegration::instance().refreshImGuiStyle();
			}

			ImGui::EndPopup();
		}

		buildWindowContent();

		DevModeWindowBase::allowDragScrolling();
	}
	ImGui::End();
}

void ImGuiFileBrowser::buildWindowContent()
{
	mUIScale = Configuration::instance().mDevMode.mUIScale;

	if (mBasePath.empty())
	{
		mBasePath = Configuration::instance().mAppDataPath;
		FTX::FileSystem->normalizePath(mBasePath, true);

		mLocalPath.clear();
		if (mStartInModsFolder)
		{
			// TODO: Use game-specific sub-path
			mLocalPath.push_back(L"mods");
		}
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
				const std::wstring filePath = mFullPath + WString(filename).toStdWString();
				if (FTX::FileSystem->exists(filePath))
				{
					// TODO: First show a confirmation dialog here?
					unmountModZipFile(filePath);
				}
				FTX::FileSystem->saveFile(filePath, fileSelection.mFileContent);

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

void ImGuiFileBrowser::updateFullPath()
{
	mFullPath = mBasePath;
	mIsReadOnlyLocation = false;

	for (const std::wstring& dir : mLocalPath)
	{
		mFullPath += dir + L"/";

		// Regard zip file content as read-only
		if (isZipFileName(dir))
			mIsReadOnlyLocation = true;
	}

	mRefreshFileEntries = true;
}

void ImGuiFileBrowser::refreshFileEntries()
{
	mDirectories.clear();
	mFileEntries.clear();
	FTX::FileSystem->listDirectories(mFullPath, mDirectories);
	FTX::FileSystem->listFiles(mFullPath, false, mFileEntries);

	// Filter out all ZIP directories
	for (size_t k = 0; k < mDirectories.size(); ++k)
	{
		const std::wstring& dirName = mDirectories[k];
		if (isZipFileName(dirName))
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

#if defined(PLATFORM_WEB)
	extern "C" EMSCRIPTEN_KEEPALIVE void ImGuiFileBrowser_OnWebImportDone()
	{
		if (sActiveFileBrowser != nullptr)
		{
			sActiveFileBrowser->refreshFileEntries();
		}
	}
#endif

void ImGuiFileBrowser::drawFileBrowser()
{
	const std::wstring* clickedDirectory = nullptr;
	const std::wstring* openActionsForDirectory = nullptr;
	const rmx::FileIO::FileEntry* openActionsForFile = nullptr;
	bool openCreateDirectoryDialog = false;

	ImGui::Text("  ");
	ImGui::SameLine();

	// "Refresh" button
	if (ImGui::Button("Refresh"))
	{
		mRefreshFileEntries = true;
	}

	ImGui::SameLine();
	ImGui::Text("  |  ");
	ImGui::SameLine();

	ImGui::BeginDisabled(mIsReadOnlyLocation);

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	if (ImGui::Button("New " PLATFORM_DIRECTORY_STRING))
	{
		openCreateDirectoryDialog = true;
		mActionsMenuPosition = cursorScreenPos;
		mActionsMenuPosition.y += mUIScale * 23.0f;
	}

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB) || defined(DEBUG)
	// "Import file" button
	ImGui::SameLine();
	if (ImGui::Button("Import file..."))
	{
	#if defined(PLATFORM_ANDROID)
		AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
		javaInterface.openFileSelectionDialog();
	#elif defined(PLATFORM_WEB)
		const std::string uploadPath = rmx::convertToUTF8(mFullPath);
		EM_ASM({
			window.__s3airFileUploadPath = UTF8ToString($0);
			triggerFileSelect();
		}, uploadPath.c_str());
	#endif
	}
#endif

	ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Spacing();

	// Address line
	drawAddressLine();

	ImGui::Spacing();

	// Refresh directories & file entries if needed
	if (mRefreshFileEntries)
	{
		refreshFileEntries();
	}

	ImGui::PushStyleVarY(ImGuiStyleVar_CellPadding, 5.0f * mUIScale);
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImGuiHelpers::getAccentColorMix(1.0f, 0.1f, 0.05f));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImGuiHelpers::getAccentColorMix(1.0f, 0.05f, 0.05f));

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

		if (mDirectories.empty() && mFileEntries.empty())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextColored(ImGuiHelpers::COLOR_GRAY60, " - Empty " PLATFORM_DIRECTORY_STRING " -");
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
	drawCreateDirectoryPopup(openCreateDirectoryDialog);
}

void ImGuiFileBrowser::drawAddressLine()
{
	// Draw background frame
	const float rectWidth = ImGui::GetWindowWidth() - 32 * mUIScale;
	const ImVec2 rectStart = ImGui::GetCursorScreenPos();
	const ImVec2 rectEnd = rectStart + ImVec2(rectWidth, 29 * mUIScale);
	ImGui::GetWindowDrawList()->AddRectFilled(rectStart, rectEnd, ImGui::ColorConvertFloat4ToU32(ImGuiHelpers::getAccentColorMix(0.25f, 0.5f, 0.25f)), 6 * mUIScale);
	ImGui::GetWindowDrawList()->AddRect(rectStart, rectEnd, ImGui::ColorConvertFloat4ToU32(ImGuiHelpers::getAccentColorMix(0.75f, 0.5f, 0.75f)), 6 * mUIScale);

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

	ImGui::PushStyleColor(ImGuiCol_Button,        ImGuiHelpers::getAccentColorMix(1.0f, 0.8f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiHelpers::getAccentColorMix(1.0f, 0.8f, 0.4f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGuiHelpers::getAccentColorMix(1.0f, 0.8f, 0.8f));
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
	ImGui::PopStyleColor(3);

	ImGui::PopClipRect();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
}

void ImGuiFileBrowser::drawActionsMenu(bool openMenuNow)
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
			// Don't allow modifications for read-only locations, and for the mods base directory
			// TODO: Protect other directories in the same way...?
			const bool canModify = !mIsReadOnlyLocation && (mFullPath + *mOpenActionsForDirectory + L'/') != ModManager::instance().getModsBasePath();

			ImGui::Image(ImGuiHelpers::getTextureRef(mFolderIconTexture), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::TextColored(FOLDER_COLOR, "%s", rmx::convertToUTF8(*mOpenActionsForDirectory).c_str());

			ImGui::Separator();

			ImGui::BeginDisabled(!canModify);
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
			ImGui::EndDisabled();

		#if defined(OPEN_EXTERNAL_TEXT)		// Only for platforms that support it
			ImGui::Separator();
			if (ImGui::Button(OPEN_EXTERNAL_TEXT, BUTTON_SIZE))
			{
				PlatformFunctions::openDirectoryExternal(mFullPath + *mOpenActionsForDirectory);
				ImGui::CloseCurrentPopup();
			}
		#endif
		}

		moveWindowToFitOnScreen(mActionsMenuPosition, mUIScale);
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
			const bool canModify = !mIsReadOnlyLocation;

			ImGui::Image(ImGuiHelpers::getTextureRef(getFileIcon(mOpenActionsForFile->mFilename)), ImVec2(16, 16) * mUIScale);
			ImGui::SameLine();
			ImGui::Text("%s", rmx::convertToUTF8(mOpenActionsForFile->mFilename).c_str());

			if (isZipFileName(mOpenActionsForFile->mFilename) && rmx::startsWith(mFullPath, ModManager::instance().getModsBasePath()))	// Second check is needed because mod manager won't mount zips outside of the mods base directory
			{
				if (ImGui::Button("Open zip file", BUTTON_SIZE))
				{
					// Make sure the zip file is mounted
					mountModZipFile(mFullPath + mOpenActionsForFile->mFilename);
					mLocalPath.push_back(mOpenActionsForFile->mFilename);
					updateFullPath();
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::Separator();

			ImGui::BeginDisabled(!canModify);
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
			ImGui::EndDisabled();

		#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
			ImGui::Separator();

			if (ImGui::Button("Export file...", BUTTON_SIZE))
			{
				#if defined(PLATFORM_ANDROID)
				std::vector<uint8> contents;
				if (FTX::FileSystem->readFile(mOpenActionsForFile->mPath + mOpenActionsForFile->mFilename, contents))
				{
					AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
					javaInterface.openFileExportDialog(mOpenActionsForFile->mFilename, contents);
				}
				#elif defined(PLATFORM_WEB)
				const std::wstring filePath = mOpenActionsForFile->mPath + mOpenActionsForFile->mFilename;
				const std::string filePathUtf8 = rmx::convertToUTF8(filePath);
				EM_ASM({
					window.fileManagerExportFile(UTF8ToString($0));
				}, filePathUtf8.c_str());
				#endif
				ImGui::CloseCurrentPopup();
			}
		#endif
		}

		moveWindowToFitOnScreen(mActionsMenuPosition, mUIScale);
		ImGui::EndPopup();
	}

	drawConfirmDeletionPopup(openConfirmDeletionPopup);
	drawRenamingPopup(openRenamingPopup);
}

void ImGuiFileBrowser::drawConfirmDeletionPopup(bool openPopupNow)
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
			ImGui::Text(nullptr != mOpenActionsForFile ? "Really delete file " : "Really delete " PLATFORM_DIRECTORY_STRING " ");
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

			// "Yes" button
			if (ImGui::RedButton("Yes", ImVec2(100 * mUIScale, 0)))
			{
				performDeletion = true;
				ImGui::CloseCurrentPopup();
			}

			// "No" button
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
		bool success = true;
		if (nullptr != mOpenActionsForFile)
		{
			const std::wstring fullFilePath = currentPath + mOpenActionsForFile->mFilename;
			unmountModZipFile(fullFilePath);	// Just in case it's a mounted mod zip, we need to unmount it first
			success = FTX::FileSystem->removeFile(fullFilePath);
		}
		else if (nullptr != mOpenActionsForDirectory)
		{
			success = FTX::FileSystem->removeDirectory(currentPath + *mOpenActionsForDirectory);
		}

		RMX_CHECK(success, "Deletion failed with error: " << FTX::FileSystem->getLastErrorCode().message(), );
		mRefreshFileEntries = true;
	}
}

void ImGuiFileBrowser::drawRenamingPopup(bool openPopupNow)
{
	static ImGuiHelpers::WideInputString newNameInput;
	bool performRenaming = false;

#if defined(PLATFORM_WEB)
	if (openPopupNow)
	{
		if (nullptr == mOpenActionsForFile && nullptr == mOpenActionsForDirectory)
		{
			return;
		}

		const bool isFile = (nullptr != mOpenActionsForFile);
		const std::wstring oldName = isFile ? mOpenActionsForFile->mFilename : *mOpenActionsForDirectory;
		const std::string oldNameUtf8 = rmx::convertToUTF8(oldName);
		const std::string currentPathUtf8 = rmx::convertToUTF8(mFullPath);
		EM_ASM({
			window.fileManagerPromptRename(UTF8ToString($0), UTF8ToString($1), $2);
		}, oldNameUtf8.c_str(), currentPathUtf8.c_str(), isFile ? 1 : 0);
		return;
	}
#endif

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
			ImGui::Text(nullptr != mOpenActionsForFile ? "Rename file " : "Rename " PLATFORM_DIRECTORY_STRING " ");
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
			ImGuiHelpers::InputText("##NewName", newNameInput, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter, &filterFileNameInput);

			ImGui::Separator();

			// Center the buttons
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max<float>(ImGui::GetContentRegionAvail().x - 208 * mUIScale, 0) / 2);

			// Buttons
			ImGui::BeginDisabled(!rmx::FileIO::isValidFileName(newNameInput.mWideString));
			if (ImGui::Button("Rename", ImVec2(100 * mUIScale, 0)))
			{
				performRenaming = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
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

		moveWindowToFitOnScreen(mActionsMenuPosition, mUIScale);
		ImGui::EndPopup();
	}

	if (performRenaming)
	{
		const std::wstring currentPath = mFullPath;
		bool success = true;
		if (nullptr != mOpenActionsForFile)
		{
			const std::wstring oldName = mOpenActionsForFile->mFilename;
			const std::wstring newName = newNameInput.get().toStdWString();
			if (oldName != newName)
			{
				const std::wstring oldFullFilePath = currentPath + oldName;
				const std::wstring newFullFilePath = currentPath + newName;
				unmountModZipFile(oldFullFilePath);		// Just in case it's a mounted mod zip, we need to unmount it first
				success = FTX::FileSystem->renameFile(oldFullFilePath, newFullFilePath);
			}
		}
		else if (nullptr != mOpenActionsForDirectory)
		{
			const std::wstring oldName = *mOpenActionsForDirectory;
			const std::wstring newName = newNameInput.get().toStdWString();
			if (oldName != newName)
			{
				success = FTX::FileSystem->renameDirectory(currentPath + oldName, currentPath + newName);
			}
		}

		RMX_CHECK(success, "Renaming failed", );
		mRefreshFileEntries = true;
	}
}

void ImGuiFileBrowser::drawCreateDirectoryPopup(bool openPopupNow)
{
	bool performCreation = false;
	static ImGuiHelpers::WideInputString directoryNameInput;

	if (openPopupNow)
	{
		ImGui::OpenPopup("FileBrowser_CreateDirectory");
	}

	ImGui::SetNextWindowPos(mActionsMenuPosition);
	if (ImGui::BeginPopup("FileBrowser_CreateDirectory"))
	{
		ImGui::Text("Name of new " PLATFORM_DIRECTORY_STRING ":");

		if (openPopupNow)
		{
			ImGui::SetKeyboardFocusHere();
		}
		ImGuiHelpers::InputText("##NewName", directoryNameInput, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter, &filterFileNameInput);

		ImGui::Separator();

		// Center the buttons
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max<float>(ImGui::GetContentRegionAvail().x - 208 * mUIScale, 0) / 2);

		// Buttons
		ImGui::BeginDisabled(!rmx::FileIO::isValidFileName(directoryNameInput.mWideString));
		if (ImGui::Button("Create", ImVec2(100 * mUIScale, 0)))
		{
			performCreation = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100 * mUIScale, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		moveWindowToFitOnScreen(mActionsMenuPosition, mUIScale);
		ImGui::EndPopup();
	}

	if (performCreation)
	{
		const std::wstring fullPath = mFullPath + directoryNameInput.get().toStdWString();
		bool success = FTX::FileSystem->createDirectory(fullPath);

		RMX_CHECK(success, "Failed to create new " << PLATFORM_DIRECTORY_STRING, );
		mRefreshFileEntries = true;
	}
}

DrawerTexture& ImGuiFileBrowser::getFileIcon(const std::wstring& filename)
{
	if (isZipFileName(filename))
	{
		return mZipIconTexture;
	}
	return mFileIconTexture;
}

#endif
