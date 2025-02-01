/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/overlays/SecretUnlockedWindow.h"
#include "oxygen/drawing/DrawerTexture.h"

class GameView;
class GameMenuManager;
class MenuBackground;
class PauseMenu;
class TimeAttackResultsMenu;
class SkippableCutsceneWindow;
class ApplicationContextMenu;


class GameApp : public GuiBase, public SingleInstance<GameApp>
{
public:
	GameApp();
	~GameApp();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void onStartGame();
	void openTitleScreen();
	void openMainMenu();
	void openOptionsMenuInGame();
	void onFadedOutOptions();
	void onGamePaused(bool canRestart);
	void restartTimeAttack();
	void returnToMenu();

	void showTimeAttackResults(int hundreds, const std::vector<int>& otherTimes);
	void enableStillImageBlur(bool enable, float timeout = 0.0f);

	void showUnlockedWindow(SecretUnlockedWindow::EntryType entryType, const std::string& title, const std::string& content);
	void showSkippableCutsceneWindow(bool show);

	inline GameView& getGameView() const { return *mGameView; }
	inline GameMenuManager& getGameMenuManager() const  { return *mGameMenuManager; }
	inline MenuBackground& getMenuBackground() const	{ return *mMenuBackground; }

private:
	void gotoPhase(int phaseNumber);

private:
	enum class State
	{
		UNDEFINED = 0,
		DISCLAIMER,
		TITLE_SCREEN,
		MAIN_MENU,
		INGAME_OPTIONS,
		INGAME,
		TIME_ATTACK_RESULTS
	};
	State mCurrentState = State::UNDEFINED;
	float mStateTimeout = 0.0f;

	DrawerTexture mDisclaimerTexture;
	float mDisclaimerVisibility = 0.0f;

	GameView* mGameView = nullptr;
	GameMenuManager* mGameMenuManager = nullptr;
	MenuBackground* mMenuBackground = nullptr;
	PauseMenu* mPauseMenu = nullptr;
	TimeAttackResultsMenu* mTimeAttackResultsMenu = nullptr;
	SecretUnlockedWindow* mSecretUnlockedWindow = nullptr;
	SkippableCutsceneWindow* mSkippableCutsceneWindow = nullptr;
	ApplicationContextMenu* mApplicationContextMenu = nullptr;

	GuiBase* mRemoveChild = nullptr;
};
