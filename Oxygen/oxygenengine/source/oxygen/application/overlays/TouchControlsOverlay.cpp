/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/TouchControlsOverlay.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/utils/RenderUtils.h"
#include "oxygen/resources/SpriteCache.h"


namespace
{
	static const constexpr float CONFIG_TO_SETUP_SCALE = 0.01f;
	static const Rectf DONE_BUTTON_RECT(-0.24f, -0.6f, 0.48f, 0.2f);

	inline static const float square(float value)
	{
		return value * value;
	}
}


float TouchControlsOverlay::TouchArea::getWeight(const Vec2f& position) const
{
	const Vec2f closest = mRect.getClosestPoint(position);
	if (closest == position)
		return mPriority;

	const float sqrDistance = closest.sqrDist(position);
	if (sqrDistance >= mRadius * mRadius)
		return 0.0f;

	return (1.0f - (std::sqrt(sqrDistance) / mRadius)) * mPriority;
}


TouchControlsOverlay::TouchControlsOverlay()
{
}

TouchControlsOverlay::~TouchControlsOverlay()
{
}

void TouchControlsOverlay::buildTouchControls()
{
	registerAtInputManager(InputManager::instance());

	// Update conversion variables
	const Rectf letterBoxRect = RenderUtils::getLetterBoxRect(FTX::screenRect(), 16.0f / 9.0f);
	mScreenCenter.set((float)FTX::screenWidth() / 2.0f, (float)FTX::screenHeight() / 2.0f);
	mScreenScale.set(letterBoxRect.height / 2.0f, letterBoxRect.height / 2.0f);

	InputManager::ControllerScheme& controller = mInputManager->accessController(0);
	Configuration& config = Configuration::instance();

	mTouchAreas.clear();
	mVisualElements.clear();

	mSetup.mDirectionalPadCenter = Vec2f(-1.15f, 0.45f) + Vec2f(config.mVirtualGamepad.mDirectionalPadCenter) * CONFIG_TO_SETUP_SCALE * 0.5f;
	mSetup.mDirectionalPadSize = (float)config.mVirtualGamepad.mDirectionalPadSize * CONFIG_TO_SETUP_SCALE;
	mSetup.mFaceButtonsCenter = Vec2f(1.15f, 0.45f) + Vec2f(config.mVirtualGamepad.mFaceButtonsCenter) * CONFIG_TO_SETUP_SCALE * 0.5f;
	mSetup.mFaceButtonsSize = (float)config.mVirtualGamepad.mFaceButtonsSize * CONFIG_TO_SETUP_SCALE;
	mSetup.mStartButtonCenter = Vec2f(0.95f, -0.92f) + Vec2f(config.mVirtualGamepad.mStartButtonCenter) * CONFIG_TO_SETUP_SCALE * 0.5f;
	mSetup.mGameRecButtonCenter = Vec2f(-0.95f, -0.92f) + Vec2f(config.mVirtualGamepad.mGameRecButtonCenter) * CONFIG_TO_SETUP_SCALE * 0.5f;

	// Create touch areas and visual elements
	{
		const float size = mSetup.mDirectionalPadSize;
		buildRectangularButton(mSetup.mDirectionalPadCenter + Vec2f(-0.3f, 0.0f) * size, Vec2f(0.2f, 0.125f) * size, "touch_overlay_left",  &controller.Left,  ConfigMode::State::MOVING_DPAD);
		buildRectangularButton(mSetup.mDirectionalPadCenter + Vec2f(+0.3f, 0.0f) * size, Vec2f(0.2f, 0.125f) * size, "touch_overlay_right", &controller.Right, ConfigMode::State::MOVING_DPAD);
		buildRectangularButton(mSetup.mDirectionalPadCenter + Vec2f(0.0f, -0.3f) * size, Vec2f(0.125f, 0.2f) * size, "touch_overlay_up",    &controller.Up,	   ConfigMode::State::MOVING_DPAD);
		buildRectangularButton(mSetup.mDirectionalPadCenter + Vec2f(0.0f, +0.3f) * size, Vec2f(0.125f, 0.2f) * size, "touch_overlay_down",  &controller.Down,  ConfigMode::State::MOVING_DPAD);
	}
	{
		const float size = mSetup.mDirectionalPadSize;
		buildPointButton(mSetup.mDirectionalPadCenter + Vec2f(-0.2f, -0.2f) * size, 0.55f * size, 1.0f, controller.Left,  &controller.Up);
		buildPointButton(mSetup.mDirectionalPadCenter + Vec2f(+0.2f, -0.2f) * size, 0.55f * size, 1.0f, controller.Right, &controller.Up);
		buildPointButton(mSetup.mDirectionalPadCenter + Vec2f(-0.2f, +0.2f) * size, 0.55f * size, 1.0f, controller.Left,  &controller.Down);
		buildPointButton(mSetup.mDirectionalPadCenter + Vec2f(+0.2f, +0.2f) * size, 0.55f * size, 1.0f, controller.Right, &controller.Down);
	}
	{
		const float size = mSetup.mFaceButtonsSize;
		buildRoundButton(mSetup.mFaceButtonsCenter + Vec2f(-0.28f, 0.0f) * size, 0.15f * size, "touch_overlay_X", controller.X, ConfigMode::State::MOVING_BUTTONS);
		buildRoundButton(mSetup.mFaceButtonsCenter + Vec2f(+0.28f, 0.0f) * size, 0.15f * size, "touch_overlay_B", controller.B, ConfigMode::State::MOVING_BUTTONS);
		buildRoundButton(mSetup.mFaceButtonsCenter + Vec2f(0.0f, -0.28f) * size, 0.15f * size, "touch_overlay_Y", controller.Y, ConfigMode::State::MOVING_BUTTONS);
		buildRoundButton(mSetup.mFaceButtonsCenter + Vec2f(0.0f, +0.28f) * size, 0.15f * size, "touch_overlay_A", controller.A, ConfigMode::State::MOVING_BUTTONS);
	}
	buildRectangularButton(mSetup.mStartButtonCenter, Vec2f(0.18f, 0.06f), "touch_overlay_start", &controller.Start, ConfigMode::State::MOVING_START);
	buildRectangularButton(mSetup.mGameRecButtonCenter, Vec2f(0.15f, 0.06f), "touch_overlay_rec", nullptr, ConfigMode::State::MOVING_GAMEREC, TouchArea::SpecialType::GAMEREC);

	mLastScreenSize = FTX::screenSize();
}

