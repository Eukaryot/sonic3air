/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/ActSelectMenu.h"
#include "sonic3air/menu/ExtrasMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/GameMenuManager.h"
#include "sonic3air/menu/MainMenu.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/menu/TimeAttackMenu.h"
#include "sonic3air/menu/mods/ModsMenu.h"
#include "sonic3air/menu/options/OptionsMenu.h"
#include "sonic3air/Game.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"


namespace detail
{
	template<typename T>
	void createGameMenuInstance(T*& gameMenu, std::vector<GameMenuBase*>& allGameMenus, MenuBackground& menuBackground)
	{
		gameMenu = new T(menuBackground);
		allGameMenus.push_back(gameMenu);
	}

	void drawQuad(Drawer& drawer, float splitX1, float splitX2, DrawerTexture& texture)
	{
		DrawerMeshVertex quad[4];
		quad[0].mPosition.set(splitX1 + 15.0f, 0.0f);
		quad[1].mPosition.set(splitX2 + 15.0f, 0.0f);
		quad[2].mPosition.set(splitX1 - 15.0f, 224.0f);
		quad[3].mPosition.set(splitX2 - 15.0f, 224.0f);

		for (int i = 0; i < 4; ++i)
		{
			quad[i].mTexcoords.x = (quad[i].mPosition.x + 8.0f) / 416.0f;
			quad[i].mTexcoords.y = (quad[i].mPosition.y) / 224.0f;
		}
		drawer.drawQuad(quad, texture);
	}

	void drawSeparator(Drawer& drawer, float splitX, float animationOffset, bool mirrored)
	{
		DrawerMeshVertex quad[4];
		if (mirrored)
		{
			quad[0].mPosition.set(splitX + 10.0f, -3.0f);
			quad[1].mPosition.set(splitX + 25.0f,  0.0f);
			quad[2].mPosition.set(splitX - 20.0f, 224.0f);
			quad[3].mPosition.set(splitX -  5.0f, 227.0f);

			quad[0].mTexcoords.set(1.0f, animationOffset);
			quad[1].mTexcoords.set(0.0f, animationOffset);
			quad[2].mTexcoords.set(1.0f, animationOffset + 16.0f);
			quad[3].mTexcoords.set(0.0f, animationOffset + 16.0f);
		}
		else
		{
			quad[0].mPosition.set(splitX +  5.0f, -3.0f);
			quad[1].mPosition.set(splitX + 20.0f,  0.0f);
			quad[2].mPosition.set(splitX - 25.0f, 224.0f);
			quad[3].mPosition.set(splitX - 10.0f, 227.0f);

			quad[0].mTexcoords.set(0.0f, animationOffset);
			quad[1].mTexcoords.set(1.0f, animationOffset);
			quad[2].mTexcoords.set(0.0f, animationOffset + 16.0f);
			quad[3].mTexcoords.set(1.0f, animationOffset + 16.0f);
		}
		drawer.drawQuad(quad, global::mMainMenuBackgroundSeparator);
	}
}


MenuBackground::MenuBackground()
{
	detail::createGameMenuInstance(mMainMenu,		mAllChildren, *this);
	detail::createGameMenuInstance(mActSelectMenu,	mAllChildren, *this);
	detail::createGameMenuInstance(mTimeAttackMenu,	mAllChildren, *this);
	detail::createGameMenuInstance(mOptionsMenu,	mAllChildren, *this);
	detail::createGameMenuInstance(mExtrasMenu,		mAllChildren, *this);
	detail::createGameMenuInstance(mModsMenu,		mAllChildren, *this);
}

MenuBackground::~MenuBackground()
{
	for (GameMenuBase* child : mAllChildren)
	{
		delete child;
	}
}

void MenuBackground::initialize()
{
	mLightLayer.setPosition(1.0f);
	mBlueLayer.setPosition(1.0f);
	mAlterLayer.setPosition(0.0f);
	mBackgroundLayer.setPosition(0.0f);

	mTarget = Target::TITLE;
	startTransition(Target::SPLIT);

	mAnimationTimer = 0.0f;
	setPreviewZoneAndAct(0, 0, true);

	// Do not automatically open a menu here, that always needs to be done separately via a call like "openMainMenu"
}

void MenuBackground::deinitialize()
{
	for (GameMenuBase* child : mAllChildren)
	{
		removeChild(child);
	}
}

void MenuBackground::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);

	mAnimationTimer += timeElapsed;
	if (mAnimationTimer >= 60.0f)
		mAnimationTimer -= 60.0f;

	updatePreview(timeElapsed);
	updateTransition(timeElapsed);
}

