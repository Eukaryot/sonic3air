/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/GameRecorder.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/SaveStateSerializer.h"
#include "oxygen/simulation/SimulationState.h"
#include "oxygen/simulation/analyse/ROMDataAnalyser.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/input/InputRecorder.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/rendering/parts/RenderParts.h"


namespace
{
	void recordKeyFrame(uint32 frameNumber, Simulation& simulation, GameRecorder& gameRecorder, const GameRecorder::InputData& inputData)
	{
		static std::vector<uint8> data;
		data.reserve(0x128000);
		data.clear();

		SaveStateSerializer serializer(simulation, RenderParts::instance());
		serializer.saveState(data);

		gameRecorder.addKeyFrame(frameNumber, inputData, data);
	}
}


Simulation::Simulation() :
	mCodeExec(*new CodeExec()),
	mSimulationState(*new SimulationState()),
	mGameRecorder(*new GameRecorder()),
	mInputRecorder(*new InputRecorder())
{
	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		mROMDataAnalyser = new ROMDataAnalyser();
	}
}

Simulation::~Simulation()
{
	delete &mCodeExec;
	delete &mSimulationState;
	delete &mGameRecorder;
	delete &mInputRecorder;
	delete mROMDataAnalyser;
}

bool Simulation::startup()
{
	Configuration& config = Configuration::instance();

	RMX_LOG_INFO("Setup of EmulatorInterface");
	mCodeExec.startup();

	// Load scripts
	RMX_LOG_INFO("Loading scripts");
	bool success = mCodeExec.reloadScripts(true, false);	// Note: First parameter could just as well be set to false
	if (success)
	{
		mCodeExec.reinitRuntime(nullptr, CodeExec::CallStackInitPolicy::RESET);
	}

	// Optionally load save state
	mStateLoaded.clear();
	if (success && EngineMain::getDelegate().useDeveloperFeatures() && !config.mLoadSaveState.empty())
	{
		success = loadState(config.mSaveStatesDirLocal + config.mLoadSaveState + L".state", false);
		if (!success)
			loadState(config.mSaveStatesDir + config.mLoadSaveState + L".state");
	}
	RMX_LOG_INFO("Runtime environment ready");

	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		// Startup input recorder
		mInputRecorder.initFromConfig();
	}

	if (config.mGameRecorder.mIsPlayback)
	{
		// Try the long and short name
		if (mGameRecorder.loadRecording(L"gamerecording.bin"))
		{
			RMX_LOG_INFO("Playback of 'gamerecording.bin'");
		}
		else if (mGameRecorder.loadRecording(L"gamerec.bin"))
		{
			RMX_LOG_INFO("Playback of 'gamerec.bin'");
		}

		if (mGameRecorder.hasFrameNumber(mFrameNumber))
		{
			mGameRecorder.setIgnoreKeys(config.mGameRecorder.mPlaybackIgnoreKeys);
			config.setSettingsReadOnly(true);	// Do not overwrite settings
			jumpToFrame(config.mGameRecorder.mPlaybackStartFrame, false);
		}
	}

	return true;
}

void Simulation::shutdown()
{
	RMX_LOG_INFO("Simulation shutdown");
	mInputRecorder.shutdown();

	mIsRunning = false;
}

void Simulation::reloadScriptsAfterModsChange()
{
	// Immediate reload of the scripts (while the loading text box is shown)
	if (mCodeExec.reloadScripts(false, false))
	{
		mCodeExec.reinitRuntime(nullptr, CodeExec::CallStackInitPolicy::RESET);
	}
}

EmulatorInterface& Simulation::getEmulatorInterface()
{
	return mCodeExec.getEmulatorInterface();
}

void Simulation::resetState()
{
	EngineMain::instance().getAudioOut().reset();
	resetIntoGame(nullptr);
}

