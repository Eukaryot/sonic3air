/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/SaveStateSerializer.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/input/InputRecorder.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/GameRecorder.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/analyse/ROMDataAnalyser.h"


Simulation::Simulation() :
	mCodeExec(*new CodeExec()),
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

	if (config.mGameRecording == 2)
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

		if (mGameRecorder.isPlaying())
		{
			mGameRecorder.setIgnoreKeys(config.mGameRecIgnoreKeys);
			mFastForwardTarget = config.mGameRecPlayFrom;
			config.setSettingsReadOnly(true);	// Do not overwrite settings
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
	// Immediate reload the scripts (while the loading text box is shown)
	if (mCodeExec.reloadScripts(false, false))
	{
		mCodeExec.reinitRuntime(nullptr, CodeExec::CallStackInitPolicy::RESET);
	}
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

	const float tickLength = 1.0f / getSimulationFrequency();
	const float tickAccumInitial = tickLength * 0.25f;		// You would usually except this to be zero, but that's a bit too far from the stable center

	mAccumulatedTime = tickAccumInitial;
	mNextSingleStep = false;
	mSingleStepContinue = false;
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
	SaveStateSerializer serializer(mCodeExec, RenderParts::instance());

	const bool success = serializer.loadState(filename, &stateType);
	if (!success)
	{
		if (showError)
			RMX_ERROR("Failed to load save state '" << WString(filename).toStdString() << "'", );
		return false;
	}

	mStateLoaded = filename;
	mCodeExec.reinitRuntime(nullptr, (stateType == SaveStateSerializer::StateType::GENSX) ? CodeExec::CallStackInitPolicy::READ_FROM_ASM : CodeExec::CallStackInitPolicy::USE_EXISTING);
	return true;
}

void Simulation::saveState(const std::wstring& filename)
{
	SaveStateSerializer serializer(mCodeExec, RenderParts::instance());
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

	const float tickLength = 1.0f / getSimulationFrequency();
	const float tickStableCenter = tickLength * 0.5f;		// Right in between the tick length, where it's most clearly inside the tick (towards the edges 0.0f and tick length, things get a bit ambiguous)
	const float tickAccumInitial = tickLength * 0.25f;		// You would usually except this to be zero, but that's a bit too far from the stable center

	// Handle fast forwarding (for game recording playback)
	if (mFrameNumber < mFastForwardTarget)
	{
		for (int i = std::min<int>(mFastForwardTarget - mFrameNumber, 500); i > 0; --i)
		{
			// Update emulation
			const bool result = generateFrame();
			if (!result)
				break;
		}
		mAccumulatedTime = tickAccumInitial;
		return;
	}

	// Limit length of one frame to 100ms
	timeElapsed = clamp(timeElapsed, 0.0f, 0.1f);

	// Do nothing as long as not enough time has passed
	if (mSimulationSpeed <= 0.0f)
	{
		if (mNextSingleStep)
		{
			mAccumulatedTime = tickLength;
			mNextSingleStep = mSingleStepContinue;
		}
		else
		{
			mAccumulatedTime = tickAccumInitial;
		}
	}
	else
	{
		const float step = timeElapsed * mSimulationSpeed;
		mAccumulatedTime += step;
	}

	if (mAccumulatedTime >= tickLength)
	{
		const uint32 startTime = SDL_GetTicks();
		const uint32 limitTime = startTime + 200;

		while (true)
		{
			// Update emulation
			const bool result = generateFrame();
			mAccumulatedTime -= tickLength;

			if (!result || mAccumulatedTime < tickLength)
				break;

			// Time limit to prevent non-responding application
			if (SDL_GetTicks() >= limitTime)
			{
				mAccumulatedTime = tickAccumInitial;
				break;
			}
		}

		// Each second, a small correction to the accumulated time gets applied
		if (mFrameNumber - mLastCorrectionFrame >= 60)
		{
			// The idea here is to bring the accumulated time towards the midpoint, where it's most stable against unintentional double frames or frame skips (which might happen otherwise)
			//  -> This is most useful for 60 Hz displays with V-sync on, but should have a similar effect on e.g. 75 Hz, 90 Hz, 120 Hz
			//  -> It should not introduce any noticeable issues or game speed changes in other cases
			const float diff = mAccumulatedTime - tickStableCenter;
			const float maxChange = tickLength * 0.1f;
			if (std::fabs(diff) < tickLength * 0.4f)	// Don't do this while close to the instable edges
			{
				mAccumulatedTime += clamp(-diff, -maxChange, maxChange);
				mLastCorrectionFrame = mFrameNumber;
			}
		}
	}

	VideoOut::instance().setInterFramePosition(saturate(mAccumulatedTime / tickLength));

#if 0
	// Meant for debugging of accumulated time stability
	if ((mFrameNumber % 6) == 0)
		LogDisplay::instance().setModeDisplay(String(0, "accum = %02d%%", roundToInt(mAccumulatedTime / tickLength * 100)));
#endif
}

