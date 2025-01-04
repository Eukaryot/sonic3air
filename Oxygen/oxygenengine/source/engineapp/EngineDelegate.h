#pragma once

#include "engineapp/ConfigurationImpl.h"
#include "engineapp/experiments/Experiments.h"
#include "engineapp/version.inc"

#include "oxygen/application/EngineMain.h"


class EngineDelegate : public EngineDelegateInterface
{
public:
	const AppMetaData& getAppMetaData() override;
	GuiBase& createGameApp() override;
	AudioOutBase& createAudioOut() override;

	bool onEnginePreStartup() override;
	bool setupCustomGameProfile() override;

	void startupGame(EmulatorInterface& emulatorInterface) override;
	void shutdownGame() override;
	void updateGame(float timeElapsed) override;

	void registerScriptBindings(lemon::Module& module) override;
	void registerNativizedCode(lemon::Program& program) override;

	void onRuntimeInit(CodeExec& codeExec) override;
	void onPreFrameUpdate() override;
	void onPostFrameUpdate() override;
	void onControlsUpdate() override;
	void onPreSaveStateLoad() override;

	bool mayLoadScriptMods() override;
	bool allowModdedData() override;
	bool useDeveloperFeatures() override;
	void onActiveModsChanged() override;

	void onStartNetplayGame(bool isHost) override;
	void onStopNetplayGame(bool isHost) override;
	void serializeGameSettings(VectorBinarySerializer& serializer) override;

	void onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer) override;
	void onGameRecordingHeaderSave(std::vector<uint8>& buffer) override;

	Font& getDebugFont(int size) override;
	void fillDebugVisualization(Bitmap& bitmap, int& mode) override;

private:
	AppMetaData mAppMetaData;
	ConfigurationImpl mConfiguration;

#ifdef USE_EXPERIMENTS
	Experiments mExperiments;
#endif
};
