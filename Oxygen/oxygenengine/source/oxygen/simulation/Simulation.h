/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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


class Simulation
{
public:
	Simulation();
	~Simulation();

	bool startup();
	void shutdown();

	void reloadScriptsAfterModsChange();

	inline bool isRunning() const		  { return mIsRunning; }
	inline void setRunning(bool running)  { mIsRunning = running; }

	CodeExec& getCodeExec()				  { return mCodeExec; }
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

	float getSimulationFrequency() const;
	void setSimulationFrequencyOverride(float frequency) { mSimulationFrequencyOverride = frequency; }
	void disableSimulationFrequencyOverride()			 { mSimulationFrequencyOverride = 0.0f; }

	inline float getSpeed() const  { return mSimulationSpeed; }
	void setSpeed(float emulatorSpeed);
	inline float getDefaultSpeed() const  { return mDefaultSimulationSpeed; }
	inline void setDefaultSpeed(float defaultSpeed)  { mDefaultSimulationSpeed = defaultSpeed; }
	void setNextSingleStep(bool singleStep, bool continueToDebugEvent = false);
	void stopSingleStepContinue();

	void refreshDebugging();

	uint32 saveGameRecording(WString* outFilename = nullptr);

private:
	CodeExec& mCodeExec;
	GameRecorder& mGameRecorder;
	InputRecorder& mInputRecorder;
	ROMDataAnalyser* mROMDataAnalyser = nullptr;

	bool	mIsRunning = false;
	float	mSimulationFrequencyOverride = 0.0f;
	float	mSimulationSpeed = 1.0f;
	float	mDefaultSimulationSpeed = 1.0f;
	bool	mNextSingleStep = false;
	bool	mSingleStepContinue = false;

	double	mCurrentTargetFrame = 0.0f;
	uint32	mFrameNumber = 0;
	uint32	mLastCorrectionFrame = 0;

	std::wstring mStateLoaded;
};
