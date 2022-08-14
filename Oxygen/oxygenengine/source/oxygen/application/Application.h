/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"
#include "oxygen/helper/HighResolutionTimer.h"

class AudioPlayer;
class BackdropView;
class CheatSheetOverlay;
class DebugSidePanel;
class GameApp;
class GameLoader;
class GameSetupScreen;
class GameView;
class OxygenMenu;
class ProfilingView;
class SaveStateMenu;
class Simulation;
class TouchControlsOverlay;


class Application : public GuiBase, public SingleInstance<Application>
{
public:
	using WindowMode = Configuration::WindowMode;

public:
	Application();
	~Application();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void sdlEvent(const SDL_Event& ev) override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void childClosed(GuiBase& child);

	inline Simulation& getSimulation()		   { return *mSimulation; }
	inline GameView& getGameView()			   { return *mGameView; }
	inline DebugSidePanel* getDebugSidePanel() { return mDebugSidePanel; }

	WindowMode getWindowMode() const		   { return mWindowMode; }
	void setWindowMode(WindowMode windowMode, bool force = false);
	void toggleFullscreen();

	void enablePauseOnFocusLoss();
	void triggerGameRecordingSave();

	bool hasKeyboard() const;
	bool hasVirtualGamepad() const;

private:
	int updateWindowDisplayIndex();
	void setUnscaledWindow();
	bool updateLoading();
	void setPausedByFocusLoss(bool enable);

private:
	WindowMode mWindowMode = WindowMode::WINDOWED;
	HighResolutionTimer mApplicationTimer;
	double mNextRefreshTime = 0.0;		// In milliseconds since application start
	bool mIsVeryFirstFrameForLogging = true;
	bool mPausedByFocusLoss = false;

	// Simulation
	Simulation* mSimulation = nullptr;

	// Game
	GameLoader* mGameLoader = nullptr;
	GuiBase* mGameApp = nullptr;
	GameView* mGameView = nullptr;

	// GUI
	Font mLogDisplayFont;
	GuiBase* mRemoveChild = nullptr;

	BackdropView* mBackdropView = nullptr;
	TouchControlsOverlay* mTouchControlsOverlay = nullptr;
	CheatSheetOverlay* mCheatSheetOverlay = nullptr;
	GameSetupScreen* mGameSetupScreen = nullptr;
	OxygenMenu* mOxygenMenu = nullptr;
	SaveStateMenu* mSaveStateMenu = nullptr;
	DebugSidePanel* mDebugSidePanel = nullptr;
	ProfilingView* mProfilingView = nullptr;
	
	// Input
	float mMouseHideTimer = 0.0f;
};
