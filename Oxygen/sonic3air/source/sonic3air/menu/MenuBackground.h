/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "sonic3air/menu/SharedResources.h"

class GameMenuBase;
class MainMenu;
class ActSelectMenu;
class TimeAttackMenu;
class OptionsMenu;
class ExtrasMenu;
class ModsMenu;


class MenuBackground : public GuiBase
{
public:
	enum class Target
	{
		TITLE,		// Angel Island background
		SPLIT,		// Split view with Angel Island on the left, and light background on the right
		LIGHT,		// Light background (from Data Select)
		BLUE,		// Blue background (from Level Select)
		ALTER		// Alternative background (from Competition Mode)
	};

public:
	MenuBackground();
	virtual ~MenuBackground();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void startTransition(Target target);

	void setPreviewZoneAndAct(uint8 zone, uint8 act, bool forceReset = false);
	void showPreview(bool show, bool useTransition = true);

	void openMainMenu();
	void openActSelectMenu();
	void openTimeAttackMenu();
	void openOptions(bool enteredInGame = false);
	void openExtras();
	void openMods();
	void openGameStartedMenu();
	void fadeToExit();

	void setGameStartedMenu();

private:
	void openMenu(GameMenuBase& menu);
	void skipTransition();
	void updateTransition(float timeElapsed);
	void updatePreview(float timeElapsed);

private:
	Target mTarget = Target::TITLE;
	bool mInTransition = false;

	// Children
	std::vector<GameMenuBase*> mAllChildren;
	MainMenu* mMainMenu = nullptr;
	ActSelectMenu* mActSelectMenu = nullptr;
	TimeAttackMenu* mTimeAttackMenu = nullptr;
	OptionsMenu* mOptionsMenu = nullptr;
	ExtrasMenu* mExtrasMenu = nullptr;
	ModsMenu* mModsMenu = nullptr;
	GameMenuBase* mLastOpenedMenu = nullptr;
	GameMenuBase* mGameStartedMenu = nullptr;

	// Background
	struct PreviewImage
	{
		DrawerTexture* mTexture = nullptr;
		int mSubIndex = 0;
		float mOffset = 0.0f;
		float mVisibility = 0.0f;
	};
	PreviewImage mPreviewImage[2];
	global::ZoneActPreviewKey mPreviewKey;

	float mCurrentTime = 0.0f;
	float mAnimationTimer = 0.0f;	// In seconds; loops back to zero after 1 minute
	bool mAnimatedBackgroundActive = false;

	float mPreviewVisibility = 0.0f;
	float mPreviewVisibilityChange = 0.0f;

	struct Layer
	{
		float mCurrentPosition = 0.0f;		// Normalized position: 0.0f and 1.0f are a bit outside the screen on the lefthand and righthand side, respectively
		float mTargetPosition = 0.0f;
		float mMoveSpeed = 3.75f;			// Change rate of normalized position per second
		float mDelay = 0.0f;

		inline void setPosition(float pos) { mCurrentPosition = mTargetPosition = pos; }
	};
	Layer mLightLayer;
	Layer mBlueLayer;
	Layer mAlterLayer;
	Layer mBackgroundLayer;
};
