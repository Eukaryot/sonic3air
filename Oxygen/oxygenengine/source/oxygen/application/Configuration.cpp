/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/helper/JsonSerializer.h"
#include "oxygen/platform/PlatformFunctions.h"

#include <lemon/translator/SourceCodeWriter.h>


namespace
{
	void readInputDevices(const Json::Value& rootJson, std::vector<InputConfig::DeviceDefinition>& inputDeviceDefinitions)
	{
		// Input devices
		const auto& devicesJson = rootJson["InputDevices"];
		if (!devicesJson.isObject())
			return;

		for (auto it = devicesJson.begin(); it != devicesJson.end(); ++it)
		{
			const std::string key = it.key().asString();

			// Check for overwrite
			InputConfig::DeviceDefinition* inputDeviceDefinition = nullptr;
			for (InputConfig::DeviceDefinition& definition : inputDeviceDefinitions)
			{
				if (definition.mIdentifier == key)
				{
					inputDeviceDefinition = &definition;
					break;
				}
			}

			if (nullptr == inputDeviceDefinition)
			{
				// Definition does not exist yet, this must be an unknown gamepad
				inputDeviceDefinition = &vectorAdd(inputDeviceDefinitions);
				inputDeviceDefinition->mIdentifier = it.key().asString();
				inputDeviceDefinition->mDeviceType = InputConfig::DeviceType::GAMEPAD;
			}

			// Collect device names
			const Json::Value deviceNames = (*it)["DeviceNames"];
			if (deviceNames.isArray())
			{
				for (Json::ArrayIndex i = 0; i < deviceNames.size(); ++i)
				{
					if (deviceNames[i].isString())
					{
						const std::string name = deviceNames[i].asString();
						if (!name.empty())
						{
							String str(name);
							str.lowerCase();
							const uint64 hash = rmx::getMurmur2_64(str);
							inputDeviceDefinition->mDeviceNames[hash] = *str;
						}
					}
				}
			}

			// Read mappings
			std::vector<InputConfig::Assignment> newAssignments;
			for (size_t buttonIndex = 0; buttonIndex < InputConfig::DeviceDefinition::NUM_BUTTONS; ++buttonIndex)
			{
				const Json::Value& mappingJson = (*it)[InputConfig::DeviceDefinition::BUTTON_NAME[buttonIndex]];
				newAssignments.clear();
				if (mappingJson.isArray())
				{
					for (Json::ArrayIndex k = 0; k < mappingJson.size(); ++k)
					{
						if (mappingJson[k].isString())
						{
							InputConfig::Assignment assignment;
							if (InputConfig::Assignment::setFromMappingString(assignment, mappingJson[k].asString(), inputDeviceDefinition->mDeviceType))
							{
								newAssignments.push_back(assignment);
							}
						}
					}
				}
				else if (mappingJson.isString())
				{
					std::vector<String> parts;
					String(mappingJson.asCString()).split(parts, ',');
					for (String& part : parts)
					{
						// Trim whitespace
						part.trimWhitespace();
						if (part.length() > 0)
						{
							InputConfig::Assignment assignment;
							if (InputConfig::Assignment::setFromMappingString(assignment, part.toStdString(), inputDeviceDefinition->mDeviceType))
							{
								newAssignments.push_back(assignment);
							}
						}
					}
				}
				else
				{
					// Use the previously set default assignments (unfortunately, this only works for keyboards)
					continue;
				}

				// Set assignments, and allow for duplicate assignments (i.e. having a real button mapped to multiple controls) in this case
				//  -> This is not allowed when using S3AIR's controls setup menu, as it can potentially lead to weird in-game behavior
				//  -> But by manipulating the settings_input.json directly, players can still have duplicate assignments if they want to - e.g. if their controller has only very few buttons
				InputConfig::setAssignments(*inputDeviceDefinition, buttonIndex, newAssignments, false);
			}
		}
	}

