/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputConfig.h"

#include <lemon/compiler/PreprocessorDefinition.h>

class JsonHelper;


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
		WINDOWED,
		BORDERLESS_FULLSCREEN,
		EXCLUSIVE_FULLSCREEN
	};

	enum class FrameSyncType
	{
		VSYNC_OFF,
		VSYNC_ON,
		VSYNC_FRAMECAP,
		FRAME_INTERPOLATION,
		_NUM
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
		bool mEnabled = false;
		float mGameViewScale = 1.0f;
		Vec2f mGameViewAlignment;
		Color mUIAccentColor = Color(0.2f, 0.5f, 0.8f);
		ExternalCodeEditor mExternalCodeEditor;
	};

	struct GameRecorder
	{
		int mRecordingMode = -1;		// -1 = Auto, 0 = Recording disabled, 1 = Recording enabled
		bool mIsRecording = false;
		bool mIsPlayback = false;
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
		GLOBAL = 2		// "settings_global.json"
	};

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

	void evaluateGameRecording();

protected:
	virtual void preLoadInitialization() = 0;
	virtual bool loadConfigurationInternal(JsonHelper& jsonHelper) = 0;
	virtual bool loadSettingsInternal(JsonHelper& jsonHelper, SettingsType settingsType) = 0;
	virtual void saveSettingsInternal(Json::Value& root, SettingsType settingsType) = 0;

private:
	void loadConfigurationProperties(JsonHelper& rootHelper);
	void loadDevModeSettings(JsonHelper& rootHelper);
	void saveSettingsInput(const std::wstring& filename) const;

public:
	// Paths
	std::wstring mProjectPath;	// Only used in Engine App
	std::wstring mExePath;
	std::wstring mAppDataPath;
	std::wstring mSettingsFilenames[3];		// Uses SettingsType as key
	std::wstring mEngineDataPath;
	std::wstring mGameDataPath;
	std::wstring mRomPath;		// From configuration
	std::wstring mLastRomPath;	// From settings
	std::wstring mScriptsDir;
	std::wstring mMainScriptName;
	lemon::PreprocessorDefinitionMap mPreprocessorDefinitions;
	std::wstring mSaveStatesDir;
	std::wstring mSaveStatesDirLocal;
	std::wstring mAnalysisDir;
	std::wstring mPersistentDataBasePath;

	// General
	bool   mFailSafeMode = false;
	int	   mPlatformFlags = -1;

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
	int   mAudioSampleRate = 48000;
	float mAudioVolume = 1.0f;
	bool  mUseAudioThreading = true;		// Disabled in constructor for platforms that don't support it

	// Input
	std::vector<InputConfig::DeviceDefinition> mInputDeviceDefinitions;
	VirtualGamepad mVirtualGamepad;
	std::string mPreferredGamepad[2];
	int mAutoAssignGamepadPlayerIndex = 0;	// Default is player 1 (who has index 0)
	float mControllerRumbleIntensity[2] = { 0, 0 };

	// Input recorder
	std::wstring mInputRecorderInput;
	std::wstring mInputRecorderOutput;

	// Misc
	bool mMirrorMode = false;

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
	static Configuration* mSingleInstance;
	bool mSettingsReadOnly = false;
};
