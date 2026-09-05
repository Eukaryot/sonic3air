/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"
#include "oxygen/drawing/Drawer.h"
#include "oxygen/engine/EngineDelegateInterface.h"

class ArgumentsReader;
class PackedFileProvider;

namespace oxygen
{
	class EngineSystems;
}


class EngineMain : public SingleInstance<EngineMain>
{
public:
	static EngineDelegateInterface& getDelegate()  { return EngineMain::instance().mDelegate; }
	static void earlySetup();

public:
	EngineMain(EngineDelegateInterface& delegate_, ArgumentsReader& arguments);
	~EngineMain();

	void execute();

	void onActiveModsChanged();
	bool reloadFilePackage(std::wstring_view packageName, bool forceReload);

	inline AudioOutBase& getAudioOut()  { return *mAudioOut; }

	inline SDL_Window& getSDLWindow() const	{ return *mSDLWindow; }
	inline Drawer& getDrawer()				{ return mDrawer; }

	uint32 getPlatformFlags() const;
	void switchToRenderMethod(Configuration::RenderMethod newRenderMethod);
	void setVSyncMode(Configuration::FrameSyncType frameSyncMode);
	Vec2i getDisplaySize(int displayIndex) const;

private:
	bool startupEngine();
	void run();
	void shutdown();

	void initDirectories();
	bool initConfigAndSettings();
	void loadConfigJson();
	void updateGameProfilePaths();

	bool initFileSystem();
	bool loadFilePackages(bool forceReload);
	bool loadFilePackageByIndex(size_t index, bool forceReload);

	bool createWindow();
	void destroyWindow();

private:
	EngineDelegateInterface& mDelegate;
	ArgumentsReader& mArguments;

	oxygen::EngineSystems& mSystems;

	AudioOutBase* mAudioOut = nullptr;
	SDL_Window*	  mSDLWindow = nullptr;
	Drawer		  mDrawer;
	std::vector<PackedFileProvider*> mPackedFileProviders;
};