	void tryReadRenderMethod(JsonSerializer& serializer, bool failSafeMode, Configuration::RenderMethod& outRenderMethod, bool& outAutoDetect)
	{
		String renderMethodString;
		{
			std::string renderMethodStdString;
			if (serializer.serialize("RenderMethod", renderMethodStdString))
			{
				renderMethodString = renderMethodStdString;
				renderMethodString.lowerCase();

				outAutoDetect = (renderMethodString == "auto");
				if (outAutoDetect)
				{
					outRenderMethod = Configuration::getHighestSupportedRenderMethod();
				}
			}
		}

		if (failSafeMode)
		{
			outRenderMethod = Configuration::RenderMethod::SOFTWARE;
		}
		else
		{
			if (renderMethodString.startsWith("opengl"))
			{
				outRenderMethod = (renderMethodString.endsWith("soft") || renderMethodString.endsWith("software")) ? Configuration::RenderMethod::OPENGL_SOFT : Configuration::RenderMethod::OPENGL_FULL;
			}
			else if (renderMethodString == "software")
			{
				outRenderMethod = Configuration::RenderMethod::SOFTWARE;
			}

			if (outRenderMethod == Configuration::RenderMethod::UNDEFINED)
			{
				bool useSoftwareRenderer = false;
				if (serializer.serialize("UseSoftwareRenderer", useSoftwareRenderer))
				{
					outRenderMethod = useSoftwareRenderer ? Configuration::RenderMethod::OPENGL_SOFT : Configuration::RenderMethod::OPENGL_FULL;
				}
			}
		}
	}

	void loadModSettings(const Json::Value& rootJson, std::map<uint64, Configuration::Mod>& modSettings)
	{
		const auto& modJson = rootJson["ModSettings"];
		if (!modJson.isObject())
			return;

		for (auto it = modJson.begin(); it != modJson.end(); ++it)
		{
			const std::string modName = it.key().asString();
			const uint64 modNameHash = rmx::getMurmur2_64(modName);

			Configuration::Mod& mod = modSettings[modNameHash];
			mod.mModName = modName;

			for (auto it2 = it->begin(); it2 != it->end(); ++it2)
			{
				if (it2->isNumeric())
				{
					const std::string key = it2.key().asString();
					const uint64 keyHash = rmx::getMurmur2_64(key);

					uint32 value = 0;
					try
					{
						value = it2->asUInt();
					}
					catch (std::exception& e)	// Catching exception like "LargestInt out of UInt range"
					{
						RMX_ERROR("Failed to read '" << key << "' setting for mod '" << modName << "' with error: " << e.what(), );
						continue;
					}

					Configuration::Mod::Setting& setting = mod.mSettings[keyHash];
					setting.mIdentifier = key;
					setting.mValue = value;
				}
			}
		}
	}

	void saveModSettings(Json::Value& rootJson, const std::map<uint64, Configuration::Mod>& modSettings)
	{
		Json::Value modSettingsJson;
		for (const auto& pair : modSettings)
		{
			if (!pair.second.mSettings.empty())
			{
				Json::Value modJson;
				for (const auto& pair2 : pair.second.mSettings)
				{
					modJson[pair2.second.mIdentifier] = pair2.second.mValue;
				}
				modSettingsJson[pair.second.mModName] = modJson;
			}
		}
		rootJson["ModSettings"] = modSettingsJson;
	}
}



Configuration::RenderMethod Configuration::getHighestSupportedRenderMethod()
{
#if defined(PLATFORM_WEB) || (defined(PLATFORM_MAC) && defined(__arm64__)) || defined(PLATFORM_VITA)
	return RenderMethod::OPENGL_SOFT;
#else
	// Default is OpenGL Hardware render method (as it's the highest one), but this can be lowered as needed, e.g. for individual platforms or depending on the execution environment
	return RenderMethod::OPENGL_FULL;
#endif
}

Configuration::Configuration()
{
	mSingleInstance = this;

	// Setup default value, just in case (needed e.g. for compiling scripts from source on Android)
	mScriptsDir = L"./scripts/";

#if defined(PLATFORM_WEB)
	// Threading in general is not (afaik) supported by emscripten
	mAudio.mUseAudioThreading = false;
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
	// Use a much larger default UI scale on mobile platforms, otherwise it's too finicky to interact with ImGui at all
	mDevMode.mUIScale = 2.5f;
#endif
}

void Configuration::initialization()
{
	// Setup defaults for input devices
	mInputDeviceDefinitions.reserve(8);
	InputConfig::setupDefaultDeviceDefinitions(mInputDeviceDefinitions);

	preLoadInitialization();
}