void TouchControlsOverlay::setForceHidden(bool hidden)
{
	mForceHidden = hidden;
}

bool TouchControlsOverlay::isInConfigMode() const
{
	return mConfigMode.mEnabled;
}

void TouchControlsOverlay::enableConfigMode(bool enable)
{
	mConfigMode.mEnabled = enable;
	mConfigMode.mState = ConfigMode::State::TOUCH_DOWN;
}

void TouchControlsOverlay::initialize()
{
	FileHelper::loadTexture(mDoneText, L"data/images/overlay/done.png");
}

void TouchControlsOverlay::deinitialize()
{
}

void TouchControlsOverlay::update(float timeElapsed)
{
	if (mLastScreenSize != FTX::screenSize())
	{
		buildTouchControls();
	}

	mAutoHideTimer += timeElapsed;

	// Update visibility
	{
		const InputManager::InputType lastInputType = mInputManager->getLastInputType();
		const bool shouldBeVisible = !mForceHidden && (lastInputType == InputManager::InputType::TOUCH || lastInputType == InputManager::InputType::NONE) && (mAutoHideTimer < 4.0f);
		if (shouldBeVisible)
		{
			if (mVisibility < 1.0f)
			{
				mVisibility = saturate(mVisibility + timeElapsed / 0.2f);
			}
		}
		else
		{
			if (mVisibility > 0.0f)
			{
				mVisibility = saturate(mVisibility - timeElapsed / 0.2f);
			}
			mConfigMode.mEnabled = false;
		}
	}
}

