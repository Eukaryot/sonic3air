#include "engineapp/pch.h"
#include "engineapp/EngineDelegate.h"
#include "engineapp/GameApp.h"
#include "engineapp/audio/AudioOut.h"


const EngineDelegateInterface::AppMetaData& EngineDelegate::getAppMetaData()
{
	if (mAppMetaData.mTitle.empty())
	{
		mAppMetaData.mTitle = "Oxygen Engine";
		mAppMetaData.mBuildVersionString = "0.1.0";		// Oxygen Engine currently doesn't use a version number to take serious in any way...
		mAppMetaData.mBuildVersionNumber = 0x00010000;
	}
	mAppMetaData.mAppDataFolder = L"OxygenEngine";
	return mAppMetaData;
}

GuiBase& EngineDelegate::createGameApp()
{
	return *new GameApp();
}

AudioOutBase& EngineDelegate::createAudioOut()
{
	return *new AudioOut();
}

bool EngineDelegate::onEnginePreStartup()
{
	return true;
}

bool EngineDelegate::setupCustomGameProfile()
{
	// Return false to signal that there's no custom game profile, and the oxygenproject.json should be loaded instead
	return false;
}

void EngineDelegate::startupGame(EmulatorInterface& emulatorInterface)
{
}

void EngineDelegate::shutdownGame()
{
}

void EngineDelegate::updateGame(float timeElapsed)
{
}

void EngineDelegate::registerScriptBindings(lemon::Module& module)
{
#ifdef USE_EXPERIMENTS
	mExperiments.registerScriptBindings(module);
#endif
}

void EngineDelegate::registerNativizedCode(lemon::Program& program)
{
}

void EngineDelegate::onRuntimeInit(CodeExec& codeExec)
{
}

void EngineDelegate::onPreFrameUpdate()
{
#ifdef USE_EXPERIMENTS
	mExperiments.onPreFrameUpdate();
#endif
}

void EngineDelegate::onPostFrameUpdate()
{
#ifdef USE_EXPERIMENTS
	mExperiments.onPostFrameUpdate();
#endif
}

void EngineDelegate::onControlsUpdate()
{
}

void EngineDelegate::onPreSaveStateLoad()
{
}

bool EngineDelegate::mayLoadScriptMods()
{
	return true;
}

bool EngineDelegate::allowModdedData()
{
	return true;
}

bool EngineDelegate::useDeveloperFeatures()
{
	return true;
}

void EngineDelegate::onActiveModsChanged()
{
}

void EngineDelegate::onStartNetplayGame()
{
}

void EngineDelegate::onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer)
{
}

void EngineDelegate::onGameRecordingHeaderSave(std::vector<uint8>& buffer)
{
}

Font& EngineDelegate::getDebugFont(int size)
{
	if (size >= 10)
	{
		static Font font10;
		if (font10.getLineHeight() == 0)
			font10.loadFromFile("data/font/oxyfont_regular.json", 0.0f);
		return font10;
	}
	else
	{
		static Font font3;
		if (font3.getLineHeight() == 0)
			font3.loadFromFile("data/font/smallfont.json", 0.0f);
		return font3;
	}
}

void EngineDelegate::fillDebugVisualization(Bitmap& bitmap, int& mode)
{
}
