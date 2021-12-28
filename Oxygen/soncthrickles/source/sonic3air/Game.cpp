/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/Game.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/data/TimeAttackData.h"
#include "sonic3air/helper/GameUtils.h"
#include "sonic3air/helper/DiscordIntegration.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/scriptimpl/ScriptImplementations.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/drawing/software/Blitter.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/FunctionWrapper.h>
#include <lemon/program/Module.h>


namespace
{
	const constexpr float CUTSCENE_SKIPPING_SPEED = 4.0f;	// Should be more or less "unique", not one of the debug game speeds (3.0f or 5.0f)

	void setDiscordDetails(lemon::StringRef text)
	{
		if (text.isValid())
			DiscordIntegration::setModdedDetails(*text);
	}

	void setDiscordState(lemon::StringRef text)
	{
		if (text.isValid())
			DiscordIntegration::setModdedState(*text);
	}

	void setDiscordLargeImage(lemon::StringRef imageName)
	{
		if (imageName.isValid())
			DiscordIntegration::setModdedLargeImage(*imageName);
	}

	void setDiscordSmallImage(lemon::StringRef imageName)
	{
		if (imageName.isValid())
			DiscordIntegration::setModdedSmallImage(*imageName);
	}

	bool isModdedSound(uint8 sfxId)
	{
		return AudioOut::instance().isModdedSound(sfxId);
	}

	void setUnderwaterAudioEffect(uint8 value)
	{
		return AudioOut::instance().enableUnderwaterEffect((float)value / 255.0f);
	}
}


Game::Game()
{
}

void Game::startup()
{
	mPlayerProgress.load();
	mBlueSpheresRendering.startup();
	mGameClient.setupClient();

	DiscordIntegration::startup();
}

void Game::shutdown()
{
	mPlayerProgress.save();

	DiscordIntegration::shutdown();
}

void Game::update(float timeElapsed)
{
	// Update game client
	mGameClient.updateClient(timeElapsed);

	// Discord rich presence update
	{
		mTimeoutUntilDiscordRefresh -= timeElapsed;
		if (mTimeoutUntilDiscordRefresh <= 0.0f)
		{
			DiscordIntegration::updateInfo(mMode, mSubMode, EmulatorInterface::instance());
			mTimeoutUntilDiscordRefresh = 3.0f;
		}
		DiscordIntegration::update();
	}

#ifndef ENDUSER
	// Just for testing
	static bool pressed = false;
	if (FTX::keyState('x') != pressed)
	{
		pressed = !pressed;
		if (pressed)
		{
			if (FTX::keyState(SDLK_LSHIFT))
				GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, "Secret unlocked!", "Knuckles & Tails character combination can now be selected in Normal Game and Act Select.");
			else
				GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::ACHIEVEMENT, "Achievement unlocked!", "Cheated an achievement. Well done!");
		}
	}
#endif

	if (mReturnToMenuTriggered)
	{
		Application::instance().getSimulation().setSpeed(0.0f);
		GameApp::instance().returnToMenu();
		mReturnToMenuTriggered = false;
	}
	else if (mRestartTriggered)
	{
		startIntoLevel(mMode, mSubMode, mLastZoneAndAct, mLastCharacters);
		mRestartTriggered = false;
	}
}