void TouchControlsOverlay::render()
{
	GuiBase::render();

	if (mVisibility <= 0.0f)
		return;

	Drawer& drawer = EngineMain::instance().getDrawer();
	const float alpha = mVisibility * Configuration::instance().mVirtualGamepad.mOpacity;

	// Debug output: Show touch sensitive regions
#if 0
	{
		const Recti screen = FTX::screenRect();
		for (int iy = 0; iy < 100; ++iy)
		{
			for (int ix = 0; ix < 200; ++ix)
			{
				const Vec2f position(((float)ix + 0.5f) / 200.0f, ((float)iy + 0.5f) / 100.0f);
				const TouchArea* touchArea = getTouchAreaAtNormalizedPosition(position);
				if (nullptr != touchArea)
				{
					const size_t index = (size_t)(touchArea - &mTouchAreas[0]);
					Color color;
					float dummy;
					color.setHSL(Vec3f(std::modf((float)index * 0.3f, &dummy), 1.0f, 0.5f));
					color.a = 0.75f;
					drawer.drawRect(Recti(Vec2f(position.x * screen.width - 2, position.y * screen.height - 2), Vec2f(5, 5)), color);
				}
			}
		}
	}
#endif

	// Config mode
	if (mConfigMode.mEnabled)
	{
		drawer.drawRect(FTX::screenRect(), Color(0.0f, 0.0f, 0.0f, 0.8f));

		Color color = (mConfigMode.mState == ConfigMode::State::DONE_BUTTON_DOWN) ? Color::YELLOW : Color::WHITE;
		color.a = mAlpha;
		drawer.drawRect(getScreenFromNormalizedTouchRect(DONE_BUTTON_RECT), mDoneText, color);
	}

	// Render visual elements
	if (alpha > 0.0f)
	{
		drawer.setSamplingMode(DrawerSamplingMode::BILINEAR);
		for (VisualElement& visualElement : mVisualElements)
		{
			// Skip game rec button is game recording is disabled
			if (visualElement.mReactToState == ConfigMode::State::MOVING_GAMEREC && !Configuration::instance().mGameRecorder.mIsRecording)
				continue;

			const bool pressed = (nullptr == visualElement.mControl) ? false : visualElement.mControl->isPressed();
			const uint64 spriteKey = visualElement.mSpriteKeys[pressed ? 1 : 0];
			const SpriteCache::CacheItem* item = SpriteCache::instance().getSprite(spriteKey);
			if (nullptr == item)
				continue;

			Rectf rect;
			rect.setPos(visualElement.mCenter - visualElement.mHalfExtend);
			rect.setSize(visualElement.mHalfExtend * 2.0f);

			Color color = (mConfigMode.mEnabled && visualElement.mReactToState == mConfigMode.mState) ? Color::CYAN : Color::WHITE;
			color.a = alpha;

			rect = getScreenFromNormalizedTouchRect(rect);
			const Vec2f scale = rect.getSize() / Vec2f(item->mSprite->getSize());
			drawer.drawSprite(rect.getPos() + rect.getSize() / 2, spriteKey, color, scale);
		}
		drawer.setSamplingMode(DrawerSamplingMode::POINT);
	}
}

void TouchControlsOverlay::buildPointButton(const Vec2f& center, float radius, float priority, InputManager::Control& control, InputManager::Control* control2)
{
	TouchArea& touchArea = vectorAdd(mTouchAreas);
	touchArea.mRect.set(center, Vec2f::ZERO);
	touchArea.mRadius = radius;
	touchArea.mPriority = priority;
	touchArea.mControls.push_back(&control);
	if (nullptr != control2)
		touchArea.mControls.push_back(control2);
}

void TouchControlsOverlay::buildRectangularButton(const Vec2f& center, const Vec2f& halfExtend, const char* spriteKey, InputManager::Control* control, ConfigMode::State reactToState, TouchArea::SpecialType specialType)
{
	VisualElement& visualElement = vectorAdd(mVisualElements);
	visualElement.mCenter.set(center);
	visualElement.mHalfExtend = halfExtend;
	visualElement.mSpriteKeys[0] = rmx::getMurmur2_64(spriteKey);
	visualElement.mSpriteKeys[1] = rmx::getMurmur2_64(spriteKey + std::string("_hl"));
	visualElement.mControl = control;
	visualElement.mReactToState = reactToState;

	TouchArea& touchArea = vectorAdd(mTouchAreas);
	touchArea.mSpecialType = specialType;
	touchArea.mRect.set(center - halfExtend, halfExtend * 2.0f);
	touchArea.mRadius = 0.35f;
	touchArea.mPriority = 1.0f;
	if (nullptr != control)
		touchArea.mControls.push_back(control);
}

void TouchControlsOverlay::buildRoundButton(const Vec2f& center, float radius, const char* spriteKey, InputManager::Control& control, ConfigMode::State reactToState)
{
	VisualElement& visualElement = vectorAdd(mVisualElements);
	visualElement.mCenter.set(center);
	visualElement.mHalfExtend.set(radius, radius);
	visualElement.mSpriteKeys[0] = rmx::getMurmur2_64(spriteKey);
	visualElement.mSpriteKeys[1] = rmx::getMurmur2_64(spriteKey + std::string("_hl"));
	visualElement.mControl = &control;
	visualElement.mReactToState = reactToState;

	TouchArea& touchArea = vectorAdd(mTouchAreas);
	touchArea.mRect.set(center, Vec2f::ZERO);
	touchArea.mRadius = radius + 0.4f;
	touchArea.mPriority = 1.0f;
	touchArea.mControls.push_back(&control);
}