void Simulation::resetIntoGame(const std::vector<std::pair<std::string, std::string>>* enforcedCallStack)
{
	// Reset randomization
	randomize();

	// Reset simulation
	mSimulationState.reset();

	// Reset video & audio
	VideoOut::instance().reset();
	EngineMain::instance().getAudioOut().resetGame();

	// Reset code execution
	mCodeExec.reset();
	mStateLoaded.clear();

	// Reload and initialize scripts as needed
	if (mCodeExec.reloadScripts(false, false))
	{
		mCodeExec.reinitRuntime(enforcedCallStack, CodeExec::CallStackInitPolicy::RESET);
	}

	// Apply mod settings
	for (Mod* mod : ModManager::instance().getActiveMods())
	{
		for (Mod::SettingCategory& modSettingCategory : mod->mSettingCategories)
		{
			for (Mod::Setting& modSetting : modSettingCategory.mSettings)
			{
				mCodeExec.getLemonScriptRuntime().setGlobalVariableValue_int64(modSetting.mBinding, modSetting.mCurrentValue);
			}
		}
	}

	mFrameNumber = 0;
	mCurrentTargetFrame = 0.0;
	mNextSingleStep = false;
	mSingleStepContinue = false;
	mGameRecorder.clear();
}

void Simulation::resetIntoGame(const std::string& entryFunctionName)
{
	const std::vector<std::pair<std::string, std::string>> enforcedCallStack = { { entryFunctionName, "" } };
	resetIntoGame(&enforcedCallStack);
}

void Simulation::reloadLastState()
{
	if (!mStateLoaded.empty())
	{
		loadState(mStateLoaded);
	}
}

bool Simulation::loadState(const std::wstring& filename, bool showError)
{
	VideoOut::instance().reset();
	EngineMain::instance().getAudioOut().reset();

	SaveStateSerializer::StateType stateType;
	SaveStateSerializer serializer(*this, RenderParts::instance());

	const bool success = serializer.loadState(filename, &stateType);
	if (!success)
	{
		if (showError)
			RMX_ERROR("Failed to load save state '" << WString(filename).toStdString() << "'", );
		return false;
	}

	mStateLoaded = filename;
	mCodeExec.reinitRuntime(nullptr, (stateType == SaveStateSerializer::StateType::GENSX) ? CodeExec::CallStackInitPolicy::READ_FROM_ASM : CodeExec::CallStackInitPolicy::USE_EXISTING);

	mFrameNumber = 0;
	mCurrentTargetFrame = 0.0;
	mNextSingleStep = false;
	mSingleStepContinue = false;

	mGameRecorder.clear();
	return true;
}

void Simulation::saveState(const std::wstring& filename)
{
	SaveStateSerializer serializer(*this, RenderParts::instance());
	const bool success = serializer.saveState(filename);
	RMX_CHECK(success, "Failed to save save state '" << WString(filename).toStdString() << "'", return);

	// Also save a screenshot
	Bitmap bmp;
	VideoOut::instance().getScreenshot(bmp);
	bmp.save(filename + L".bmp");

	// Set as default for "reloadLastState"
	mStateLoaded = filename;
}

bool Simulation::triggerFullScriptsReload()
{
	if (mCodeExec.reloadScripts(true, true))
	{
		mCodeExec.restoreRuntimeState(!mStateLoaded.empty());
		return true;
	}
	else
	{
		return false;
	}
}

