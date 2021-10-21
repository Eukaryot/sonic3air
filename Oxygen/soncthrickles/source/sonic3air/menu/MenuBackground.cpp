/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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
#include "sonic3air/menu/ModsMenu.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/menu/TimeAttackMenu.h"
#include "sonic3air/menu/options/OptionsMenu.h"

#include "oxygen/application/EngineMain.h"
#include "oxygen/application/mainview/GameView.h"


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

	mTarget = Target::TITLE;
	startTransition(Target::SPLIT);

	mAnimationTimer = 0.0f;
	setPreviewZoneAndAct(0, 0, true);

	GameMenuManager& manager = GameApp::instance().getGameMenuManager();
	if (nullptr == mLastOpenedMenu)
	{
		manager.addMenu(*mMainMenu);
	}
	else
	{
		manager.addMenu(*mLastOpenedMenu);
	}
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

	// Transitioning?
	if (mInTransition)
	{
		mInTransition = false;

		// Update layer movement
		Layer* layers[3] = { &mLightLayer, &mBlueLayer, &mAlterLayer };
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
			detail::drawQuad(drawer, titleLeft, titleRight, global::mMainMenuBackgroundLeft);
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
	drawer.setSamplingMode(DrawerSamplingMode::BILINEAR);
	drawer.setWrapMode(DrawerWrapMode::REPEAT);
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
	drawer.setSamplingMode(DrawerSamplingMode::POINT);
	drawer.setWrapMode(DrawerWrapMode::CLAMP);

	if (mPreviewVisibility > 0.0f)
	{
		for (int i = 0; i < 2; ++i)
		{
			const PreviewImage& img = mPreviewImage[i];
			if (nullptr == img.mTexture)
				continue;

			Recti previewRect = mRect;
			previewRect.x = roundToInt(480.0f * (1.0f - img.mVisibility) - img.mOffset * 80.0f);
			previewRect.y = 18;
			previewRect.width = roundToInt(480.0f * img.mVisibility);
			previewRect.height = 80;
			drawer.drawRect(previewRect, *img.mTexture, Vec2f(1.0f - img.mVisibility, 0.0f), Vec2f(1.0f, 1.0f), Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
		}

		Recti rect = mRect;
		rect.width = global::mPreviewBorder.getWidth();
		rect.height = global::mPreviewBorder.getHeight();
		rect.x = (roundToInt(mRect.width) - rect.width) / 2;
		rect.y = 8;
		drawer.drawRect(rect, global::mPreviewBorder, Color(1.0f, 1.0f, 1.0f, mPreviewVisibility));
	}

	GuiBase::render();

	if (titleLeft < titleRight)
	{
		const float alpha = std::min((titleRight - titleLeft) / (splitMax - splitMin) * 2.2f, 1.0f);

		Recti rect = mRect;
		rect.width = global::mGameLogo.getWidth();
		rect.height = global::mGameLogo.getHeight();
		rect.x = roundToInt(interpolate(splitMin, splitMax, normalizedTitleRight) - 91.0f) - rect.width / 2;
		rect.y = -1;
		drawer.drawRect(rect, global::mGameLogo, Color(1.0f, 1.0f, 1.0f, alpha));
	}

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
	mLightLayer.mDelay = 0.0f;
	mBlueLayer.mDelay = 0.0f;
	mAlterLayer.mDelay = 0.0f;

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
			break;
		}

		case Target::BLUE:
		{
			mBlueLayer.mTargetPosition = 0.0f;
			mLightLayer.mTargetPosition = -0.5f;	// Far to the left
			break;
		}

		case Target::ALTER:
		{
			mLightLayer.mTargetPosition = 1.5f;
			mLightLayer.mDelay = 0.1f;
			mAlterLayer.mTargetPosition = 1.0f;
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

	mPreviewImage[0].mTexture = &global::mZoneActPreview[mPreviewKey];
	mPreviewImage[0].mSubIndex = 0;
	mPreviewImage[0].mOffset = 0.5f;
	mPreviewImage[0].mVisibility = 1.0f;
	mPreviewImage[1].mTexture = nullptr;

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

void MenuBackground::openOptions(bool noAnimation)
{
	openMenu(*mOptionsMenu);

	if (noAnimation)
		mOptionsMenu->onEnteredFromIngame();
	showPreview(false, !noAnimation);
}

void MenuBackground::openExtras()
{
	openMenu(*mExtrasMenu);
}

void MenuBackground::openMods()
{
	openMenu(*mModsMenu);
}

void MenuBackground::fadeToExit()
{
	startTransition(MenuBackground::Target::TITLE);
}

void MenuBackground::openMenu(GameMenuBase& menu)
{
	GameApp::instance().getGameMenuManager().addMenu(menu);

	if (&menu == mOptionsMenu || &menu == mExtrasMenu || &menu == mModsMenu)
	{
		showPreview(false, false);
	}

	mLastOpenedMenu = &menu;
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
			mPreviewImage[1].mTexture = nullptr;
			mCurrentTime -= TOTAL_TIME;
		}

		if (mCurrentTime >= MOVE_TIME)
		{
			// Transition animation
			if (nullptr == mPreviewImage[1].mTexture)
			{
				mPreviewKey.mImage = (mPreviewImage[0].mSubIndex + 1) % 2;
				mPreviewImage[1].mTexture = &global::mZoneActPreview[mPreviewKey];

				if (mPreviewImage[1].mTexture->getWidth() == 0)
				{
					// Error handling
					mPreviewKey.mImage = 0;
					mPreviewImage[1].mTexture = &global::mZoneActPreview[mPreviewKey];
				}

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