void Game::registerScriptBindings(lemon::Module& module)
{
	const uint8 defaultFlags = lemon::UserDefinedFunction::FLAG_ALLOW_INLINE_EXECUTION;
	const uint8 noInlineExecution = 0;

	// Game
	{
		module.addUserDefinedFunction("Game.getSetting", lemon::wrap(*this, &Game::useSetting), defaultFlags)
			.setParameterInfo(0, "settingId");

		module.addUserDefinedFunction("Game.isSecretUnlocked", lemon::wrap(*this, &Game::isSecretUnlocked), defaultFlags)
			.setParameterInfo(0, "secretId");

		module.addUserDefinedFunction("Game.triggerRestart", lemon::wrap(*this, &Game::triggerRestart), defaultFlags);

		module.addUserDefinedFunction("Game.onGamePause", lemon::wrap(*this, &Game::onGamePause), defaultFlags)
			.setParameterInfo(0, "canRestart");

		module.addUserDefinedFunction("Game.allowRestartInGamePause", lemon::wrap(*this, &Game::allowRestartInGamePause), defaultFlags)
			.setParameterInfo(0, "canRestart");

		module.addUserDefinedFunction("Game.onLevelStart", lemon::wrap(*this, &Game::onLevelStart), defaultFlags);

		module.addUserDefinedFunction("Game.onZoneActCompleted", lemon::wrap(*this, &Game::onZoneActCompleted), defaultFlags)
			.setParameterInfo(0, "zoneAndAct");

		module.addUserDefinedFunction("Game.onTriggerNextZone", lemon::wrap(*this, &Game::onTriggerNextZone), defaultFlags)
			.setParameterInfo(0, "zoneAndAct");

		module.addUserDefinedFunction("Game.onFadedOutLoadingZone", lemon::wrap(*this, &Game::onFadedOutLoadingZone), defaultFlags)
			.setParameterInfo(0, "zoneAndAct");

		module.addUserDefinedFunction("Game.onCharacterDied", lemon::wrap(*this, &Game::onCharacterDied), noInlineExecution)		// No inline execution as this function manipulated the call stack
			.setParameterInfo(0, "playerIndex");

		module.addUserDefinedFunction("Game.onScreenFadedOutBeforeDataSelect", lemon::wrap(*this, &Game::onScreenFadedOutBeforeDataSelect), defaultFlags);
		module.addUserDefinedFunction("Game.returnToMainMenu", lemon::wrap(*this, &Game::returnToMainMenu), defaultFlags);
		module.addUserDefinedFunction("Game.isNormalGame", lemon::wrap(*this, &Game::isNormalGame), defaultFlags);
		module.addUserDefinedFunction("Game.isTimeAttack", lemon::wrap(*this, &Game::isTimeAttack), defaultFlags);
		module.addUserDefinedFunction("Game.onTimeAttackFinish", lemon::wrap(*this, &Game::onTimeAttackFinish), defaultFlags);

		module.addUserDefinedFunction("Game.changePlanePatternRectAtex", lemon::wrap(*this, &Game::changePlanePatternRectAtex), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "width")
			.setParameterInfo(3, "height")
			.setParameterInfo(4, "planeIndex")
			.setParameterInfo(5, "atex");

		module.addUserDefinedFunction("Game.renderBlueSpheresGround", lemon::wrap(*this, &Game::renderBlueSpheresGround), defaultFlags)
			.setParameterInfo(0, "px")
			.setParameterInfo(1, "py")
			.setParameterInfo(2, "rotation")
			.setParameterInfo(3, "fieldColorA")
			.setParameterInfo(4, "fieldColorB");

		module.addUserDefinedFunction("Game.getBlueSpheresGroundSprite", lemon::wrap(*this, &Game::getBlueSpheresGroundSprite), defaultFlags)
			.setParameterInfo(0, "part");

		module.addUserDefinedFunction("Game.writeBlueSpheresData", lemon::wrap(*this, &Game::writeBlueSpheresData), defaultFlags)
			.setParameterInfo(0, "targetAddress")
			.setParameterInfo(1, "sourceAddress")
			.setParameterInfo(2, "px")
			.setParameterInfo(3, "py")
			.setParameterInfo(4, "rotation");

		module.addUserDefinedFunction("Game.getAchievementValue", lemon::wrap(*this, &Game::getAchievementValue), defaultFlags)
			.setParameterInfo(0, "achievementId");

		module.addUserDefinedFunction("Game.setAchievementValue", lemon::wrap(*this, &Game::setAchievementValue), defaultFlags)
			.setParameterInfo(0, "achievementId")
			.setParameterInfo(1, "value");

		module.addUserDefinedFunction("Game.isAchievementComplete", lemon::wrap(*this, &Game::isAchievementComplete), defaultFlags)
			.setParameterInfo(0, "achievementId");

		module.addUserDefinedFunction("Game.setAchievementComplete", lemon::wrap(*this, &Game::setAchievementComplete), defaultFlags)
			.setParameterInfo(0, "achievementId");

		module.addUserDefinedFunction("Game.startSkippableCutscene", lemon::wrap(*this, &Game::startSkippableCutscene), defaultFlags);
		module.addUserDefinedFunction("Game.endSkippableCutscene", lemon::wrap(*this, &Game::endSkippableCutscene), defaultFlags);
	}

	// Discord
	{
		module.addUserDefinedFunction("Game.setDiscordDetails", lemon::wrap(&setDiscordDetails), defaultFlags)
			.setParameterInfo(0, "text");

		module.addUserDefinedFunction("Game.setDiscordState", lemon::wrap(&setDiscordState), defaultFlags)
			.setParameterInfo(0, "text");

		module.addUserDefinedFunction("Game.setDiscordLargeImage", lemon::wrap(&setDiscordLargeImage), defaultFlags)
			.setParameterInfo(0, "imageName");

		module.addUserDefinedFunction("Game.setDiscordSmallImage", lemon::wrap(&setDiscordSmallImage), defaultFlags)
			.setParameterInfo(0, "imageName");
	}

	// Audio
	{
		module.addUserDefinedFunction("Game.isModdedSound", lemon::wrap(&isModdedSound), defaultFlags)
			.setParameterInfo(0, "sfxId");

		module.addUserDefinedFunction("Game.setUnderwaterAudioEffect", lemon::wrap(&setUnderwaterAudioEffect), defaultFlags)
			.setParameterInfo(0, "value");
	}

	ScriptImplementations::registerScriptBindings(module);
}