void Simulation::update(float timeElapsed)
{
	if (!isRunning() || !mCodeExec.isCodeExecutionPossible())
		return;

	if (mRewindSteps >= 0)
	{
		setSpeed(0.0f);
		while (mRewindSteps >= 1)
		{
			if (jumpToFrame(mFrameNumber - mRewindSteps))
				mRewindSteps = 0;
			else
				--mRewindSteps;		// Try again with one step less
		}
		mRewindSteps = -1;
	}

	// Limit length of one frame to 100ms
	timeElapsed = clamp(timeElapsed, 0.0f, 0.1f);

	// Do nothing as long as not enough time has passed
	const double oldTargetFrame = mCurrentTargetFrame;
	if (mSimulationSpeed <= 0.0f)
	{
		if (mNextSingleStep)
		{
			mCurrentTargetFrame = roundToDouble(mCurrentTargetFrame + 1.0);
			mNextSingleStep = mSingleStepContinue;
		}
		else
		{
			mCurrentTargetFrame = roundToDouble(mCurrentTargetFrame);
		}
	}
	else
	{
		const float step = timeElapsed * mSimulationSpeed;
		mCurrentTargetFrame += (double)step * (double)getSimulationFrequency();
	}

	const bool useFrameInterpolation = (Configuration::instance().mFrameSync == Configuration::FrameSyncType::FRAME_INTERPOLATION);
	const uint32 requiredFrameNumber = useFrameInterpolation ? (uint32)std::ceil(mCurrentTargetFrame) : (uint32)roundToInt(mCurrentTargetFrame);

	if (mFrameNumber < requiredFrameNumber)
	{
		const uint32 startTime = SDL_GetTicks();
		const uint32 limitTime = startTime + 200;

		while (mFrameNumber < requiredFrameNumber)
		{
			// Update emulation
			const bool result = generateFrame();
			if (!result)
				break;

			// Time limit to prevent non-responding application
			if (SDL_GetTicks() >= limitTime)
			{
				// Reset target frame to an earlier frame, but still make sure we had any progress at all
				mCurrentTargetFrame = std::max((double)mFrameNumber, oldTargetFrame);
				break;
			}
		}

		// Each second, a small correction to the accumulated time gets applied
		if ((int)(mFrameNumber - mLastCorrectionFrame) >= (int)getSimulationFrequency())
		{
			// The idea here is to bring the accumulated time towards the midpoint, where it's most stable against unintentional double frames or frame skips (which might happen otherwise)
			//  -> This is most useful for 60 Hz displays with V-sync on, but should have a similar effect on e.g. 75 Hz, 90 Hz, 120 Hz
			//  -> It should not introduce any noticeable issues or game speed changes in other cases
			const double stableOffset = useFrameInterpolation ? 0.25 : 0.0;
			const double diff = mCurrentTargetFrame - (roundToDouble(mCurrentTargetFrame - stableOffset) + stableOffset);
			const constexpr double maxChange = 0.1;
			mCurrentTargetFrame += clamp(-diff, -maxChange, maxChange);
			mLastCorrectionFrame = mFrameNumber;
		}
	}

	if (mFrameNumber == requiredFrameNumber)
	{
		const float position = saturate((float)(mCurrentTargetFrame - (double)(mFrameNumber - 1)));
		VideoOut::instance().setInterFramePosition(position);
	}
	else
	{
		VideoOut::instance().setInterFramePosition(0.0f);
	}

#if 0
	// Meant for debugging of accumulated time stability
	if ((mFrameNumber % 6) == 0)
		LogDisplay::instance().setModeDisplay(String(0, "diff = %+0.3f", (float)(mCurrentTargetFrame - roundToDouble(mCurrentTargetFrame))));
#endif
}