Vec2f TouchControlsOverlay::getNormalizedTouchFromScreenPosition(Vec2f vec) const
{
	return (vec - mScreenCenter) / mScreenScale;
}

Vec2f TouchControlsOverlay::getScreenFromNormalizedTouchPosition(Vec2f vec) const
{
	return mScreenCenter + vec * mScreenScale;
}

Rectf TouchControlsOverlay::getScreenFromNormalizedTouchRect(Rectf rect) const
{
	return Rectf(mScreenCenter + rect.getPos() * mScreenScale, rect.getSize() * mScreenScale);
}

const TouchControlsOverlay::TouchArea* TouchControlsOverlay::getTouchAreaAtNormalizedPosition(const Vec2f& position) const
{
	const Vec2f touchAreaCoords = getNormalizedTouchFromScreenPosition(position * Vec2f(FTX::screenSize()));
	const TouchArea* bestTouchArea = nullptr;
	float bestWeight = 0.0f;
	for (const TouchArea& touchArea : mTouchAreas)
	{
		const float weight = touchArea.getWeight(touchAreaCoords);
		if (weight > bestWeight)
		{
			bestTouchArea = &touchArea;
			bestWeight = weight;
		}
	}
	return bestTouchArea;
}

void TouchControlsOverlay::updateControls()
{
	if (mConfigMode.mEnabled)
	{
		updateConfigMode();
		return;
	}

	// Update touch areas
	bool gameRecPressed = false;
	if (!mInputManager->getActiveTouches().empty())
	{
		// There is at least one active touch, reset auto-hide
		mAutoHideTimer = 0.0f;

		for (const InputManager::Touch& touch : mInputManager->getActiveTouches())
		{
			TouchArea* touchArea = const_cast<TouchArea*>(getTouchAreaAtNormalizedPosition(touch.mPosition));
			if (nullptr != touchArea)
			{
				switch (touchArea->mSpecialType)
				{
					case TouchArea::SpecialType::GAMEREC:
					{
						gameRecPressed = true;
						break;
					}
					default:
					{
						for (InputManager::Control* control : touchArea->mControls)
							control->mState = true;
						break;
					}
				}
			}
		}
	}

	if (gameRecPressed != mGameRecPressed)
	{
		mGameRecPressed = gameRecPressed;
		if (gameRecPressed)
		{
			Application::instance().triggerGameRecordingSave();
		}
	}
}

