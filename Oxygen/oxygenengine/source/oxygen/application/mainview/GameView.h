/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/DrawerTexture.h"

class Simulation;


class GameView : public GuiBase
{
public:
	enum class StillImageMode
	{
		NONE,			// Normal running simulation, no still image
		STILL_IMAGE,	// Permanently showing the last rendered image
		BLURRING		// Showing & incrementally blurring the last rendered image
	};

public:
	GameView(Simulation& simulation);
	~GameView();

	inline const Rectf& getGameViewport() const  { return mGameViewport; }
	void updateGameViewport();

	bool translatePositionIntoGameViewport(Vec2f& outPosition, const Vec2f& inPosition) const;
	void translateRectIntoGameViewport(Rectf& outRect, const Rectf& inRect) const;
	void translatePositionIntoScreenCoords(Vec2f& outPosition, const Vec2f& inPosition) const;
	void translateRectIntoScreenCoords(Rectf& outRect, const Rectf& inRect) const;

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	inline float isFading() const    { return (mFadeChange > 0.0f && mFadeValue < 1.0f) || (mFadeChange < 0.0f && mFadeValue > 0.0f); }
	inline float isFadedOut() const  { return mFadeValue <= 0.0f && mFadeChange <= 0.0f; }
	void setFadedIn();
	void startFadingIn(float fadeTime = 0.25f);
	void startFadingOut(float fadeTime = 0.25f);
	inline void setWhiteOverlayAlpha(float alpha)  { mWhiteOverlayAlpha = alpha; }

	void getScreenshot(Bitmap& outBitmap);

	void setStillImageMode(StillImageMode mode, float timeout = 0.0f);

private:
	void setLogDisplay(const String& string, float time = 2.0f);
	void setGameSpeed(float speed);

private:
	struct StillImage
	{
		StillImageMode mMode = StillImageMode::NONE;
		float mBlurringTimeout = 0.0f;
		float mBlurringStepTimer = 0.0f;
	};

private:
	Simulation& mSimulation;

	Rectf mGameViewport;

	float mFadeValue = 1.0f;
	float mFadeChange = 0.0f;
	float mWhiteOverlayAlpha = 0.0f;

	StillImage mStillImage;

	int  mDebugOutput = -1;
	int  mDebugPaletteDisplay = -1;
	bool mDebugVisualizationsEnabled = false;
	int  mDebugVisualizationsMode = 0;
	DrawerTexture mDebugVisualizationsOverlay;

	DrawerTexture mFinalGameTexture;

	float mRewindTimer = 0.0f;
	int mRewindCounter = 0;
};
