/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"
#include "oxygen/helper/HighResolutionTimer.h"
#include "oxygen/menu/imgui/ImGuiIntegration.h"

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

	virtual void beginFrame() override;
	virtual void endFrame() override;

	virtual void sdlEvent(const SDL_Event& ev) override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void childClosed(GuiBase& child);

	inline Simulation& getSimulation()						{ return *mSimulation; }
	inline GameView& getGameView()							{ return *mGameView; }
	inline TouchControlsOverlay* getTouchControlsOverlay()	{ return mTouchControlsOverlay; }
	inline DebugSidePanel* getDebugSidePanel()				{ return mDebugSidePanel; }

	WindowMode getWindowMode() const  { return mWindowMode; }
	void setWindowMode(WindowMode windowMode, bool force = false);
	void toggleFullscreen();

	void enablePauseOnFocusLoss();
	void triggerGameRecordingSave();

	bool hasKeyboard() const;
	bool hasVirtualGamepad() const;

	void requestActiveTextInput();

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

	ImGuiIntegration mImGuiIntegration;

	// Input
	float mMouseHideTimer = 0.0f;
	bool mRequestActiveTextInput = false;
};