bool Simulation::generateFrame()
{
	ControlsIn& controlsIn = ControlsIn::instance();
	GameplayConnector& gameplayConnector = GameplayConnector::instance();

	const bool isGameRecorderPlayback = Configuration::instance().mGameRecorder.mIsPlayback;
	const bool isGameRecorderRecording = Configuration::instance().mGameRecorder.mIsRecording;

	const bool beginningNewFrame = mCodeExec.willBeginNewFrame();
	const float tickLength = 1.0f / getSimulationFrequency();

	bool completedCurrentFrame = false;
	bool inputWasInjected = false;

	// Steps to do when beginning a new frame
	if (beginningNewFrame)
	{
		// Tell game instance
		EngineMain::getDelegate().onPreFrameUpdate();

		// Tell video that we begin a new frame
		VideoOut::instance().preFrameUpdate();

		// Game recorder: Save initial frame
		if (isGameRecorderRecording && mGameRecorder.getRangeEnd() == 0)
		{
			recordKeyFrame(0, *this, mGameRecorder, GameRecorder::InputData());
		}

		// Update gameplay connector
		gameplayConnector.onFrameUpdate(controlsIn, mFrameNumber, inputWasInjected);

		// If game recorder has input data for the frame transition, then use that
		//  -> This is particularly relevant for rewinds, namely for the small fast forwards from the previous keyframe
		GameRecorder::PlaybackResult result;
		if (mGameRecorder.getFrameData(mFrameNumber + 1, result))
		{
			if (isGameRecorderPlayback)
				LogDisplay::instance().setModeDisplay("Game recorder playback at frame: " + std::to_string(mFrameNumber + 1));

			if (nullptr != result.mData && !Configuration::instance().mGameRecorder.mPlaybackIgnoreKeys)
			{
				// Load save state
				SaveStateSerializer::StateType stateType;
				SaveStateSerializer serializer(*this, RenderParts::instance());

				const bool success = serializer.loadState(*result.mData, &stateType);
				if (success)
				{
					mCodeExec.reinitRuntime(nullptr, (stateType == SaveStateSerializer::StateType::GENSX) ? CodeExec::CallStackInitPolicy::READ_FROM_ASM : CodeExec::CallStackInitPolicy::USE_EXISTING);
					completedCurrentFrame = true;
				}
				else
				{
					RMX_ERROR("Failed to load save state", );
				}
			}

			controlsIn.injectInput(0, result.mInput->mInputs[0]);
			controlsIn.injectInput(1, result.mInput->mInputs[1]);
			inputWasInjected = true;
		}

		// Update input state
		{
			if (EngineMain::getDelegate().useDeveloperFeatures())
			{
				// Input recorder playback
				if (mInputRecorder.isPlaying())
				{
					const InputRecorder::InputState& inputState = mInputRecorder.updatePlayback(mFrameNumber);
					controlsIn.injectInput(0, inputState.mInputFlags[0]);
					controlsIn.injectInput(1, inputState.mInputFlags[1]);
					inputWasInjected = true;
				}
			}

			controlsIn.update(!inputWasInjected);

			EngineMain::getDelegate().onControlsUpdate();

			// Input state can be queried by scripts via "Input.getController" and "Input.getControllerPrevious"
		}
	}

	if (!completedCurrentFrame)		// Can be true already if game recorder playback just loaded a state
	{
		// Perform game simulation
		completedCurrentFrame = mCodeExec.performFrameUpdate();
	}

	// Steps to do when a frame got completed
	if (completedCurrentFrame)
	{
		// Tell game instance
		EngineMain::getDelegate().onPostFrameUpdate();

		// Tell video that we begin a new frame
		VideoOut::instance().postFrameUpdate();

		// Update audio
		EngineMain::instance().getAudioOut().update(tickLength);

		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			// Update input recording
			if (mInputRecorder.isRecording())
			{
				InputRecorder::InputState inputState;
				inputState.mInputFlags[0] = controlsIn.getInputPad(0);
				inputState.mInputFlags[1] = controlsIn.getInputPad(1);
				mInputRecorder.updateRecording(inputState);
			}
		}

		// Update game recording
		if (isGameRecorderRecording)
		{
			if (!mGameRecorder.hasFrameNumber(mFrameNumber + 1))
			{
				GameRecorder::InputData inputData;
				inputData.mInputs[0] = controlsIn.getInputPad(0);
				inputData.mInputs[1] = controlsIn.getInputPad(1);

				// Keyframe every 3 seconds - except when dev mode is active, because rewinding requires more frequent keyframes
				const int keyframeFrequency = EngineMain::getDelegate().useDeveloperFeatures() ? 10 : 180;
				const int framesToKeep = EngineMain::getDelegate().useDeveloperFeatures() ? 3600 : 1800;
				if (((mFrameNumber + 1) % keyframeFrequency) == 0)
				{
					recordKeyFrame(mFrameNumber + 1, *this, mGameRecorder, inputData);
					mGameRecorder.discardOldFrames(framesToKeep);
				}
				else
				{
					mGameRecorder.addFrame(mFrameNumber + 1, inputData);
				}
			}
		}
		else if (isGameRecorderPlayback && EngineMain::getDelegate().useDeveloperFeatures())
		{
			// Generate a keyframe every 10 frames, to allow for quick rewinds during game recording playback as well
			const int keyframeFrequency = 10;
			if (((mFrameNumber + 1) % keyframeFrequency) == 0 && !mGameRecorder.isKeyframe(mFrameNumber + 1))
			{
				GameRecorder::InputData inputData;
				inputData.mInputs[0] = controlsIn.getInputPad(0);
				inputData.mInputs[1] = controlsIn.getInputPad(1);
				recordKeyFrame(mFrameNumber + 1, *this, mGameRecorder, inputData);
			}
		}

		++mFrameNumber;
	}

	// Return false if frame got interrupted
	//  -> In this case, the outer loop should break
	//  -> Same if code execution is not possible any more
	return (completedCurrentFrame && mCodeExec.isCodeExecutionPossible());
}

