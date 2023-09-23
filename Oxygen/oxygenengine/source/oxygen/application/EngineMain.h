/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"
#include "oxygen/drawing/Drawer.h"

class AudioOutBase;
class CodeExec;
class Configuration;
class EmulatorInterface;
class LogDisplay;
class PackedFileProvider;

namespace lemon
{
	class Module;
	class Program;
}


class EngineDelegateInterface
{
public:
	struct AppMetaData
	{
		std::string  mTitle;
		std::wstring mIconFile;
		int			 mWindowsIconResource = 0;
		std::string	 mBuildVersionString;
		uint32		 mBuildVersionNumber;
		std::wstring mAppDataFolder;
	};

public:
	virtual const AppMetaData& getAppMetaData() = 0;
	virtual GuiBase& createGameApp() = 0;
	virtual AudioOutBase& createAudioOut() = 0;

	virtual bool onEnginePreStartup() = 0;
	virtual bool setupCustomGameProfile() = 0;

	virtual void startupGame(EmulatorInterface& emulatorInterface) = 0;
	virtual void shutdownGame() = 0;
	virtual void updateGame(float timeElapsed) = 0;

	virtual void registerScriptBindings(lemon::Module& module) = 0;
	virtual void registerNativizedCode(lemon::Program& program) = 0;

	virtual void onRuntimeInit(CodeExec& codeExec) = 0;
	virtual void onPreFrameUpdate() = 0;
	virtual void onPostFrameUpdate() = 0;
	virtual void onControlsUpdate() = 0;
	virtual void onPreSaveStateLoad() = 0;

	virtual void onApplicationLostFocus() {}

	virtual bool mayLoadScriptMods() = 0;
	virtual bool allowModdedData() = 0;
	virtual bool useDeveloperFeatures() = 0;
	virtual void onActiveModsChanged() = 0;

	virtual void onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer) = 0;
	virtual void onGameRecordingHeaderSave(std::vector<uint8>& buffer) = 0;

	virtual Font& getDebugFont(int size) = 0;
	virtual void fillDebugVisualization(Bitmap& bitmap, int& mode) = 0;
};


class EngineMain : public SingleInstance<EngineMain>
{
public:
	static EngineDelegateInterface& getDelegate()  { return EngineMain::instance().mDelegate; }
	static void earlySetup();

public:
	EngineMain(EngineDelegateInterface& delegate_);
	~EngineMain();

	void execute(int argc, char** argv);

	void onActiveModsChanged();
	bool reloadFilePackage(std::wstring_view packageName, bool forceReload);

	inline AudioOutBase& getAudioOut() { return *mAudioOut; }

	inline SDL_Window& getSDLWindow() const	{ return *mSDLWindow; }
	inline Drawer& getDrawer()				{ return mDrawer; }

	uint32 getPlatformFlags() const;
	void switchToRenderMethod(Configuration::RenderMethod newRenderMethod);
	void setVSyncMode(Configuration::FrameSyncType frameSyncMode);

private:
	bool startupEngine();
	void run();
	void shutdown();

	void initDirectories();
	bool initConfigAndSettings(const std::wstring& argumentProjectPath);
	bool initFileSystem();
	bool loadFilePackages(bool forceReload);
	bool loadFilePackageByIndex(size_t index, bool forceReload);

	bool createWindow();
	void destroyWindow();

private:
	EngineDelegateInterface& mDelegate;
	std::vector<std::string> mArguments;

	struct Internal;
	Internal& mInternal;

	AudioOutBase*	mAudioOut = nullptr;
	SDL_Window*		mSDLWindow = nullptr;
	Drawer			mDrawer;
	std::vector<PackedFileProvider*> mPackedFileProviders;
};
