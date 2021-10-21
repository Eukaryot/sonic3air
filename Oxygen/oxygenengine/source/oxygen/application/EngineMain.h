/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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
class GameProfile;
class ControlsIn;
class InputManager;
class ModManager;
class VideoOut;
class ResourcesCache;
class LogDisplay;
class PersistentData;
#if defined (PLATFORM_ANDROID)
	class AndroidJavaInterface;
#endif

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
		int			 mWindowsIconResource;
		std::string	 mBuildVersion;
		std::wstring mAppDataFolder;
	};

public:
	virtual const AppMetaData& getAppMetaData() = 0;
	virtual GuiBase& createGameApp() = 0;
	virtual AudioOutBase& createAudioOut() = 0;

	virtual bool onEnginePreStartup() = 0;
	virtual bool setupCustomGameProfile() = 0;

	virtual void startupGame() = 0;
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

	inline AudioOutBase& getAudioOut() { return *mAudioOut; }
	inline ControlsIn& getControlsIn() { return mControlsIn; }

	inline SDL_Window& getSDLWindow() const	{ return *mSDLWindow; }
	inline Drawer& getDrawer()				{ return mDrawer; }

	uint32 getPlatformFlags() const;
	void switchToRenderMethod(Configuration::RenderMethod newRenderMethod);
	void setVSyncMode(int mode);

private:
	bool startupEngine();
	void run();
	void shutdown();

	bool initConfigAndSettings(const std::wstring& argumentProjectPath);
	bool initFileSystem();

	bool createWindow();
	void destroyWindow();

private:
	EngineDelegateInterface& mDelegate;
	std::vector<std::string> mArguments;

	GameProfile&	mGameProfile;
	InputManager&	mInputManager;
	LogDisplay&		mLogDisplay;
	ModManager&		mModManager;
	ResourcesCache&	mResourcesCache;
	PersistentData&	mPersistentData;

	VideoOut&		mVideoOut;
	AudioOutBase*   mAudioOut = nullptr;
	ControlsIn&		mControlsIn;

	SDL_Window*		mSDLWindow = nullptr;
	Drawer			mDrawer;

#if defined(PLATFORM_ANDROID)
	AndroidJavaInterface& mAndroidJavaInterface;
#endif
};