uint32 Game::getSetting(uint32 settingId, bool ignoreGameMode) const
{
	if (!ignoreGameMode && isTimeAttackMode())
	{
		// In Time Attack, non-visual settings are always using the default value
		if ((settingId & 0x80000000) == 0)
		{
			// Only exception are the "max control" settings
			if (settingId == SharedDatabase::Setting::SETTING_DROPDASH || settingId == SharedDatabase::Setting::SETTING_SUPER_PEELOUT)
			{
				return (mSubMode == 0x11) ? 1 : 0;
			}
			else
			{
				return (settingId & 0xff);
			}
		}
	}

	const SharedDatabase::Setting* setting = SharedDatabase::getSetting(settingId);
	if (nullptr != setting)
	{
		return setting->mValue;
	}

	// Use default value
	return (settingId & 0xff);
}

void Game::setSetting(uint32 settingId, uint32 value)
{
	const SharedDatabase::Setting* setting = SharedDatabase::getSetting(settingId);
	RMX_CHECK(nullptr != setting, "Setting not found", return);
	setting->mValue = value;
}

void Game::checkForUnlockedSecrets()
{
	// Check for unlocked secrets
	uint32 achievementsCompleted = 0;
	for (const SharedDatabase::Achievement& achievement : SharedDatabase::getAchievements())
	{
		if (mPlayerProgress.getAchievementState(achievement.mType) > 0)
		{
			++achievementsCompleted;
		}
	}

	for (const SharedDatabase::Secret& secret : SharedDatabase::getSecrets())
	{
		if (!mPlayerProgress.isSecretUnlocked(secret.mType) && achievementsCompleted >= secret.mRequiredAchievements)
		{
			// Unlock secret now
			mPlayerProgress.setSecretUnlocked(secret.mType);
			GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, "Secret unlocked!", secret.mName);
		}
	}
}

void Game::startIntoDataSelect()
{
	mMode = Mode::NORMAL_GAME;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.dataSelect");

	startIntoGameInternal();
}

void Game::startIntoLevel(Mode mode, uint32 submode, uint16 zoneAndAct, uint8 characters)
{
	mMode = mode;
	mSubMode = submode;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.actSelect");

	startIntoGameInternal();
	mLastZoneAndAct = zoneAndAct;
	mLastCharacters = characters;

	EmulatorInterface::instance().writeMemory16(0xfffffe10, zoneAndAct);
	EmulatorInterface::instance().writeMemory16(0xffffff0a, characters);

	if (mMode == Mode::TIME_ATTACK)
	{
		mPlayerRecorder.setMaxGhosts(getSetting(SharedDatabase::Setting::SETTING_TIME_ATTACK_GHOSTS, true));

		const uint8 category = submode;
		std::wstring recBaseFilename;
		const std::wstring recordingsDir = TimeAttackData::getSavePath(zoneAndAct, category, &recBaseFilename);

		if (!recordingsDir.empty())
		{
			mPlayerRecorder.initForDirectory(recordingsDir, recBaseFilename, zoneAndAct, category);
		}
	}

	GameApp::instance().enableStillImageBlur(false);
}

void Game::restartLevel()
{
	Simulation& simulation = Application::instance().getSimulation();
	simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartLevel");
	simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
}

void Game::restartAtCheckpoint()
{
	Simulation& simulation = Application::instance().getSimulation();
	simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartAtCheckpoint");
	simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
}

void Game::restartTimeAttack(bool skipFadeout)
{
	if (skipFadeout)
	{
		startIntoLevel(mMode, mSubMode, mLastZoneAndAct, mLastCharacters);
	}
	else
	{
		if (!mRestartingTimeAttack)
		{
			mRestartingTimeAttack = true;
			Simulation& simulation = Application::instance().getSimulation();
			simulation.getCodeExec().getLemonScriptRuntime().callFunctionByName("restartTimeAttack");
			simulation.setSpeed(simulation.getDefaultSpeed());		// Because it could be paused
		}
	}
}