bool Configuration::loadConfiguration(const std::wstring& filename)
{
	// Open file
	Json::Value root = JsonHelper::loadFile(filename);
	const bool loaded = !root.isNull();		// If the config.json was not found, just silently ignore that for now, and return false in the end
	JsonSerializer serializer(true, root);

#ifdef PLATFORM_WINDOWS
	// Just for debugging
	bool wait = false;
	if (loaded && serializer.serialize("WaitForDebugger", wait) && wait)
	{
		static bool alreadyWaited = false;
		if (!alreadyWaited)		// This is needed because we enter config.json loading twice during startup, but waiting for debugger is meant to be done just once
		{
			PlatformFunctions::showMessageBox("Waiting for debugger", "Attach debugger now, or don't...");
			alreadyWaited = true;
		}
	}
#endif

	// Define fallback values
	mMainScriptName = L"main.lemon";
	if (mEngineDataPath.empty())
		mEngineDataPath = L"data";
	if (mGameDataPath.empty())
		mGameDataPath = L"data";
	if (mAnalysisDir.empty())
		mAnalysisDir = L"___internal/analysis/";

	// This does not get read from the file, but defined by code
	mPreprocessorDefinitions.clear();
	mPreprocessorDefinitions.setDefinition("STANDALONE");

	// Load project path
	if (serializer.serialize("LoadProject", mProjectPath))
	{
		mProjectPath += L"/";
	}

	// Load everything shared with settings
	loadConfigurationProperties(serializer);

	// Call subclass implementation
	const bool success = loadConfigurationInternal(serializer);
	return loaded && success;
}

bool Configuration::loadSettings(const std::wstring& filename, SettingsType settingsType)
{
	const int settingsIndex = (int)settingsType;
	mSettingsFilenames[settingsIndex] = filename;

	// Open file
	Json::Value root = JsonHelper::loadFile(filename);
	if (root.isNull())
		return false;
	JsonSerializer serializer(true, root);

	if (settingsType == SettingsType::INPUT)
	{
		// Input devices
		readInputDevices(root, mInputDeviceDefinitions);
	}
	else
	{
		// All kinds of stuff
		serializeStandardSettings(serializer);

		// Dev mode
		serializeDevMode(serializer);

		// Mod settings
		loadModSettings(root, mModSettings);
	}

	// Call subclass implementation
	const bool success = loadSettingsInternal(serializer, settingsType);

	// Cleanup?
	{
		bool retainOldEntries = true;
		switch (settingsType)
		{
			case SettingsType::STANDARD:
			{
				bool performCleanup = false;
				if (serializer.serialize("CleanupSettings", performCleanup))
				{
					retainOldEntries = !performCleanup;
				}
				break;
			}

			case SettingsType::INPUT:
			{
				retainOldEntries = false;
				break;
			}
		}

		if (retainOldEntries)
		{
			// Backup old settings values
			//  -> Especially the unknown keys, for easy forward compatibility with future additions
			mSettingsJsons[settingsIndex].swap(root);
		}
		else
		{
			mSettingsJsons[settingsIndex] = Json::Value();
		}
	}

	return success;
}

void Configuration::saveSettings()
{
	// Do not save if settings are set to read-only
	if (mSettingsReadOnly)
		return;

	// Save standard settings
	int settingsIndex = (int)SettingsType::STANDARD;
	if (!mSettingsFilenames[settingsIndex].empty())
	{
		Json::Value root = mSettingsJsons[settingsIndex];
		root["CleanupSettings"] = 0;

		JsonSerializer serializer(false, root);
		serializeStandardSettings(serializer);

		// Dev mode
		serializeDevMode(serializer);

		// Mod settings
		saveModSettings(root, mModSettings);

		// Call subclass implementation
		saveSettingsInternal(serializer, SettingsType::STANDARD);

		// Save file
		JsonHelper::saveFile(mSettingsFilenames[settingsIndex], root);
	}

	// Save input settings
	settingsIndex = (int)SettingsType::INPUT;
	if (!mSettingsFilenames[settingsIndex].empty())
	{
		saveSettingsInput(mSettingsFilenames[settingsIndex]);
	}
}

