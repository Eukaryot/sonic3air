/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/DevModeMainWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/devmode/windows/AudioBrowserWindow.h"
#include "oxygen/devmode/windows/CallFramesWindow.h"
#include "oxygen/devmode/windows/CustomSidePanelWindow.h"
#include "oxygen/devmode/windows/DebugLogWindow.h"
#include "oxygen/devmode/windows/GameSimWindow.h"
#include "oxygen/devmode/windows/GameVisualizationsWindow.h"
#include "oxygen/devmode/windows/MemoryHexViewWindow.h"
#include "oxygen/devmode/windows/NetworkingWindow.h"
#include "oxygen/devmode/windows/PaletteBrowserWindow.h"
#include "oxygen/devmode/windows/PaletteViewWindow.h"
#include "oxygen/devmode/windows/RenderedGeometryWindow.h"
#include "oxygen/devmode/windows/ScriptBuildWindow.h"
#include "oxygen/devmode/windows/SettingsWindow.h"
#include "oxygen/devmode/windows/SpriteBrowserWindow.h"
#include "oxygen/devmode/windows/VRAMWritesWindow.h"
#include "oxygen/devmode/windows/WatchesWindow.h"

#if defined(PLATFORM_ANDROID)
	// Just for testing
	#include "oxygen/platform/AndroidJavaInterface.h"
#endif


DevModeMainWindow::DevModeMainWindow() :
	DevModeWindowBase("Dev Mode (F1)", Category::MISC, ImGuiWindowFlags_AlwaysAutoResize)
{
	const Configuration::DevModeSettings& config = Configuration::instance().mDevMode;
	mIsWindowOpen = config.mMainWindowOpen;

	// Create windows
	//  -> Note that the order of creation defines the listing order inside each category
	{
		createWindow(mGameSimWindow);
		createWindow(mCallFramesWindow);
		createWindow(mScriptBuildWindow);
		createWindow(mMemoryHexViewWindow);
		createWindow(mWatchesWindow);
		createWindow(mDebugLogWindow);

		createWindow(mGameVisualizationsWindow);
		createWindow(mPaletteViewWindow);
		createWindow(mRenderedGeometryWindow);
		createWindow(mVRAMWritesWindow);
		createWindow(mSpriteBrowserWindow);
		createWindow(mPaletteBrowserWindow);

		createWindow(mAudioBrowserWindow);
		createWindow(mCustomSidePanelWindow);
		createWindow(mSettingsWindow);
	#ifdef DEBUG
		createWindow(mNetworkingWindow);
	#endif
	}

	// Open windows depending on what was saved in settings
	for (const std::string& windowTitle : config.mOpenUIWindows)
	{
		DevModeWindowBase* window = findWindowByTitle(windowTitle);
		if (nullptr != window)
			window->mIsWindowOpen = true;
	}
}

DevModeMainWindow::~DevModeMainWindow()
{
	// Save which windows are open
	{
		Configuration::DevModeSettings& config = Configuration::instance().mDevMode;
		config.mMainWindowOpen = mIsWindowOpen;

		std::vector<std::string>& windowTitles = config.mOpenUIWindows;
		windowTitles.clear();
		for (DevModeWindowBase* window : mAllWindows)
		{
			if (window->mIsWindowOpen)
				windowTitles.push_back(window->mTitle);
		}
	}

	// Destroy window instances
	for (DevModeWindowBase* window : mAllWindows)
	{
		delete window;
	}
}

bool DevModeMainWindow::buildWindow()
{
	const bool result = DevModeWindowBase::buildWindow();

	for (DevModeWindowBase* window : mAllWindows)
	{
		window->buildWindow();
	}

	// ImGui demo window for testing
	if (mShowImGuiDemo)
		ImGui::ShowDemoWindow();

	return result;
}

void DevModeMainWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(150.0f, 200.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowCollapsed(true, ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	const bool useTabs = Configuration::instance().mDevMode.mUseTabsInMainWindow;
	int& configActiveTab = Configuration::instance().mDevMode.mActiveMainWindowTab;
	const bool firstRun = (mActiveTab == -1);

	const char* TEXT_BY_CATEGORY[] =
	{
		"Simulation",
		"Graphics",
		"Misc"
	};
	constexpr int NUM_CATEGORIES = (int)DevModeWindowBase::Category::MISC + 1;
	static_assert(sizeof(TEXT_BY_CATEGORY) / sizeof(const char*) == NUM_CATEGORIES);

	if (!useTabs || ImGui::BeginTabBar("Tab Bar", 0))
	{
		std::vector<DevModeWindowBase*> windows;

		for (int categoryIndex = 0; categoryIndex < NUM_CATEGORIES; ++categoryIndex)
		{
			windows.clear();
			for (DevModeWindowBase* window : mAllWindows)
			{
				if (window->mCategory == (DevModeWindowBase::Category)categoryIndex && window->shouldBeAvailable())
				{
					windows.push_back(window);
				}
			}

		#ifdef DEBUG
			if (categoryIndex == NUM_CATEGORIES - 1)
			{
				// For ImGui demo
				windows.push_back(nullptr);
			}
		#endif

			if (!windows.empty())
			{
				const size_t itemsPerColumn = std::max<size_t>(3, (windows.size() + 1) / 2);
				const size_t numColumns = (windows.size() + itemsPerColumn - 1) / itemsPerColumn;
				const int flags = (firstRun && configActiveTab == categoryIndex) ? ImGuiTabItemFlags_SetSelected : 0;

				if (!useTabs)
					ImGui::SeparatorText(TEXT_BY_CATEGORY[categoryIndex]);

				if (!useTabs || ImGui::BeginTabItem(TEXT_BY_CATEGORY[categoryIndex], nullptr, flags))
				{
					mActiveTab = categoryIndex;

					if (ImGui::BeginTable("Table", numColumns, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp, ImVec2(300 * uiScale, 0)))
					{
						for (size_t k = 0; k < numColumns; ++k)
							ImGui::TableSetupColumn("", 0, 1.0f);

						ImGui::TableNextRow();

						for (size_t k = 0; k < windows.size(); ++k)
						{
							if ((k % itemsPerColumn) == 0)
							ImGui::TableSetColumnIndex(k / itemsPerColumn);

							DevModeWindowBase* window = windows[k];
							if (nullptr != window)
							{
								ImGui::Checkbox(window->mTitle.c_str(), &window->mIsWindowOpen);
							}
						#ifdef DEBUG
							else
							{
								ImGui::Checkbox("ImGui Demo", &mShowImGuiDemo);
							}
						#endif
						}

						ImGui::EndTable();
					}

					if (useTabs)
						ImGui::EndTabItem();
				}
			}
		}
		
		if (useTabs)
			ImGui::EndTabBar();
	}

	configActiveTab = mActiveTab;

#if defined(DEBUG) && 0
	// Just for debugging
	ImGui::Separator();
	ImGui::Text("ImGui Capture:   %s %s", ImGui::GetIO().WantCaptureMouse ? "[M]" : "      ", ImGui::GetIO().WantCaptureKeyboard ? "[K]" : "");
#endif

#if defined(PLATFORM_ANDROID) && 0
	// Test for Android folder picker
	if (ImGui::Button("Select Folder"))
	{
		AndroidJavaInterface::instance().openFolderAccessDialog();
	}

	static bool useSDL = true;
	static std::string outputText;

	const std::string path = "/tree/primary:S3AIR_Savedata";	// TODO: Use the picked result instead
	if (ImGui::Button("Write File"))
	{
		if (useSDL)
		{
			SDL_RWops* file = SDL_RWFromFile((path + "/written_by_cpp.txt").c_str(), "w");
			if (nullptr != file)
			{
				const size_t bytesWritten = SDL_RWwrite(file, "Hello SDL!", 11, 1);
				SDL_RWclose(file);
				outputText = "File write successful, " + std::to_string(bytesWritten) + " bytes written";
			}
			else
				outputText = "File write failed!";
		}
		else
		{
			String str = "Hello World!";
			if (str.saveFile(path + "/written_by_cpp.txt"))
				outputText = "File write successful";
			else
				outputText = "File write failed!";
		}
	}

	std::string filenameToRead;
	ImGui::SameLine();
	if (ImGui::Button("Read File"))
		filenameToRead = "written_by_cpp.txt";
	ImGui::SameLine();
	if (ImGui::Button("Read Custom"))
		filenameToRead = "custom.txt";

	if (!filenameToRead.empty())
	{
		if (useSDL)
		{
			SDL_RWops* file = SDL_RWFromFile((path + "/" + filenameToRead).c_str(), "r");
			if (nullptr != file)
			{
				char buffer[1024];
				const size_t bytesRead = SDL_RWread(file, &buffer, 1023, 1);
				buffer[bytesRead] = 0;
				SDL_RWclose(file);
				outputText = std::string("File read with content: ") + buffer;
			}
			else
				outputText = "File read failed!";
		}
		else
		{
			String str;
			if (str.loadFile(path + "/custom.txt", UnicodeEncoding::AUTO))
				outputText = "File read with content: " + str.toStdString();
			else
				outputText = "File read failed!";
		}
	}

	ImGui::Checkbox("Use SDL RW", &useSDL);
	ImGui::Text("%s", outputText.c_str());
#endif
}

void DevModeMainWindow::openWatchesWindow()
{
	mWatchesWindow->mIsWindowOpen = true;
}

DevModeWindowBase* DevModeMainWindow::findWindowByTitle(const std::string& title)
{
	for (DevModeWindowBase* window : mAllWindows)
	{
		if (window->mTitle == title)
			return window;
	}
	return nullptr;
}

#endif