void Game::startIntoCompetitionMode()
{
	mMode = Mode::COMPETITION;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.competitionMode");

	startIntoGameInternal();
}

void Game::startIntoBlueSphere()
{
	mMode = Mode::BLUE_SPHERE;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.blueSphereGame");

	startIntoGameInternal();
}

void Game::startIntoLevelSelect()
{
	mMode = Mode::ACT_SELECT;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.resetIntoGame("EntryFunctions.levelSelect");

	startIntoGameInternal();
}

void Game::onPreUpdateFrame()
{
}

void Game::onPostUpdateFrame()
{
	EmulatorInterface& emulatorInterface = EmulatorInterface::instance();

	// Check for invalid game mode inside simulation
	{
		const uint8 gameMode = emulatorInterface.readMemory8(0xfffff600) & 0x7f;
		bool isValidGameMode = true;
		if (mMode == Mode::TIME_ATTACK)
		{
			// Only allowed is the main game
			isValidGameMode = (gameMode <= 0x0c);
		}

		if (!isValidGameMode)
		{
			mMode = Mode::UNDEFINED;
			Application::instance().getSimulation().setSpeed(0.0f);
			GameApp::instance().returnToMenu();
			return;
		}
	}

	// Update skippable cutscene
	if (mSkippableCutsceneFrames > 0)
	{
		--mSkippableCutsceneFrames;
		if (mSkippableCutsceneFrames == 0)
		{
			endSkippableCutscene();
		}
		else
		{
			Simulation& simulation = Application::instance().getSimulation();
			if (mButtonYPressedDuringSkippableCutscene)
			{
				if (simulation.getSpeed() == 1.0f)
				{
					simulation.setSpeed(CUTSCENE_SKIPPING_SPEED);
					GameApp::instance().showSkippableCutsceneWindow(true);
				}
			}
			else
			{
				if (simulation.getSpeed() == CUTSCENE_SKIPPING_SPEED)
				{
					simulation.setSpeed(simulation.getDefaultSpeed());
				}
				GameApp::instance().showSkippableCutsceneWindow(false);
			}
		}
	}

	// Update player recorder
	mPlayerRecorder.onPostUpdateFrame();

	// Update ghost sync
	GameClient::instance().getGhostSync().onPostUpdateFrame();

	// Check for unlocked hidden secrets
	//  - SECRET_LEVELSELECT	unlocked when u8[0x02219e] changes from 0xb2 to 0x14
	//  - SECRET_TITLE_SK		unlocked when u8[0x065fde] changes from 0x08 to 0x93
	//  - SECRET_GAME_SPEED		unlocked when u64[0x003e32] changes from 0xd522427870e16100 to 0x0101020201010101 (= up, up, down, down, up, up, up, up)
	if (emulatorInterface.readMemory8(0x02219e) == 0x14)
	{
		setSecretUnlocked(SharedDatabase::Secret::SECRET_LEVELSELECT);
	}
	if (emulatorInterface.readMemory8(0x065fde) == 0x93)
	{
		setSecretUnlocked(SharedDatabase::Secret::SECRET_TITLE_SK);
	}
	if (emulatorInterface.readMemory64(0x003e32) == 0x0101020201010101ull)
	{
		setSecretUnlocked(SharedDatabase::Secret::SECRET_GAME_SPEED);
	}

	if (mReceivedTimeAttackFinished)
	{
		int hundreds = 0;
		std::vector<int> otherTimes;
		if (getPlayerRecorder().onTimeAttackFinish(hundreds, otherTimes))
		{
			GameApp::instance().showTimeAttackResults(hundreds, otherTimes);
		}
	}
}

void Game::onUpdateControls()
{
	if (mSkippableCutsceneFrames > 0 || mButtonYPressedDuringSkippableCutscene)	// Last check makes sure we'll ignore the press until it gets released
	{
		// Block input to the game
		mButtonYPressedDuringSkippableCutscene = (ControlsIn::instance().getInputPad(0) & (int)ControlsIn::Button::Y);
		if (mButtonYPressedDuringSkippableCutscene)
		{
			ControlsIn::instance().injectInput(0, 0);
			ControlsIn::instance().injectInput(1, 0);
		}
	}
}

