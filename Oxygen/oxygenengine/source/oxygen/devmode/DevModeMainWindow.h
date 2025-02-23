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

class AudioBrowserWindow;
class CallFramesWindow;
class CustomSidePanelWindow;
class DebugLogWindow;
class GameSimWindow;
class GameVisualizationsWindow;
class MemoryHexViewWindow;
class NetworkingWindow;
class PaletteBrowserWindow;
class PaletteViewWindow;
class PersistentDataWindow;
class RenderedGeometryWindow;
class ScriptBuildWindow;
class SettingsWindow;
class SpriteBrowserWindow;
class VRAMWritesWindow;
class WatchesWindow;


class DevModeMainWindow : public DevModeWindowBase
{
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

	AudioBrowserWindow* mAudioBrowserWindow = nullptr;
	CallFramesWindow* mCallFramesWindow = nullptr;
	CustomSidePanelWindow* mCustomSidePanelWindow = nullptr;
	DebugLogWindow* mDebugLogWindow = nullptr;
	GameSimWindow* mGameSimWindow = nullptr;
	GameVisualizationsWindow* mGameVisualizationsWindow = nullptr;
	MemoryHexViewWindow* mMemoryHexViewWindow = nullptr;
	NetworkingWindow* mNetworkingWindow = nullptr;
	PaletteBrowserWindow* mPaletteBrowserWindow = nullptr;
	PaletteViewWindow* mPaletteViewWindow = nullptr;
	PersistentDataWindow* mPersistentDataWindow = nullptr;
	RenderedGeometryWindow* mRenderedGeometryWindow = nullptr;
	ScriptBuildWindow* mScriptBuildWindow = nullptr;
	SettingsWindow* mSettingsWindow = nullptr;
	SpriteBrowserWindow* mSpriteBrowserWindow = nullptr;
	VRAMWritesWindow* mVRAMWritesWindow = nullptr;
	WatchesWindow* mWatchesWindow = nullptr;

	bool mShowImGuiDemo = false;
	int mActiveTab = -1;
};

#endif
