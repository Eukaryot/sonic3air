/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/client/GameClient.h"
#include "sonic3air/data/PlayerProgress.h"
#include "sonic3air/data/PlayerRecorder.h"
#include "sonic3air/helper/BlueSpheresRendering.h"
#include "sonic3air/resources/DynamicSprites.h"

#include "oxygen/resources/ResourcesCache.h"

namespace lemon
{
	class Module;
}


class Game : public SingleInstance<Game>
{
public:
	enum class Mode
	{
		UNDEFINED = 0,	// Undefined mode is used in debugging
		TITLE_SCREEN,	// SEGA logo, intro, title screen
		NORMAL_GAME,	// Normal game (started from the menu)
		ACT_SELECT,		// Act Select mode
		TIME_ATTACK,	// Time Attack mode
		COMPETITION,	// Competition mode
		BLUE_SPHERE,	// Blue Sphere game mode
		MAIN_MENU_BG	// Main menu background
	};

public:
	Game();

	void startup();
	void shutdown();
	void update(float timeElapsed);

	void registerScriptBindings(lemon::Module& module);

	uint32 getSetting(uint32 settingId, bool ignoreGameMode) const;
	void setSetting(uint32 settingId, uint32 value);

	void checkForUnlockedSecrets();

	void startIntoTitleScreen();
	void startIntoDataSelect();
	void startIntoLevel(Mode mode, uint32 submode, uint16 zoneAndAct, uint8 characters);
	void restartLevel();
	void restartAtCheckpoint();
	void restartTimeAttack(bool skipFadeout);
	void startIntoCompetitionMode();
	void startIntoBlueSphere();
	void startIntoLevelSelect();
	void startIntoMainMenuBG();

	void onPreUpdateFrame();
	void onPostUpdateFrame();
	void onUpdateControls();

	void updateSpecialInput(float timeElapsed);

	inline Mode getCurrentMode() const			{ return mMode; }
	inline void setCurrentMode(Mode mode)		{ mMode = mode; }

	inline bool isTimeAttackMode() const		{ return mMode == Mode::TIME_ATTACK; }
	inline PlayerRecorder& getPlayerRecorder()	{ return mPlayerRecorder; }

	bool shouldPauseOnFocusLoss() const;

	void fillDebugVisualization(Bitmap& bitmap, int& mode);

	void onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer);
	void onGameRecordingHeaderSave(std::vector<uint8>& buffer);

private:
	void startIntoGameInternal();

	// Script bindings
	uint32 useSetting(uint32 settingId);

	int32 getAchievementValue(uint32 achievementId);
	void setAchievementValue(uint32 achievementId, int32 value);
	bool isAchievementComplete(uint32 achievementId);
	void setAchievementComplete(uint32 achievementId);

	bool isSecretUnlocked(uint32 secretId);
	void setSecretUnlocked(uint32 secretId);

	void triggerRestart();
	void onGamePause(uint8 canRestart);
	void allowRestartInGamePause(uint8 canRestart);
	void onLevelStart();
	void onZoneActCompleted(uint16 zoneAndAct);
	uint16 onTriggerNextZone(uint16 zoneAndAct);
	uint16 onFadedOutLoadingZone(uint16 zoneAndAct);
	bool onCharacterDied(uint8 playerIndex);
	void returnToMainMenu();

	inline bool isNormalGame()	{ return mMode == Mode::NORMAL_GAME; }
	inline bool isTimeAttack()	{ return mMode == Mode::TIME_ATTACK; }
	bool onTimeAttackFinish();

	void changePlanePatternRectAtex(uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex);

	void setupBlueSpheresGroundSprites();
	void writeBlueSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation);

	void startSkippableCutscene();
	void endSkippableCutscene();

private:
	Mode mMode = Mode::UNDEFINED;
	uint32 mSubMode = 0;

	BlueSpheresRendering mBlueSpheresRendering;
	PlayerProgress mPlayerProgress;
	PlayerRecorder mPlayerRecorder;
	GameClient mGameClient;
	DynamicSprites mDynamicSprites;

	uint16 mLastZoneAndAct = 0;
	uint8  mLastCharacters = 0;

	bool mReceivedTimeAttackFinished = false;
	bool mReturnToMenuTriggered = false;
	bool mRestartTriggered = false;
	bool mRestartingTimeAttack = false;
	bool mAllowRestartInGamePause = false;

	uint32 mSkippableCutsceneFrames = 0;	// If > 0, a skippable cutscene is active
	bool mButtonYPressedDuringSkippableCutscene = false;

	bool mTimeAttackRestartCharging = false;
	float mTimeAttackRestartCharge = 0.0f;

	float mTimeoutUntilDiscordRefresh = 1.0f;
};