void Game::updateSpecialInput(float timeElapsed)
{
	// In time attack mode: Restart if holding Y button for a while
	bool isCharging = false;
	if (mMode == Mode::TIME_ATTACK)
	{
		if (Application::instance().getSimulation().getSpeed() > 0.0f)	// Not in pause
		{
			const InputManager::Control& buttonY = InputManager::instance().getController(0).Y;
			if (buttonY.isPressed())
			{
				if (buttonY.justPressed())
				{
					// Start charging
					isCharging = true;
					mTimeAttackRestartCharge = 0.0f;
				}
				else
				{
					// Keep charging if already start, otherwise ignore the button
					isCharging = mTimeAttackRestartCharging;
				}
			}
		}
	}

	if (isCharging)
	{
		if (ConfigurationImpl::instance().mInstantTimeAttackRestart)
		{
			// Restart now
			mTimeAttackRestartCharge = 0.0f;
			restartTimeAttack(false);
			isCharging = false;
		}
		else
		{
			mTimeAttackRestartCharge += timeElapsed / 0.5f;
			if (mTimeAttackRestartCharge >= 1.0f)
			{
				// Restart now
				mTimeAttackRestartCharge = 1.0f;
				restartTimeAttack(true);
				isCharging = false;
			}
		}
	}
	else if (mTimeAttackRestartCharge > 0.0f)
	{
		mTimeAttackRestartCharge = std::max(0.0f, mTimeAttackRestartCharge - timeElapsed / 0.2f);
	}
	mTimeAttackRestartCharging = isCharging;

	// Update white overlay
	Application::instance().getGameView().setWhiteOverlayAlpha(mTimeAttackRestartCharge);
}

void Game::enableGamePauseByApplication()
{
	if (GameApp::hasInstance() && Application::instance().getSimulation().getSpeed() > 0.0f)
	{
		GameApp::instance().onGamePaused(mAllowRestartInGamePause);
	}
}

bool Game::isDebugModeActive() const
{
	const bool settingActive = (getSetting(SharedDatabase::Setting::SETTING_DEBUG_MODE, true) != 0);

#ifdef ENDUSER
	if (settingActive)
	{
		// If Debug Mode got unlocked (and enabled in the options menu), it can be used everywhere -- except for Time Attack and Competition Mode
		return (mMode != Game::Mode::TIME_ATTACK && mMode != Game::Mode::COMPETITION);
	}
	else if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		// If Dev Mode is active without having Debug Mode unlocked, it's limited to Act Select and directly started games
		return (mMode == Game::Mode::UNDEFINED || mMode == Game::Mode::ACT_SELECT);
	}
	else
	{
		return false;
	}
#else
	if (EngineMain::getDelegate().useDeveloperFeatures() && Configuration::instance().mGameRecording != 2)
	{
		return true;
	}
	else
	{
		return settingActive;
	}
#endif
}

