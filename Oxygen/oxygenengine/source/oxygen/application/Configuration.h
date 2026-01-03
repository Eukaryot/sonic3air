/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputConfig.h"

#include <lemon/compiler/PreprocessorDefinition.h>

class JsonSerializer;


class Configuration
{
public:
	enum class RenderMethod
	{
		UNDEFINED	= 0x00,
		SOFTWARE	= 0x10,
		OPENGL_SOFT	= 0x20,
		OPENGL_FULL	= 0x21
	};

	enum class WindowMode
	{
		WINDOWED,					// Windowed mode
		FULLSCREEN_BORDERLESS,		// Borderless fullscreen window
		FULLSCREEN_DESKTOP,			// Fullscreen window with Desktop resolution
		FULLSCREEN_EXCLUSIVE		// Real exclusive fullscreen
	};

	enum class FrameSyncType
	{
		VSYNC_OFF,
		VSYNC_ON,
		VSYNC_FRAMECAP,
		FRAME_INTERPOLATION,
		_NUM
	};

	struct GameServerBase
	{
		std::string mServerHostName;
		int mServerPortUDP = 21094;		// Used by most platforms
		int mServerPortTCP = 21095;		// Used only as a fallback for UDP
		int mServerPortWSS = 21096;		// Used by the web version
	};

	struct ExternalCodeEditor
	{
		std::string mActiveType;			// Can be "custom", "vscode", "npp", or empty
		std::wstring mVisualStudioCodePath;
		std::wstring mNotepadPlusPlusPath;
		std::wstring mCustomEditorPath;
		std::wstring mCustomEditorArgs = L"--file \"{file}\" --line {line}";
	};

	struct DevModeSettings
	{
		bool mEnabled = false;			// Set if dev mode is currently enabled
		bool mEnableAtStartup = false;	// Set if dev mode is meant to be enabled at startup
		float mGameViewScale = 1.0f;
		Vec2f mGameViewAlignment;
		float mUIScale = 1.0f;
		Color mUIAccentColor = Color(0.2f, 0.5f, 0.8f);
		bool mScrollByDragging = true;
		std::vector<std::string> mOpenUIWindows;
		bool mMainWindowOpen = true;
		bool mUseTabsInMainWindow = true;
		int mActiveMainWindowTab = 0;
		ExternalCodeEditor mExternalCodeEditor;
		bool mApplyModSettingsAfterLoadState = false;
	};

	struct AudioSettings
	{
		float mMasterVolume = 1.0f;
		float mMusicVolume = 0.8f;
		float mSoundVolume = 0.8f;
		int   mSampleRate = 48000;
		bool  mUseAudioThreading = true;		// Disabled in constructor for platforms that don't support it
	};

	struct GameRecorder
	{
		int mRecordingMode = -1;		// -1 = Auto, 0 = Recording disabled, 1 = Recording enabled
		bool mEnablePlayback = false;
		int mPlaybackStartFrame = 0;
		bool mPlaybackIgnoreKeys = false;
	};

	struct VirtualGamepad
	{
		float mOpacity = 0.8f;
		Vec2i mDirectionalPadCenter;
		int   mDirectionalPadSize = 100;
		Vec2i mFaceButtonsCenter;
		int   mFaceButtonsSize = 100;
		Vec2i mStartButtonCenter;
		Vec2i mGameRecButtonCenter;
		Vec2i mShoulderLButtonCenter;
		Vec2i mShoulderRButtonCenter;
	};

	struct Mod
	{
		struct Setting
		{
			std::string mIdentifier;
			uint32 mValue = 0;
		};

		std::string mModName;
		std::map<uint64, Setting> mSettings;
	};

	enum class SettingsType
	{
		STANDARD = 0,	// "settings.json"
		INPUT = 1,		// "settings_input.json"
	};

	static const int NUM_PLAYERS = 4;

public:
	inline static bool hasInstance()		 { return (nullptr != mSingleInstance); }
	inline static Configuration& instance()  { return *mSingleInstance; }

	static RenderMethod getHighestSupportedRenderMethod();

public:
	Configuration();

	void initialization();
	bool loadConfiguration(const std::wstring& filename);
	bool loadSettings(const std::wstring& filename, SettingsType settingsType);
	void saveSettings();

