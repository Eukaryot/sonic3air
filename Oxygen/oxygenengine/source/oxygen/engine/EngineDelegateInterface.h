/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class AudioOutBase;
class CodeExec;
class EmulatorInterface;
class GuiBase;

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
		std::string  mInternalName;
		std::wstring mIconFile;
		int			 mWindowsIconResource = 0;
		std::string	 mBuildVersionString;
		uint32		 mBuildVersionNumber = 0;
		std::wstring mAppDataFolder;
	};

public:
	virtual const AppMetaData& getAppMetaData() = 0;
	virtual GuiBase& createGameApp() = 0;
	virtual AudioOutBase& createAudioOut() = 0;

	virtual bool onEnginePreStartup() = 0;
	virtual bool isDedicatedApplication() { return false; }		// This is expected to be true for dedicated game applications, like S3AIR
	virtual bool setupCustomGameProfile() = 0;

	virtual void startupGame(EmulatorInterface& emulatorInterface) = 0;
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
	virtual void onActiveModsChanged() = 0;

	virtual void onStartNetplayGame(bool isHost) = 0;
	virtual void onStopNetplayGame(bool isHost) = 0;
	virtual void serializeGameSettings(VectorBinarySerializer& serializer) = 0;

	virtual void onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer) = 0;
	virtual void onGameRecordingHeaderSave(std::vector<uint8>& buffer) = 0;

	virtual void fillDebugVisualization(Bitmap& bitmap, int& mode) = 0;
};