void Configuration::loadConfigurationProperties(JsonSerializer& serializer)
{
	// Read dev mode setting first, as other settings rely on it
	serializeDevMode(serializer);

	// Paths
	if (mRomPath.empty())
	{
		if (serializer.serialize("RomPath", mRomPath))
		{
			FTX::FileSystem->normalizePath(mRomPath, false);
		}
	}
	if (serializer.serialize("ScriptsDir", mScriptsDir))
	{
		FTX::FileSystem->normalizePath(mScriptsDir, true);
	}
	serializer.serialize("MainScriptName", mMainScriptName);

	if (mDevMode.mEnabled)
	{
		if (serializer.serialize("SaveStatesDir", mSaveStatesDir))
		{
			FTX::FileSystem->normalizePath(mSaveStatesDir, true);
		}
	}

	// Platform
	serializer.serialize("PlatformFlags", mPlatformFlags);

	// Game
	serializer.serialize("StartPhase", mStartPhase);

	// Game recorder
	if (serializer.beginObject("GameRecording"))
	{
		serializer.serialize("EnablePlayback", mGameRecorder.mEnablePlayback);
		if (mGameRecorder.mEnablePlayback)
		{
			serializer.serialize("PlaybackStartFrame", mGameRecorder.mPlaybackStartFrame);
			serializer.serialize("PlaybackIgnoreKeys", mGameRecorder.mPlaybackIgnoreKeys);
		}
		serializer.endObject();
	}

	if (mLoadLevel != -1 || mGameRecorder.mEnablePlayback)
	{
		// Enforce start phase 3 (in-game) when a level to load directly is defined, and in game recording playback mode
		mStartPhase = 3;
	}

	// Video
	serializer.serializeVectorAsSizeString("WindowSize", mWindowSize);
	if (mDevMode.mEnabled)
	{
		serializer.serializeVectorAsSizeString("GameScreen", mGameScreen);
	}
	serializer.serialize("Upscaling", mUpscaling);
	serializer.serialize("Filtering", mFiltering);
	serializer.serialize("Scanlines", mScanlines);
	serializer.serialize("BackgroundBlur", mBackgroundBlur);
	serializer.serialize("PerformanceDisplay", mPerformanceDisplay);
	tryReadRenderMethod(serializer, mFailSafeMode, mRenderMethod, mAutoDetectRenderMethod);

	// Input recorder
	if (mDevMode.mEnabled)
	{
		if (serializer.beginObject("InputRecorder"))
		{
			serializer.serialize("Playback", mInputRecorderInput);
			serializer.serialize("Record", mInputRecorderOutput);
			serializer.endObject();
		}
	}

#if DEBUG
	// Script
	serializer.serialize("CompileScripts", mForceCompileScripts);
#endif
}