	inline void setSettingsReadOnly(bool enable)  { mSettingsReadOnly = enable; }

protected:
	virtual void preLoadInitialization() = 0;
	virtual bool loadConfigurationInternal(JsonSerializer& jsonSerializer) = 0;
	virtual bool loadSettingsInternal(JsonSerializer& jsonSerializer, SettingsType settingsType) = 0;
	virtual void saveSettingsInternal(JsonSerializer& jsonSerializer, SettingsType settingsType) = 0;

private:
	void loadConfigurationProperties(JsonSerializer& jsonSerializer);

	void serializeStandardSettings(JsonSerializer& serializer);
	void serializeDevMode(JsonSerializer& serializer);

	void saveSettingsInput(const std::wstring& filename) const;

public:
	// Paths
	std::wstring mProjectPath;				// Only used in Engine App
	std::wstring mExePath;
	std::wstring mAppDataPath;				// App data path for the engine
	std::wstring mGameAppDataPath;			// App data path for the game; can be the same as the app data path for the engine, or a sub-folder of it
	std::wstring mSettingsFilenames[2];		// Uses SettingsType as key
	std::wstring mEngineDataPath;
	std::wstring mGameDataPath;
	std::wstring mRomPath;					// From configuration
	std::wstring mLastRomPath;				// From settings
	std::wstring mScriptsDir;
	std::wstring mMainScriptName;
	lemon::PreprocessorDefinitionMap mPreprocessorDefinitions;
	std::wstring mSaveStatesDir;			// Save states dir in the installation
	std::wstring mSaveStatesDirLocal;		// Save states dir in app data, specific for the game profile
	std::wstring mAnalysisDir;
	std::wstring mPersistentDataBasePath;

	// General
	bool   mFailSafeMode = false;
	int	   mPlatformFlags = -1;
	int    mNumPlayers = 2;			// Can be up to InputManager::NUM_PLAYERS = 4

	// Game
	std::wstring mLoadSaveState;
	int	 mLoadLevel = -1;
	int	 mUseCharacters = 1;
	int  mStartPhase = 0;
	int  mSimulationFrequency = 60;
	GameRecorder mGameRecorder;

	// Dev mode
	DevModeSettings mDevMode;

	// Video
	WindowMode mWindowMode = WindowMode::WINDOWED;
#if defined(PLATFORM_VITA)
	Vec2i mWindowSize = Vec2i(960, 544);
#else
	Vec2i mWindowSize = Vec2i(1200, 672);
#endif
	Vec2i mGameScreen = Vec2i(400, 224);
	int   mDisplayIndex = 0;
	RenderMethod mRenderMethod = RenderMethod::UNDEFINED;
	bool  mAutoDetectRenderMethod = true;
	FrameSyncType mFrameSync = FrameSyncType::VSYNC_ON;
	int   mUpscaling = 0;
	int   mBackdrop = 0;
	int   mFiltering = 0;
	int   mScanlines = 0;
	int   mBackgroundBlur = 0;
	int   mPerformanceDisplay = 0;

	// Audio
	AudioSettings mAudio;

	// Input
	std::vector<InputConfig::DeviceDefinition> mInputDeviceDefinitions;
	VirtualGamepad mVirtualGamepad;
	std::string mPreferredGamepad[NUM_PLAYERS];
	int mAutoAssignGamepadPlayerIndex = 0;	// Default is player 1 (who has index 0)
	float mControllerRumbleIntensity[NUM_PLAYERS] = { 0 };

	// Input recorder
	std::wstring mInputRecorderInput;
	std::wstring mInputRecorderOutput;

	// Misc
	bool mMirrorMode = false;

	// Game server
	GameServerBase mGameServerBase;

	// Internal
	bool mForceCompileScripts = false;
	int mScriptOptimizationLevel = -1;		// -1: Auto, 0: No optimization at all, up to 3: Full optimization
	std::wstring mCompiledScriptSavePath;
	bool mEnableROMDataAnalyser = false;
	bool mExitAfterScriptLoading = false;
	int mRunScriptNativization = 0;			// 0: Disabled, 1: Run nativization, 2: Nativization done
	std::wstring mScriptNativizationOutput;
	std::wstring mDumpCppDefinitionsOutput;

	// Mod settings
	std::map<uint64, Mod> mModSettings;

protected:
	Json::Value mSettingsJsons[3];	// Uses SettingsType as key

private:
	static inline Configuration* mSingleInstance;
	bool mSettingsReadOnly = false;
};