void Game::fillDebugVisualization(Bitmap& bitmap, int& mode)
{
	mode = mode % 2;
	static Bitmap textBitmap;

	const Vec2i cameraPosition = VideoOut::instance().getInterpolatedWorldSpaceOffset();
	const int32 cameraX = cameraPosition.x;
	const int32 cameraY = cameraPosition.y;
	const int32 cameraTileX = cameraX / 16;
	const int32 cameraTileY = cameraY / 16;

	const int32 minX = cameraTileX;
	const int32 maxX = cameraTileX + bitmap.mWidth / 16;
	const int32 minY = cameraTileY;
	const int32 maxY = cameraTileY + bitmap.mHeight / 16;

	const uint16 filterByCharacterPath = (3 << EmulatorInterface::instance().readMemory8(0xffffb046));

	for (int32 y = minY; y <= maxY; ++y)
	{
		for (int32 x = minX; x <= maxX; ++x)
		{
			const int32 screenX = x*16 - cameraX;
			const int32 screenY = y*16 - cameraY;

			// Access the right chunk
			const uint16 chunkX = (x / 8);
			const uint16 chunkY = (y / 8) & 0x1f;
			uint32 address = EmulatorInterface::instance().readMemory16(0xffff8008 + chunkY * 4) + chunkX;
			const uint16 chunkType = EmulatorInterface::instance().readMemory8(0xffff0000 + address);

			// Access tile info
			address = EmulatorInterface::instance().readMemory16(0x00f02a + chunkType * 2);
			address += ((x % 8) + (y % 8) * 8) * 2;
			const uint16 tile = EmulatorInterface::instance().readMemory16(0xffff0000 + address);
			const uint8 tileForm = EmulatorInterface::instance().readMemory8(EmulatorInterface::instance().readMemory32(0xfffff796) + (tile & 0x03ff) * 2);

			if ((tile & 0xf000) != 0 && tileForm != 0)
			{
				Color baseColor;
				uint8 angle;
				if (mode == 1)	// Angle is only relevant in mode 1
				{
					angle = EmulatorInterface::instance().readMemory8(0x096000 + tileForm);
					if (tile & 0x0400)	// Flip horizontally
						angle = 0x100 - angle;
					if (tile & 0x0800)	// Flip vertically
						angle = 0x80 - angle;

					baseColor.setHSL(Vec3f((float)angle / 256.0f, 1.0f, 0.5f));
					baseColor.a = 0.7f;
				}

				for (int32 iy = 0; iy < 16; ++iy)
				{
					const int32 py = (screenY + iy);
					if (py < 0 || py >= bitmap.mHeight)
						continue;

					for (int32 ix = 0; ix < 16; ++ix)
					{
						const int32 px = (screenX + ix);
						if (px < 0 || px >= bitmap.mWidth)
							continue;

						bool pixelVisible = false;

					#if 1
						// Vertical
						const uint16 offset = (tile & 0x0400) ? (15 - ix) : ix;
						int8 indent = (int8)EmulatorInterface::instance().readMemory8(0x096100 + (tileForm * 0x10) + offset);
						if (indent != 0)
						{
							if (tile & 0x0800)
								indent = -indent;

							if (indent > 0)
							{
								pixelVisible = ((signed)(15 - iy) < indent);
							}
							else
							{
								pixelVisible = ((signed)iy < -indent);
							}
						}
					#else
						// Horizontal
						const uint16 offset = (tile & 0x0800) ? (15 - iy) : iy;
						int8 indent = (int8)EmulatorInterface::instance().readMemory8(0x097100 + (tileForm * 0x10) + offset);
						if (indent != 0)
						{
							if (tile & 0x0400)
								indent = -indent;

							if (indent > 0)
							{
								pixelVisible = ((signed)(15 - ix) < indent);
							}
							else
							{
								pixelVisible = ((signed)ix < -indent);
							}
						}
					#endif

						uint32& dst = bitmap.mData[py * bitmap.mWidth + px];
						switch (mode)
						{
							case 0:
							{
								if (pixelVisible)
								{
									const bool isSolidAbove = (tile & 0x5000) != 0;		// Solid from above
									const bool isSolidOther = (tile & 0xa000) != 0;		// Solid from left/right/below
									const bool isPath1 = (tile & 0x3000) != 0;
									const bool isPath2 = (tile & 0xc000) != 0;

									dst = ((tile & filterByCharacterPath) != 0) ? 0xc0000000 : 0x50000000;

									if (!isPath1 && (ix + iy) & 0x04)	dst = 0x40000000;
									if (!isPath2 && (ix - iy) & 0x04)	dst = 0x40000000;
									if (isSolidAbove)	dst += 0xff00ff;
									if (isSolidOther)	dst += 0x00ff00;
								}
								break;
							}

							case 1:
							{
								dst = baseColor.getABGR32();
								if (pixelVisible)
								{
									if (ix == 0x0f || iy == 0x0f)
										dst -= (dst & 0xf8f8f8) >> 3;
								}
								else
								{
									dst = ((dst & 0xfc000000) >> 2) | (dst & 0xffffff);
								}
								break;
							}
						}
					}
				}

				if (mode == 1)
				{
					Vec2i dummy;
					global::mFont3Pure.printBitmap(textBitmap, dummy, Recti(0, 0, 16, 16), rmx::hexString(angle, 2, ""));

					Blitter::Options blitterOptions;
					blitterOptions.mUseAlphaBlending = true;
					BitmapWrapper destBitmap(bitmap);
					BitmapWrapper sourceBitmap(textBitmap);
					Blitter::blitBitmap(destBitmap, Vec2i(screenX+4, screenY+5), sourceBitmap, Recti(0, 0, textBitmap.getWidth(), textBitmap.getHeight()), blitterOptions);
				}
			}
		}
	}
}

void Game::onGameRecordingHeaderLoaded(const std::string& buildString, const std::vector<uint8>& buffer)
{
	const std::unordered_map<uint32, SharedDatabase::Setting>& settings = SharedDatabase::getSettings();
	VectorBinarySerializer serializer(true, buffer);
	const size_t numSettings = serializer.read<uint32>();
	for (size_t i = 0; i < numSettings; ++i)
	{
		const uint32 settingId = serializer.read<uint32>();
		const uint32 value = serializer.read<uint32>();
		const auto it = settings.find(settingId);
		if (it != settings.end())
		{
			it->second.mValue = value;
		}
	}
}

