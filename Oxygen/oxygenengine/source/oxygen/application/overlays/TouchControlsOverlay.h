/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/application/input/InputManager.h"


class TouchControlsOverlay : public GuiBase, public InputFeeder, public SingleInstance<TouchControlsOverlay>
{
public:
	// The coordinate system used here
	//  - is centered on the screen
	//  - uses the interval -1.0 .. 1.0f in y-direction for the game's 16:9 letter box height
	//  - is respecting the screen aspect ratio, so in x-direction it's e.g. -1.77f to 1.77f for the game's 16:9 letter box width

	struct Setup
	{
		Vec2f mDirectionalPadCenter;
		float mDirectionalPadSize = 1.0f;
		Vec2f mFaceButtonsCenter;
		float mFaceButtonsSize = 1.0f;
		Vec2f mStartButtonCenter;
		Vec2f mGameRecButtonCenter;
		Vec2f mShoulderLButtonCenter;
		Vec2f mShoulderRButtonCenter;
	};
	Setup mSetup;

public:
	TouchControlsOverlay();
	~TouchControlsOverlay();

	void buildTouchControls();
	void setForceHidden(bool hidden);

	bool isInConfigMode() const;
	void enableConfigMode(bool enable);

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

protected:
	virtual void updateControls() override;

private:
	struct ConfigMode
	{
		enum class State
		{
			TOUCH_UP,
			TOUCH_DOWN,
			DONE_BUTTON_UP,
			DONE_BUTTON_DOWN,
			MOVING_DPAD,
			MOVING_FACE_BUTTONS,
			MOVING_START,
			MOVING_GAMEREC,
			MOVING_L,
			MOVING_R,
			SCALING_DPAD,
			SCALING_BUTTONS
		};

		bool mEnabled = false;
		State mState = State::TOUCH_UP;
		Vec2f mLastTouchPosition;
		Vec2f mLastTargetPosition;
	};

	struct TouchArea
	{
		enum class SpecialType
		{
			NONE,
			GAMEREC
		};

		SpecialType mSpecialType = SpecialType ::NONE;
		Rectf mRect;				// Main rectangle, using the touch area coordinate system
		float mRadius = 0.0f;		// Additional radius outside of the rectangle
		float mPriority = 1.0f;
		std::vector<InputManager::Control*> mControls;

		float getWeight(const Vec2f& position) const;
	};

	struct VisualElement
	{
		Vec2f mCenter;			// Center position on screen (see remarks on the coordinate system above)
		Vec2f mHalfExtend;		// Relative half size on screen
		uint64 mSpriteKeys[2] = { 0, 0 };
		InputManager::Control* mControl = nullptr;
		ConfigMode::State mReactToState;
	};

private:
	void buildPointButton(const Vec2f& center, float radius, float priority, InputManager::Control& control, InputManager::Control* control2);
	void buildRectangularButton(const Vec2f& center, const Vec2f& halfExtend, const char* spriteKey, InputManager::Control* control, ConfigMode::State reactToState, TouchArea::SpecialType specialType = TouchArea::SpecialType::NONE, float radius = 0.35f);
	void buildRoundButton(const Vec2f& center, float radius, const char* spriteKey, InputManager::Control& control, ConfigMode::State reactToState);

	const TouchArea* getTouchAreaAtNormalizedPosition(const Vec2f& position) const;
	Vec2f getNormalizedTouchFromScreenPosition(Vec2f vec) const;
	Vec2f getScreenFromNormalizedTouchPosition(Vec2f vec) const;
	Rectf getScreenFromNormalizedTouchRect(Rectf rect) const;

	void updateConfigMode();
	void updateButtonPosition(Vec2i& position, Vec2f touchPosition);

private:
	Vec2i mLastScreenSize;

	std::vector<TouchArea> mTouchAreas;
	Vec2f mScreenCenter;	// Used for coordinate system conversion: Screen center in screen space (i.e. counting pixels)
	Vec2f mScreenScale;		// Used for coordinate system conversion: Screen scale in screen space

	std::vector<VisualElement> mVisualElements;
	DrawerTexture mDoneText;

	float mAutoHideTimer = 0.0f;
	bool mForceHidden = false;
	float mVisibility = 0.0f;
	bool mGameRecPressed = false;

	ConfigMode mConfigMode;
};