void Configuration::serializeStandardSettings(JsonSerializer& serializer)
{
	// Paths
	serializer.serialize("RomPath", mLastRomPath);

	// General
	serializer.serialize("FailSafeMode", mFailSafeMode);

	if (serializer.isReading() && mFailSafeMode)
		mAudio.mUseAudioThreading = false;

	// Graphics
	if (serializer.isReading())
	{
		tryReadRenderMethod(serializer, mFailSafeMode, mRenderMethod, mAutoDetectRenderMethod);
	}
	else
	{
		std::string renderMethod = mAutoDetectRenderMethod ? "auto" :
									(mRenderMethod == RenderMethod::OPENGL_FULL) ? "opengl-full" :
									(mRenderMethod == RenderMethod::OPENGL_SOFT) ? "opengl-soft" : "software";
		serializer.serialize("RenderMethod", renderMethod);

		serializer.serialize("FailSafeMode", mFailSafeMode);
		serializer.serialize("PlatformFlags", mPlatformFlags);
	}

	serializer.serializeAs<int>("Fullscreen", mWindowMode);
	serializer.serialize("DisplayIndex", mDisplayIndex);
	serializer.serializeAs<int>("FrameSync", mFrameSync);
	serializer.serialize("Upscaling", mUpscaling);
	serializer.serialize("Backdrop", mBackdrop);
	serializer.serialize("Filtering", mFiltering);
	serializer.serialize("Scanlines", mScanlines);
	serializer.serialize("BackgroundBlur", mBackgroundBlur);
	serializer.serialize("PerformanceDisplay", mPerformanceDisplay);

	// Audio
	if (serializer.beginObject("Audio"))
	{
		serializer.serialize("MasterVolume", mAudio.mMasterVolume);
		serializer.serialize("MusicVolume", mAudio.mMusicVolume);
		serializer.serialize("SoundVolume", mAudio.mSoundVolume);
		serializer.serialize("SampleRate", mAudio.mSampleRate);
		serializer.endObject();
	}
	else if (serializer.isReading())
	{
		// Legacy support for old, more flat way of storing settings (before Jan 2026)
		serializer.serialize("Volume", mAudio.mMasterVolume);
		serializer.serialize("Audio_MusicVolume", mAudio.mMusicVolume);
		serializer.serialize("Audio_SoundVolume", mAudio.mSoundVolume);
	}

	// Input
	if (serializer.beginObject("Input"))
	{
		serializer.serialize("PreferredGamepadPlayer1", mPreferredGamepad[0]);
		serializer.serialize("PreferredGamepadPlayer2", mPreferredGamepad[1]);
		serializer.serialize("PreferredGamepadPlayer3", mPreferredGamepad[2]);
		serializer.serialize("PreferredGamepadPlayer4", mPreferredGamepad[3]);
		serializer.serialize("ControllerRumblePlayer1", mControllerRumbleIntensity[0]);
		serializer.serialize("ControllerRumblePlayer2", mControllerRumbleIntensity[1]);
		serializer.serialize("ControllerRumblePlayer3", mControllerRumbleIntensity[2]);
		serializer.serialize("ControllerRumblePlayer4", mControllerRumbleIntensity[3]);
		serializer.serialize("AutoAssignGamepadPlayerIndex", mAutoAssignGamepadPlayerIndex);
		serializer.endObject();
	}
	else if (serializer.isReading())
	{
		// Legacy support for old, more flat way of storing settings (before Jan 2026)
		serializer.serialize("PreferredGamepadPlayer1", mPreferredGamepad[0]);
		serializer.serialize("PreferredGamepadPlayer2", mPreferredGamepad[1]);
		serializer.serialize("PreferredGamepadPlayer3", mPreferredGamepad[2]);
		serializer.serialize("PreferredGamepadPlayer4", mPreferredGamepad[3]);
		serializer.serialize("ControllerRumblePlayer1", mControllerRumbleIntensity[0]);
		serializer.serialize("ControllerRumblePlayer2", mControllerRumbleIntensity[1]);
		serializer.serialize("ControllerRumblePlayer3", mControllerRumbleIntensity[2]);
		serializer.serialize("ControllerRumblePlayer4", mControllerRumbleIntensity[3]);
		serializer.serialize("AutoAssignGamepadPlayerIndex", mAutoAssignGamepadPlayerIndex);
	}

	// Virtual gamepad
	if (serializer.beginObject("VirtualGamepad"))
	{
		serializer.serialize("Opacity", mVirtualGamepad.mOpacity);
		serializer.serializeComponents("DPadPos", mVirtualGamepad.mDirectionalPadCenter);
		serializer.serialize("DPadSize", mVirtualGamepad.mDirectionalPadSize);
		serializer.serializeComponents("ButtonsPos", mVirtualGamepad.mFaceButtonsCenter);
		serializer.serialize("ButtonsSize", mVirtualGamepad.mFaceButtonsSize);
		serializer.serializeComponents("StartPos", mVirtualGamepad.mStartButtonCenter);
		serializer.serializeComponents("GameRecPos", mVirtualGamepad.mGameRecButtonCenter);
		serializer.serializeComponents("ShoulderLPos", mVirtualGamepad.mShoulderLButtonCenter);
		serializer.serializeComponents("ShoulderRPos", mVirtualGamepad.mShoulderRButtonCenter);
		serializer.endObject();
	}

	// Game recorder
	serializer.serialize("GameRecordingMode", mGameRecorder.mRecordingMode);

	// Script
	serializer.serialize("ScriptOptimizationLevel", mScriptOptimizationLevel);

	// Game server
	if (serializer.beginObject("GameServer"))
	{
		serializer.serialize("ServerAddress", mGameServerBase.mServerHostName);
		serializer.serialize("ServerPortUDP", mGameServerBase.mServerPortUDP);
		serializer.serialize("ServerPortTCP", mGameServerBase.mServerPortTCP);
		serializer.serialize("ServerPortWSS", mGameServerBase.mServerPortWSS);
		serializer.endObject();
	}
}