bool Simulation::jumpToFrame(uint32 frameNumber, bool clearRecordingAfterwards)
{
	const bool isGameRecorderPlayback = Configuration::instance().mGameRecorder.mIsPlayback;
	const bool isGameRecorderRecording = Configuration::instance().mGameRecorder.mIsRecording;

	if (isGameRecorderRecording || isGameRecorderPlayback)
	{
		if (isGameRecorderPlayback)
			clearRecordingAfterwards = false;

		// Go back until the most recent keyframe, in case the selected frame is not a keyframe itself
		GameRecorder::PlaybackResult result;
		uint32 keyframeNumber = frameNumber;
		if (!mGameRecorder.getFrameData(keyframeNumber, result))
			return false;

		while (nullptr == result.mData)
		{
			if (keyframeNumber == 0)
				return false;
			--keyframeNumber;
			if (!mGameRecorder.getFrameData(keyframeNumber, result))
				return false;
		}

		SaveStateSerializer::StateType stateType;
		SaveStateSerializer serializer(*this, RenderParts::instance());

		const bool success = serializer.loadState(*result.mData, &stateType);
		if (success)
		{
			// Inject inputs for this frame, so that previous input will be set correctly in the next frame
			ControlsIn::instance().injectInput(0, result.mInput->mInputs[0]);
			ControlsIn::instance().injectInput(1, result.mInput->mInputs[1]);

			mCodeExec.reinitRuntime(nullptr, (stateType == SaveStateSerializer::StateType::GENSX) ? CodeExec::CallStackInitPolicy::READ_FROM_ASM : CodeExec::CallStackInitPolicy::USE_EXISTING);
			mFrameNumber = keyframeNumber;
			mCurrentTargetFrame = (float)frameNumber;

			if (clearRecordingAfterwards)
			{
				// Discard later frames to disable the logic that uses their recorded inputs instead of player input
				mGameRecorder.discardFramesAfter(frameNumber);
			}
			return true;
		}
	}

	return false;
}

float Simulation::getSimulationFrequency() const
{
	return (mSimulationFrequencyOverride > 0.0f) ? mSimulationFrequencyOverride : (float)Configuration::instance().mSimulationFrequency;
}

void Simulation::setSpeed(float emulatorSpeed)
{
	mSimulationSpeed = emulatorSpeed;
	mNextSingleStep = false;
	mSingleStepContinue = false;
}

void Simulation::setNextSingleStep(bool singleStep, bool continueToDebugEvent)
{
	mSimulationSpeed = 0.0f;
	mNextSingleStep = singleStep;
	mSingleStepContinue = continueToDebugEvent;
}

void Simulation::stopSingleStepContinue()
{
	if (mSingleStepContinue)
	{
		mNextSingleStep = false;
		mSingleStepContinue = false;
	}
}

void Simulation::refreshDebugging()
{
	VideoOut::instance().preRefreshDebugging();
	mCodeExec.executeScriptFunction("OxygenCallback.setupCustomSidePanelEntries", false);
	VideoOut::instance().postRefreshDebugging();
}

uint32 Simulation::saveGameRecording(WString* outFilename)
{
	std::wstring filename = L"gamerecording.bin";
	const std::string timeString = PlatformFunctions::getCompactSystemTimeString();
	if (!timeString.empty())
	{
		filename = L"gamerecording_" + String(timeString).toStdWString() + L".bin";
	}
	filename = Configuration::instance().mAppDataPath + L"gamerecordings/" + filename;

	if (!mGameRecorder.saveRecording(filename, 180))
		return 0;

	if (nullptr != outFilename)
		*outFilename = filename;

	return mGameRecorder.getCurrentNumberOfFrames();
}