void Game::onGameRecordingHeaderSave(std::vector<uint8>& buffer)
{
	std::vector<const SharedDatabase::Setting*> relevantSettings;
	{
		relevantSettings.reserve(32);
		const std::unordered_map<uint32, SharedDatabase::Setting>& settings = SharedDatabase::getSettings();
		for (const auto& pair : settings)
		{
			const SharedDatabase::Setting& setting = pair.second;
			if (!setting.mPurelyVisual)
				relevantSettings.push_back(&setting);
		}
	}

	VectorBinarySerializer serializer(false, buffer);
	serializer.writeAs<uint32>(relevantSettings.size());
	for (const SharedDatabase::Setting* setting : relevantSettings)
	{
		serializer.writeAs<uint32>(setting->mSettingId);
		serializer.write(setting->mValue);
	}
}

void Game::startIntoGameInternal()
{
	// Setup defaults -- these should not get used outside of Time Attack anyway
	mLastZoneAndAct = 0;
	mLastCharacters = 0;
	mReceivedTimeAttackFinished = false;
	mSkippableCutsceneFrames = 0;

	Simulation& simulation = Application::instance().getSimulation();
	simulation.setRunning(true);
	simulation.setSpeed(simulation.getDefaultSpeed());

	// Enforce fixed simulation frequency in Time Attack
	if (mMode == Mode::TIME_ATTACK)
		simulation.setSimulationFrequencyOverride(60.0f);
	else
		simulation.disableSimulationFrequencyOverride();

	setSetting(SharedDatabase::Setting::SETTING_KNUCKLES_AND_TAILS, false);		// Not queried by scripts at all, but it can't hurt to set it to false nevertheless

	SharedDatabase::resetAchievementValues();

	AudioOut::instance().moveMenuMusicToIngame();	// Needed only for the data select music to continue from main menu to data select
	AudioOut::instance().resumeSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_MUSIC);
	AudioOut::instance().resumeSoundContext(AudioOut::CONTEXT_INGAME + AudioOut::CONTEXT_SOUND);

	mPlayerRecorder.reset();
	GameApp::instance().enableStillImageBlur(false);

	mTimeoutUntilDiscordRefresh = 0.0f;
}

uint32 Game::useSetting(uint32 settingId)
{
	return getSetting(settingId, false);
}

int32 Game::getAchievementValue(uint32 achievementId)
{
	SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
	return (nullptr == achievement) ? 0 : achievement->mValue;
}

void Game::setAchievementValue(uint32 achievementId, int32 value)
{
	SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
	if (nullptr != achievement)
	{
		achievement->mValue = value;
	}
}

bool Game::isAchievementComplete(uint32 achievementId)
{
	return (mPlayerProgress.getAchievementState(achievementId) != 0);
}

void Game::setAchievementComplete(uint32 achievementId)
{
#ifdef ENDUSER
	// Can't affect achievements in debug mode
	if (isDebugModeActive() || EngineMain::getDelegate().useDeveloperFeatures())
		return;
#endif

	if (mPlayerProgress.getAchievementState(achievementId) == 0)
	{
		mPlayerProgress.mAchievementStates[achievementId] = 1;
		SharedDatabase::Achievement* achievement = SharedDatabase::getAchievement(achievementId);
		if (nullptr != achievement)
		{
			GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::ACHIEVEMENT, "Achievement complete", achievement->mName);
		}
		else
		{
			RMX_ERROR("Achievement not found", );
		}

		checkForUnlockedSecrets();
		mPlayerProgress.save();
	}
}

bool Game::isSecretUnlocked(uint32 secretId)
{
	return mPlayerProgress.isSecretUnlocked(secretId);
}

void Game::setSecretUnlocked(uint32 secretId)
{
	SharedDatabase::Secret* secret = SharedDatabase::getSecret(secretId);
	RMX_CHECK(nullptr != secret, "Secret with ID " << secretId << " not found", return);

	if (!mPlayerProgress.isSecretUnlocked(secretId))
	{
		mPlayerProgress.setSecretUnlocked(secretId);
		GameApp::instance().showUnlockedWindow(SecretUnlockedWindow::EntryType::SECRET, "Found hidden secret!", secret->mName);
		mPlayerProgress.save();
	}
}

void Game::triggerRestart()
{
	mRestartTriggered = true;
}

void Game::onGamePause(uint8 canRestart)
{
	GameApp::instance().showSkippableCutsceneWindow(false);
	GameApp::instance().onGamePaused(canRestart != 0);
}

void Game::allowRestartInGamePause(uint8 canRestart)
{
	mAllowRestartInGamePause = (canRestart != 0);
}

void Game::onLevelStart()
{
	mRestartingTimeAttack = false;
}