void Configuration::serializeDevMode(JsonSerializer& serializer)
{
	if (serializer.beginObject("DevMode"))
	{
		serializer.serialize("Enabled", mDevMode.mEnableAtStartup);

		serializer.serialize("LoadSaveState", mLoadSaveState);
		serializer.serializeHexValue("LoadLevel", mLoadLevel, 4);
		if (serializer.serialize("UseCharacters", mUseCharacters))
		{
			if (serializer.isReading())
				mUseCharacters = clamp(mUseCharacters, 0, 4);
		}

		serializer.serialize("EnableROMDataAnalyser", mEnableROMDataAnalyser);

		if (serializer.beginObject("DevModeUI"))
		{
			serializer.serialize("Scale", mDevMode.mUIScale);
			serializer.serializeHexColorRGB("AccentColor", mDevMode.mUIAccentColor);
			serializer.serialize("ScrollByDragging", mDevMode.mScrollByDragging);
			serializer.serializeArray("OpenWindows", mDevMode.mOpenUIWindows);
			serializer.serialize("MainWindowOpen", mDevMode.mMainWindowOpen);
			serializer.serialize("UseTabsInMainWindow", mDevMode.mUseTabsInMainWindow);
			serializer.serialize("ActiveMainWindowTab", mDevMode.mActiveMainWindowTab);
			serializer.serialize("ApplyModSettingsAfterLoadState", mDevMode.mApplyModSettingsAfterLoadState);
			serializer.endObject();
		}

		if (serializer.beginObject("ExternalCodeEditor"))
		{
			serializer.serialize("Type", mDevMode.mExternalCodeEditor.mActiveType);
			serializer.serialize("VSCodePath", mDevMode.mExternalCodeEditor.mVisualStudioCodePath);
			serializer.serialize("NppPath", mDevMode.mExternalCodeEditor.mNotepadPlusPlusPath);
			serializer.serialize("CustomEditorPath", mDevMode.mExternalCodeEditor.mCustomEditorPath);
			serializer.serialize("CustomEditorArgs", mDevMode.mExternalCodeEditor.mCustomEditorArgs);
			serializer.endObject();
		}

		serializer.endObject();
	}
}

void Configuration::saveSettingsInput(const std::wstring& filename) const
{
	// Using a custom JSON writer here to make the output easier to read, as standard formatting is kind of awful in this case

	std::vector<const InputConfig::DeviceDefinition*> sortedDeviceDefinitions;
	{
		// Sort input devices alphabetically, and so that keyboards are written first in any case
		sortedDeviceDefinitions.reserve(mInputDeviceDefinitions.size());
		for (size_t k = 0; k < mInputDeviceDefinitions.size(); ++k)
		{
			sortedDeviceDefinitions.push_back(&mInputDeviceDefinitions[k]);
		}
		std::sort(sortedDeviceDefinitions.begin(), sortedDeviceDefinitions.end(),
			[](const InputConfig::DeviceDefinition* a, const InputConfig::DeviceDefinition* b)
			{
				const bool keyboard_a = String(a->mIdentifier).startsWith("Keyboard");
				const bool keyboard_b = String(b->mIdentifier).startsWith("Keyboard");
				return (keyboard_a != keyboard_b) ? keyboard_a : (a->mIdentifier < b->mIdentifier);
			});
	}

	String output;
	lemon::SourceCodeWriter writer(output);

	writer.beginBlock();
	writer.writeLine("\"InputDevices\":");
	writer.beginBlock();

	for (size_t k = 0; k < sortedDeviceDefinitions.size(); ++k)
	{
		const InputConfig::DeviceDefinition& inputDeviceDefinition = *sortedDeviceDefinitions[k];

		writer.writeLine("\"" + inputDeviceDefinition.mIdentifier + "\":");
		writer.beginBlock();

		if (!inputDeviceDefinition.mDeviceNames.empty())
		{
			String line = "\"DeviceNames\": [ ";
			bool isFirst = true;
			for (const auto& pair : inputDeviceDefinition.mDeviceNames)
			{
				if (!isFirst)
					line << ", ";
				line << "\"" << pair.second << "\"";
				isFirst = false;
			}
			line << " ],";
			writer.writeLine(line);
		}

		for (size_t i = 0; i < InputConfig::DeviceDefinition::NUM_BUTTONS; ++i)
		{
			const std::string& name = InputConfig::DeviceDefinition::BUTTON_NAME[i];
			String line = "\"" + name + "\":";
			line.add(' ', 6 - (int)name.length());
			line << "[ ";

			const std::vector<InputConfig::Assignment>& assignments = inputDeviceDefinition.mMappings[i].mAssignments;
			bool isFirst = true;
			String str;
			for (const InputConfig::Assignment& assignment : assignments)
			{
				assignment.getMappingString(str, inputDeviceDefinition.mDeviceType);
				if (!isFirst)
					line << ", ";
				line << "\"" << str << "\"";
				isFirst = false;
			}
			line << " ]";
			if (i+1 < InputConfig::DeviceDefinition::NUM_BUTTONS)
				line << ",";
			writer.writeLine(line);
		}

		const bool lastBlock = (k+1 == mInputDeviceDefinitions.size());
		writer.endBlock(lastBlock ? "}" : "},");
	}

	writer.endBlock();
	writer.endBlock();
	output.saveFile(filename);
}