bool Simulation::generateFrame()
{
	ControlsIn& controlsIn = ControlsIn::instance();
	const bool isGameRecorderPlayback = (Configuration::instance().mGameRecording == 2);
	const bool isGameRecorderRecording = (Configuration::instance().mGameRecording == 1);

	const bool beginningNewFrame = (mCodeExec.willBeginNewFrame() || isGameRecorderPlayback);
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

		// Game recorder playback
		if (isGameRecorderPlayback)
		{
			GameRecorder::PlaybackResult result;
			if (mGameRecorder.updatePlayback(result))
			{
				if (nullptr != result.mData)
				{
					SaveStateSerializer::StateType stateType;
					SaveStateSerializer serializer(mCodeExec, RenderParts::instance());

					const bool success = serializer.loadState(*result.mData, &stateType);
					if (success)
					{
						mCodeExec.reinitRuntime(nullptr, (stateType == SaveStateSerializer::StateType::GENSX) ? CodeExec::CallStackInitPolicy::READ_FROM_ASM : CodeExec::CallStackInitPolicy::USE_EXISTING);
						completedCurrentFrame = true;
					}
					else
						RMX_ERROR("Failed to load save state", );
				}

				ControlsIn::instance().injectInput(0, result.mInputs[0]);
				ControlsIn::instance().injectInput(1, result.mInputs[1]);
				inputWasInjected = true;
			}
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
			InputRecorder::InputState inputState;
			inputState.mInputFlags[0] = controlsIn.getInputPad(0);
			inputState.mInputFlags[1] = controlsIn.getInputPad(1);

			if ((mGameRecorder.getRangeEnd() % 180) == 0)	// Keyframe every 3 seconds
			{
				static std::vector<uint8> data;
				data.reserve(0x128000);
				data.clear();

				SaveStateSerializer serializer(mCodeExec, RenderParts::instance());
				serializer.saveState(data);

				mGameRecorder.addKeyFrame(inputState.mInputFlags, data);
				mGameRecorder.discardOldFrames(1800);
			}
			else
			{
				mGameRecorder.addFrame(inputState.mInputFlags);
			}
		}

		++mFrameNumber;
	}

	// Return false if frame got interrupted
	//  -> In this case, the outer loop should break
	//  -> Same if code execution is not possible any more
	return (completedCurrentFrame && mCodeExec.isCodeExecutionPossible());
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
	WString filename = L"gamerecording.bin";
	const std::string timeString = PlatformFunctions::getSystemTimeString();
	if (!timeString.empty())
	{
		filename.format(L"gamerecording_%s.bin", *String(timeString).toWString());
	}
	filename = WString(Configuration::instance().mAppDataPath) + L"gamerecordings/" + filename;

	if (!mGameRecorder.saveRecording(*filename))
		return 0;

	if (nullptr != outFilename)
		*outFilename = filename;

	return mGameRecorder.getCurrentNumberOfFrames();
}