void Game::onZoneActCompleted(uint16 zoneAndAct)
{
	const uint8 bitNumber = (zoneAndAct >> 7) + (zoneAndAct & 1);
	const uint32 bitValue = (1 << bitNumber);
	const uint8 character = clamp(mLastCharacters, 1, 3) - 1;

	mPlayerProgress.mFinishedZoneAct |= bitValue;
	mPlayerProgress.mFinishedZoneActByCharacter[character] |= bitValue;
	mPlayerProgress.save();
}

uint16 Game::onTriggerNextZone(uint16 zoneAndAct)
{
	return zoneAndAct;
}

uint16 Game::onFadedOutLoadingZone(uint16 zoneAndAct)
{
	return zoneAndAct;
}

bool Game::onCharacterDied(uint8 playerIndex)
{
	// Death in time attack means level restart
	if (mMode == Mode::TIME_ATTACK)
	{
		if (mReceivedTimeAttackFinished)
		{
			// No death allowed!
			return false;
		}

		Application::instance().getSimulation().getCodeExec().getLemonScriptRuntime().callFunctionByName("restartTimeAttack");
	}
	return true;
}

void Game::onScreenFadedOutBeforeDataSelect()
{
	if (mMode == Mode::TITLE_SCREEN)
	{
		mReturnToMenuTriggered = true;
	}
}

void Game::returnToMainMenu()
{
	mReturnToMenuTriggered = true;

	// Do not restart data select music
	if (AudioOut::instance().isPlayingSfxId(0x2f))
	{
		AudioOut::instance().moveIngameMusicToMenu();
	}
	else
	{
		// Do a quick fade-out of the music (especially for transition from Blue Sphere mode and Competition Mode)
		AudioOut::instance().fadeOutChannel(0, 0.15f);
	}

	mTimeoutUntilDiscordRefresh = 0.0f;
}

bool Game::onTimeAttackFinish()
{
	if (mMode != Mode::TIME_ATTACK || mReceivedTimeAttackFinished)
		return false;

	mReceivedTimeAttackFinished = true;
	return true;
}

void Game::changePlanePatternRectAtex(uint16 px, uint16 py, uint16 width, uint16 height, uint8 planeIndex, uint8 atex)
{
	s3air::changePlanePatternRectAtex(EmulatorInterface::instance(), px, py, width, height, planeIndex, atex);
}

void Game::renderBlueSpheresGround(uint16 px, uint16 py, uint8 rotation, uint16 fieldColorA, uint16 fieldColorB)
{
	static const uint64 keys[2] = { rmx::getMurmur2_64(String("$generated_bluespheres_ground_opaque")),
									rmx::getMurmur2_64(String("$generated_bluespheres_ground_alpha")) };
	SpriteCache::CacheItem* items[2];

	Bitmap* bitmaps[2];
	for (int k = 0; k < 2; ++k)
	{
		SpriteCache::CacheItem& item = SpriteCache::instance().getOrCreateComponentSprite(keys[k]);
		++item.mChangeCounter;
		bitmaps[k] = &static_cast<ComponentSprite*>(item.mSprite)->accessBitmap();
		items[k] = &item;
	}

	mBlueSpheresRendering.renderToBitmap(*bitmaps[0], *bitmaps[1], VideoOut::instance().getScreenWidth(), px, py, rotation, fieldColorA, fieldColorB);

	items[0]->mSprite->mOffset.y = VideoOut::instance().getScreenHeight() - bitmaps[0]->mHeight;
	items[1]->mSprite->mOffset.y = items[0]->mSprite->mOffset.y - bitmaps[1]->mHeight;
}

uint64 Game::getBlueSpheresGroundSprite(uint8 part)
{
	static const uint64 keys[2] = { rmx::getMurmur2_64(String("$generated_bluespheres_ground_opaque")),
									rmx::getMurmur2_64(String("$generated_bluespheres_ground_alpha")) };
	return keys[(part == 0) ? 0 : 1];
}

void Game::writeBlueSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation)
{
	mBlueSpheresRendering.writeVisibleSpheresData(targetAddress, sourceAddress, px, py, rotation, EmulatorInterface::instance());
}

void Game::startSkippableCutscene()
{
	mSkippableCutsceneFrames = 5 * 60 * 60;		// Limit cutscene length to five minutes
}

void Game::endSkippableCutscene()
{
	mSkippableCutsceneFrames = 0;
	mButtonYPressedDuringSkippableCutscene = false;

	// Back to normal
	Simulation& simulation = Application::instance().getSimulation();
	if (simulation.getSpeed() == CUTSCENE_SKIPPING_SPEED)
		simulation.setSpeed(simulation.getDefaultSpeed());

	GameApp::instance().showSkippableCutsceneWindow(false);
}