void TouchControlsOverlay::updateConfigMode()
{
	// Config mode
	if (mInputManager->getActiveTouches().empty())
	{
		if (mConfigMode.mState == ConfigMode::State::DONE_BUTTON_DOWN)
		{
			mConfigMode.mEnabled = false;
		}
		mConfigMode.mState = ConfigMode::State::TOUCH_UP;
	}
	else
	{
		const InputManager::Touch& touch = mInputManager->getActiveTouches()[0];
		const Vec2f touchPosition = getNormalizedTouchFromScreenPosition(touch.mPosition * Vec2f(FTX::screenSize()));
		Configuration& config = Configuration::instance();

		switch (mConfigMode.mState)
		{
			case ConfigMode::State::TOUCH_UP:
			{
				if (touchPosition.sqrDist(mSetup.mDirectionalPadCenter) < square(mSetup.mDirectionalPadSize * 0.5f))
				{
					mConfigMode.mState = ConfigMode::State::MOVING_DPAD;
					mConfigMode.mLastTargetPosition = Vec2f(config.mVirtualGamepad.mDirectionalPadCenter);
				}
				// TODO: This is work-in-progress
				//else if (touchPosition.sqrDist(mSetup.mDirectionalPadCenter) < square(mSetup.mDirectionalPadSize * 0.7f))
				//{
				//	mConfigMode.mState = ConfigMode::State::SCALING_DPAD;
				//	mConfigMode.mLastTargetPosition.x = (float)config.mVirtualGamepad.mDirectionalPadSize;
				//}
				else if (touchPosition.sqrDist(mSetup.mFaceButtonsCenter) < square(mSetup.mFaceButtonsSize * 0.5f))
				{
					mConfigMode.mState = ConfigMode::State::MOVING_BUTTONS;
					mConfigMode.mLastTargetPosition = Vec2f(config.mVirtualGamepad.mFaceButtonsCenter);
				}
				// TODO: This is work-in-progress
				//else if (touchPosition.sqrDist(mSetup.mFaceButtonsCenter) < square(mSetup.mFaceButtonsSize * 0.7f))
				//{
				//	mConfigMode.mState = ConfigMode::State::SCALING_BUTTONS;
				//	mConfigMode.mLastTargetPosition.x = (float)config.mVirtualGamepad.mFaceButtonsSize;
				//}
				else if (touchPosition.sqrDist(mSetup.mStartButtonCenter) < square(0.3f))
				{
					mConfigMode.mState = ConfigMode::State::MOVING_START;
					mConfigMode.mLastTargetPosition = Vec2f(config.mVirtualGamepad.mStartButtonCenter);
				}
				else if (touchPosition.sqrDist(mSetup.mGameRecButtonCenter) < square(0.3f))
				{
					mConfigMode.mState = ConfigMode::State::MOVING_GAMEREC;
					mConfigMode.mLastTargetPosition = Vec2f(config.mVirtualGamepad.mGameRecButtonCenter);
				}
				else if (touchPosition.sqrDist(::DONE_BUTTON_RECT.getClosestPoint(touchPosition)) < square(0.075f))
				{
					mConfigMode.mState = ConfigMode::State::DONE_BUTTON_DOWN;
				}
				else
				{
					mConfigMode.mState = ConfigMode::State::TOUCH_DOWN;
				}
				break;
			}

			case ConfigMode::State::DONE_BUTTON_UP:
			case ConfigMode::State::DONE_BUTTON_DOWN:
			{
				mConfigMode.mState = (touchPosition.sqrDist(::DONE_BUTTON_RECT.getClosestPoint(touchPosition)) < square(0.075f)) ? ConfigMode::State::DONE_BUTTON_DOWN : ConfigMode::State::DONE_BUTTON_UP;
				break;
			}

			case ConfigMode::State::MOVING_DPAD:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition += (touchPosition - mConfigMode.mLastTouchPosition) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mDirectionalPadCenter.set(roundToInt(mConfigMode.mLastTargetPosition.x), roundToInt(mConfigMode.mLastTargetPosition.y));
					buildTouchControls();
				}
				break;
			}

			case ConfigMode::State::MOVING_BUTTONS:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition += (touchPosition - mConfigMode.mLastTouchPosition) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mFaceButtonsCenter.set(roundToInt(mConfigMode.mLastTargetPosition.x), roundToInt(mConfigMode.mLastTargetPosition.y));
					buildTouchControls();
				}
				break;
			}

			case ConfigMode::State::MOVING_START:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition += (touchPosition - mConfigMode.mLastTouchPosition) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mStartButtonCenter.set(roundToInt(mConfigMode.mLastTargetPosition.x), roundToInt(mConfigMode.mLastTargetPosition.y));
					buildTouchControls();
				}
				break;
			}

			case ConfigMode::State::MOVING_GAMEREC:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition += (touchPosition - mConfigMode.mLastTouchPosition) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mGameRecButtonCenter.set(roundToInt(mConfigMode.mLastTargetPosition.x), roundToInt(mConfigMode.mLastTargetPosition.y));
					buildTouchControls();
				}
				break;
			}

			case ConfigMode::State::SCALING_DPAD:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition.x += (touchPosition - mConfigMode.mLastTouchPosition).dot(touchPosition - mSetup.mDirectionalPadCenter) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mDirectionalPadSize = clamp(roundToInt(mConfigMode.mLastTargetPosition.x), 50, 150);
					buildTouchControls();
				}
				break;
			}

			case ConfigMode::State::SCALING_BUTTONS:
			{
				if (touchPosition != mConfigMode.mLastTouchPosition)
				{
					mConfigMode.mLastTargetPosition.x += (touchPosition - mConfigMode.mLastTouchPosition).dot(touchPosition - mSetup.mFaceButtonsCenter) / (CONFIG_TO_SETUP_SCALE * 0.5f);
					config.mVirtualGamepad.mFaceButtonsSize = clamp(roundToInt(mConfigMode.mLastTargetPosition.x), 50, 150);
					buildTouchControls();
				}
				break;
			}

			default:
				break;
		}

		mConfigMode.mLastTouchPosition = touchPosition;
	}

	// No auto-hide in config mode
	mAutoHideTimer = 0.0f;
}
