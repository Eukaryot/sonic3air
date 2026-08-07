/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/devmode/DevModeWindowBase.h"


class DevModeMainWindow : public DevModeWindowBase
{
public:
	static const uint64 PROVIDER_KEY = rmx::constMurmur2_64("DevModeMainWindow");

public:
	DevModeMainWindow();
	~DevModeMainWindow();

	virtual bool buildWindow() override;
	virtual void buildContent() override;

	void openWatchesWindow();

private:
	template<typename T>
	void createWindow(T*& output)
	{
		output = new T();
		mAllWindows.push_back(output);
		output->mDevModeMainWindow = this;
	}

private:
	DevModeWindowBase* findWindowByTitle(const std::string& title);

private:
	std::vector<DevModeWindowBase*> mAllWindows;

	class AppDebugWindow* mAppDebugWindow = nullptr;
	class AudioBrowserWindow* mAudioBrowserWindow = nullptr;
	class AudioPlaybackWindow* mAudioPlaybackWindow = nullptr;
	class CallFramesWindow* mCallFramesWindow = nullptr;
	class CustomSidePanelWindow* mCustomSidePanelWindow = nullptr;
	class DebugLogWindow* mDebugLogWindow = nullptr;
	class FileBrowserWindow* mFileBrowserWindow = nullptr;
	class GameSimWindow* mGameSimWindow = nullptr;
	class GameVisualizationsWindow* mGameVisualizationsWindow = nullptr;
	class MemoryHexViewWindow* mMemoryHexViewWindow = nullptr;
	class NetworkingWindow* mNetworkingWindow = nullptr;
	class PaletteBrowserWindow* mPaletteBrowserWindow = nullptr;
	class PaletteViewWindow* mPaletteViewWindow = nullptr;
	class PersistentDataWindow* mPersistentDataWindow = nullptr;
	class RenderedGeometryWindow* mRenderedGeometryWindow = nullptr;
	class ScriptBuildWindow* mScriptBuildWindow = nullptr;
	class SettingsWindow* mSettingsWindow = nullptr;
	class SpriteBrowserWindow* mSpriteBrowserWindow = nullptr;
	class VRAMWritesWindow* mVRAMWritesWindow = nullptr;
	class WatchesWindow* mWatchesWindow = nullptr;

	int mActiveTab = -1;
};

#endif
