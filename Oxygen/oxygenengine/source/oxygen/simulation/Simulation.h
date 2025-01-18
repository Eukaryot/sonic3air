/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

class AudioOutBase;
class EmulatorInterface;
class CodeExec;
class GameRecorder;
class InputRecorder;
class ROMDataAnalyser;
class SimulationState;


class Simulation
{
public:
	enum class BreakCondition
	{
		DEBUG_LOG	= 0x01,
		WATCH_HIT	= 0x02,
	};

public:
	Simulation();
	~Simulation();

	bool startup();
	void shutdown();

	void reloadScriptsAfterModsChange();

	inline bool isRunning() const		  { return mIsRunning; }
	inline void setRunning(bool running)  { mIsRunning = running; }

	CodeExec& getCodeExec()				  { return mCodeExec; }
	SimulationState& getSimulationState() { return mSimulationState; }
	ROMDataAnalyser* getROMDataAnalyser() { return mROMDataAnalyser; }
	EmulatorInterface& getEmulatorInterface();

	void resetState();
	void resetIntoGame(const std::vector<std::pair<std::string, std::string>>* enforcedCallStack);
	void resetIntoGame(const std::string& entryFunctionName);

	void reloadLastState();
	bool loadState(const std::wstring& filename, bool showError = true);
	void saveState(const std::wstring& filename);

	bool triggerFullScriptsReload();

	inline uint32 getFrameNumber() const  { return mFrameNumber; }

	void update(float timePassed);
	bool generateFrame();
	bool jumpToFrame(uint32 frameNumber, bool clearRecordingAfterwards = true);

	inline void setRewind(int rewindSteps) { mRewindSteps = rewindSteps; }

	float getSimulationFrequency() const;
	void setSimulationFrequencyOverride(float frequency) { mSimulationFrequencyOverride = frequency; }
	void disableSimulationFrequencyOverride()			 { mSimulationFrequencyOverride = 0.0f; }

	inline float getSpeed() const  { return mSimulationSpeed; }
	void setSpeed(float emulatorSpeed);
	inline float getDefaultSpeed() const  { return mDefaultSimulationSpeed; }
	inline void setDefaultSpeed(float defaultSpeed)  { mDefaultSimulationSpeed = defaultSpeed; }

	inline bool hasStepsLimit() const  { return mStepsLimit >= 0; }
	void setNextSingleStep();
	void removeStepsLimit();

	inline bool hasBreakCondition(BreakCondition breakCondition) const  { return mBreakConditions.isSet(breakCondition); }
	void setBreakCondition(BreakCondition breakCondition, bool enable);
	void sendBreakSignal(BreakCondition breakCondition);

	void refreshDebugging();

	uint32 saveGameRecording(WString* outFilename = nullptr);

private:
	CodeExec& mCodeExec;
	SimulationState& mSimulationState;
	GameRecorder& mGameRecorder;
	InputRecorder& mInputRecorder;
	ROMDataAnalyser* mROMDataAnalyser = nullptr;

	bool	mIsRunning = false;
	float	mSimulationFrequencyOverride = 0.0f;
	float	mSimulationSpeed = 1.0f;
	float	mDefaultSimulationSpeed = 1.0f;
	int		mStepsLimit = -1;
	BitFlagSet<BreakCondition> mBreakConditions;

	double	mCurrentTargetFrame = 0.0f;
	uint32	mFrameNumber = 0;
	uint32	mLastCorrectionFrame = 0;
	int		mRewindSteps = -1;		// -1 is no rewind enabled; 0 if rewind is enabled but inside delay before next rewind step; higher values for number of steps to rewind

	std::wstring mStateLoaded;
};