void MenuBackground::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	for (GameMenuBase* child : mAllChildren)
	{
		child->setRect(mRect);
	}

	// Calculate split positions between visible parts of layers
	const float normalizedSplitLight = std::max(mLightLayer.mCurrentPosition, mAlterLayer.mCurrentPosition);
	const float normalizedSplitBlue  = std::max(mBlueLayer.mCurrentPosition, mAlterLayer.mCurrentPosition);
	const float normalizedSplitAlter = mAlterLayer.mCurrentPosition;
	const float normalizedTitleLeft  = normalizedSplitAlter;
	const float normalizedTitleRight = std::min(normalizedSplitLight, normalizedSplitBlue);

	const float splitMin = -30.0f;
	const float splitMax = 430.0f;
	const float splitLight = interpolate(splitMin, splitMax, saturate(normalizedSplitLight));
	const float splitBlue  = interpolate(splitMin, splitMax, saturate(normalizedSplitBlue));
	const float splitAlter = interpolate(splitMin, splitMax, saturate(normalizedSplitAlter));
	const float titleLeft  = interpolate(splitMin, splitMax, saturate(normalizedTitleLeft));
	const float titleRight = interpolate(splitMin, splitMax, saturate(normalizedTitleRight));

	// Layers
	{
		if (titleLeft < titleRight)
		{
			if (!Game::instance().isInMainMenuMode())
			{
				Game::instance().startIntoMainMenuBG();
			}
			else if (!mAnimatedBackgroundActive)
			{
				Application::instance().getSimulation().setRunning(true);
			}

			LemonScriptRuntime& runtime = Application::instance().getSimulation().getCodeExec().getLemonScriptRuntime();
			static const lemon::FlyweightString SCROLL_OFFSET_NAME("MainMenuBG.scrollOffset");
			static const lemon::FlyweightString LOGO_POSITION_NAME("MainMenuBG.logoPosition");
			runtime.setGlobalVariableValue_int64(SCROLL_OFFSET_NAME, roundToInt(-mBackgroundLayer.mCurrentPosition * 150.0f));
			runtime.setGlobalVariableValue_int64(LOGO_POSITION_NAME, roundToInt(interpolate(splitMin, splitMax, normalizedTitleRight) - 91.0f));
			mAnimatedBackgroundActive = true;
		}
		else
		{
			if (mAnimatedBackgroundActive)
			{
				if (Game::instance().isInMainMenuMode())
				{
					Application::instance().getSimulation().setRunning(false);
				}
				mAnimatedBackgroundActive = false;
			}
		}

		if (splitLight < splitBlue)
		{
			detail::drawQuad(drawer, splitLight, splitBlue, global::mDataSelectBackground);
		}

		if (splitBlue < splitMax)
		{
			detail::drawQuad(drawer, splitBlue, splitMax, global::mLevelSelectBackground);
		}

		if (splitAlter > splitMin)
		{
			detail::drawQuad(drawer, splitMin, splitAlter, global::mDataSelectAltBackground);
		}
	}

	// Separators
	drawer.setSamplingMode(SamplingMode::BILINEAR);
	drawer.setWrapMode(TextureWrapMode::REPEAT);
	{
		const float separatorAnimationOffset = -mAnimationTimer * 3.0f;

		if (splitLight > splitMin && splitLight < splitMax)
		{
			detail::drawSeparator(drawer, splitLight, separatorAnimationOffset, false);
		}

		if (splitBlue > splitMin && splitBlue < splitMax)
		{
			detail::drawSeparator(drawer, splitBlue, separatorAnimationOffset, true);
		}

		if (splitAlter > splitMin && splitAlter < splitMax)
		{
			detail::drawSeparator(drawer, splitAlter, separatorAnimationOffset, true);
		}
	}
	drawer.setSamplingMode(SamplingMode::POINT);
	drawer.setWrapMode(TextureWrapMode::CLAMP);

	if (mPreviewVisibility > 0.0f)
	{
		for (int i = 0; i < 2; ++i)
		{
			const PreviewImage& img = mPreviewImage[i];
			if (img.mSpriteKey == 0)
				continue;

			const int maxOffset = std::min(480 - (int)mRect.width, 80);
			const int px = roundToInt(-img.mOffset * maxOffset);
			const int visibleWidth = roundToInt(mRect.width * img.mVisibility);

			drawer.pushScissor(Recti((int)mRect.width - visibleWidth, 0, visibleWidth, (int)mRect.height));
			drawer.drawSprite(Vec2i(px, 18), img.mSpriteKey, Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
			drawer.popScissor();
		}

		drawer.drawSprite(Vec2i(0, 8), rmx::getMurmur2_64("level_preview_border_left"), Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
		drawer.drawSpriteRect(Recti(10, 8, (int)mRect.width - 20, 100), rmx::getMurmur2_64("level_preview_border_center"), Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
		drawer.drawSprite(Vec2i((int)mRect.width, 8), rmx::getMurmur2_64("level_preview_border_right"), Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
	}

	GuiBase::render();

	drawer.performRendering();
}

void MenuBackground::startTransition(Target target)
{
	if (target == mTarget)
		return;

	// Set defaults, to be overwritten below
	mLightLayer.mTargetPosition = 1.0f;
	mBlueLayer.mTargetPosition = 1.5f;		// Far to the right
	mAlterLayer.mTargetPosition = -1.0f;	// Far to the left
	mBackgroundLayer.mTargetPosition = 0.0f;
	mLightLayer.mDelay = 0.0f;
	mBlueLayer.mDelay = 0.0f;
	mAlterLayer.mDelay = 0.0f;
	mBackgroundLayer.mDelay = 0.0f;

	switch (target)
	{
		case Target::TITLE:
		{
			// Only used when exiting the application
			mLightLayer.mDelay = 0.1f;
			break;
		}

		case Target::SPLIT:
		{
			mLightLayer.mTargetPosition = 0.465f;
			break;
		}

		case Target::LIGHT:
		{
			mLightLayer.mTargetPosition = 0.0f;
			mBackgroundLayer.mTargetPosition = -0.5f;
			break;
		}

		case Target::BLUE:
		{
			mBlueLayer.mTargetPosition = 0.0f;
			mLightLayer.mTargetPosition = -0.5f;	// Far to the left
			mBackgroundLayer.mTargetPosition = -0.5f;
			break;
		}

		case Target::ALTER:
		{
			mLightLayer.mTargetPosition = 1.5f;
			mLightLayer.mDelay = 0.1f;
			mAlterLayer.mTargetPosition = 1.0f;
			mBackgroundLayer.mTargetPosition = 1.5f;
			mBackgroundLayer.mDelay = 0.1f;
			break;
		}

		default:
			break;
	}

	mTarget = target;
	mInTransition = true;
}

void MenuBackground::setPreviewZoneAndAct(uint8 zone, uint8 act, bool forceReset)
{
	if ((mPreviewKey.mZone == zone && mPreviewKey.mAct == act) && !forceReset)
		return;

	mPreviewKey.mZone = zone;
	mPreviewKey.mAct = act;
	mPreviewKey.mImage = 0;

	mPreviewImage[0].mSpriteKey = global::mZoneActPreviewSpriteKeys[mPreviewKey];
	mPreviewImage[0].mSubIndex = 0;
	mPreviewImage[0].mOffset = 0.5f;
	mPreviewImage[0].mVisibility = 1.0f;
	mPreviewImage[1].mSpriteKey = 0;

	mCurrentTime = 0.0f;
	updatePreview(0.0f);
}

void MenuBackground::showPreview(bool show, bool useTransition)
{
	if (useTransition)
	{
		mPreviewVisibilityChange = show ? 10.0f : -10.0f;
	}
	else
	{
		mPreviewVisibility = show ? 1.0f : 0.0f;
		mPreviewVisibilityChange = 0.0f;
	}
}

void MenuBackground::openMainMenu()
{
	openMenu(*mMainMenu);
}

void MenuBackground::openActSelectMenu()
{
	openMenu(*mActSelectMenu);
}

void MenuBackground::openTimeAttackMenu()
{
	openMenu(*mTimeAttackMenu);
}

void MenuBackground::openOptions(bool enteredInGame)
{
	openMenu(*mOptionsMenu);

	if (enteredInGame)
	{
		skipTransition();
		if (nullptr == mGameStartedMenu)
			mGameStartedMenu = mMainMenu;
		mGameStartedMenu->setBaseState(GameMenuBase::BaseState::INACTIVE);
	}

	mOptionsMenu->setupOptionsMenu(enteredInGame);
	showPreview(false, !enteredInGame);
}

void MenuBackground::openExtras()
{
	openMenu(*mExtrasMenu);
}

void MenuBackground::openMods()
{
	openMenu(*mModsMenu);
}

void MenuBackground::openGameStartedMenu()
{
	// Open either the menu that started the last in-game session (should be either the Main Menu, Act Select, Time Attack, or Extras)
	if (nullptr != mGameStartedMenu && mGameStartedMenu != mMainMenu)
	{
		openMenu(*mGameStartedMenu);
		skipTransition();
	}
	else
	{
		openMenu(*mMainMenu);
	}
}

void MenuBackground::fadeToExit()
{
	startTransition(MenuBackground::Target::TITLE);
}

void MenuBackground::setGameStartedMenu()
{
	mGameStartedMenu = mLastOpenedMenu;
}

void MenuBackground::openMenu(GameMenuBase& menu)
{
	// The menus only really work in a fixed resolution, so make sure that one is set
	VideoOut::instance().setScreenSize(400, 224);

	GameApp::instance().getGameMenuManager().addMenu(menu);

	if (&menu == mOptionsMenu || &menu == mExtrasMenu || &menu == mModsMenu)
	{
		showPreview(false, false);
	}

	mLastOpenedMenu = &menu;
}

void MenuBackground::skipTransition()
{
	// Skip the transition entirely
	if (mInTransition)
	{
		Layer* layers[4] = { &mLightLayer, &mBlueLayer, &mAlterLayer, &mBackgroundLayer };
		for (Layer* layer : layers)
		{
			layer->mDelay = 0.0f;
			layer->mCurrentPosition = layer->mTargetPosition;
		}
		mInTransition = false;
	}
}

void MenuBackground::updateTransition(float timeElapsed)
{
	if (mInTransition)
	{
		mInTransition = false;

		// Update layer movement
		Layer* layers[4] = { &mLightLayer, &mBlueLayer, &mAlterLayer, &mBackgroundLayer };
		for (Layer* layer : layers)
		{
			if (layer->mDelay > 0.0f)
			{
				layer->mDelay = std::max(layer->mDelay - timeElapsed, 0.0f);
				mInTransition = true;
			}
			else if (layer->mCurrentPosition != layer->mTargetPosition)
			{
				if (layer->mCurrentPosition < layer->mTargetPosition)
				{
					layer->mCurrentPosition = std::min(layer->mCurrentPosition + timeElapsed * layer->mMoveSpeed, layer->mTargetPosition);
				}
				else
				{
					layer->mCurrentPosition = std::max(layer->mCurrentPosition - timeElapsed * layer->mMoveSpeed, layer->mTargetPosition);
				}
				mInTransition = true;
			}
		}
	}
}

void MenuBackground::updatePreview(float timeElapsed)
{
	if (timeElapsed > 0.0f && mPreviewVisibilityChange != 0.0f)
	{
		mPreviewVisibility += mPreviewVisibilityChange * timeElapsed;
		if (mPreviewVisibility <= 0.0f)
		{
			mPreviewVisibility = 0.0f;
			mPreviewVisibilityChange = 0.0f;
			setPreviewZoneAndAct(mPreviewKey.mZone, mPreviewKey.mAct, true);
		}
		else if (mPreviewVisibility >= 1.0f)
		{
			mPreviewVisibility = 1.0f;
			mPreviewVisibilityChange = 0.0f;
		}
	}

	if (mPreviewVisibility > 0.0f)
	{
		constexpr float MOVE_TIME = 2.5f;
		constexpr float CHANGE_TIME = 0.6f;
		constexpr float TOTAL_TIME = MOVE_TIME + CHANGE_TIME;

		mCurrentTime += timeElapsed;
		if (mCurrentTime >= TOTAL_TIME)
		{
			// Time to restart
			mPreviewImage[0] = mPreviewImage[1];
			mPreviewImage[0].mOffset = 0.5f;
			mPreviewImage[0].mVisibility = 1.0f;
			mPreviewImage[1].mSpriteKey = 0;
			mCurrentTime -= TOTAL_TIME;
		}

		if (mCurrentTime >= MOVE_TIME)
		{
			// Transition animation
			if (mPreviewImage[1].mSpriteKey == 0)
			{
				mPreviewKey.mImage = (mPreviewImage[0].mSubIndex + 1) % 2;
				mPreviewImage[1].mSpriteKey = global::mZoneActPreviewSpriteKeys[mPreviewKey];
				mPreviewImage[1].mSubIndex = mPreviewKey.mImage;
			}

			const float animtime = (mCurrentTime - MOVE_TIME) / CHANGE_TIME;
			const float offset = (1.0f - std::cos(animtime * PI_FLOAT)) * 0.25f;
			mPreviewImage[0].mOffset = 0.5f + offset;

			mPreviewImage[1].mOffset = offset;
			mPreviewImage[1].mVisibility = animtime;
		}
	}
}
