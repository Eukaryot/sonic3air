/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/BackdropView.h"
#include "oxygen/application/Application.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	void getDifferenceRects(std::vector<Recti>& outRects, const Recti& rectA, const Recti& rectB)
	{
		// Calculate difference A - B
		outRects.clear();

		const Vec2i a1 = rectA.getPos();
		const Vec2i a2 = a1 + rectA.getSize();
		const Vec2i b1 = rectB.getPos();
		const Vec2i b2 = b1 + rectB.getSize();
		if (b1.x >= a2.x || b2.x <= a1.x || b1.y >= a2.y || b2.y <= a1.y)
		{
			// There's no intersection at all, output full A
			outRects.emplace_back(rectA);
			return;
		}

		const int left   = b1.x - a1.x;
		const int right  = a2.x - b2.x;
		const int top    = b1.y - a1.y;
		const int bottom = a2.y - b2.y;
		if (left <= 0 && right <= 0 && top <= 0 && bottom <= 0)
		{
			// Rect B covers A completely, nothing remains
			return;
		}

		if (left > 0)
		{
			// Output the difference rect on the left (with full height of A)
			outRects.emplace_back(a1.x, a1.y, left, rectA.height);
		}
		if (right > 0)
		{
			// Output the difference rect on the right (with full height of A)
			outRects.emplace_back(b2.x, a1.y, right, rectA.height);
		}
		if (top > 0)
		{
			// Output the difference rect on the top (with only the width of B)
			outRects.emplace_back(b1.x, a1.y, rectB.width, top);
		}
		if (bottom > 0)
		{
			// Output the difference rect on the bottom (with only the width of B)
			outRects.emplace_back(b1.x, b2.y, rectB.width, bottom);
		}
	}
}


void BackdropView::setGameViewRect(const Recti& rect)
{
	// Add a black border
	const int border = 4;
	const Recti rectWithBorder(rect.x - border, rect.y - border, rect.width + border * 2, rect.height + border * 2);
	if (mCachedGameViewRect != rectWithBorder)
	{
		mCachedGameViewRect = rectWithBorder;
		mCachedScreenRect = FTX::screenRect();
		getDifferenceRects(mRenderRects, FTX::screenRect(), mCachedGameViewRect);
	}
}

void BackdropView::initialize()
{
}

void BackdropView::update(float timeElapsed)
{
	if (Configuration::instance().mRenderMethod != Configuration::RenderMethod::SOFTWARE)
	{
		mAnimationTime += timeElapsed;
	}

	float targetMultiplier = 1.0f;
	Simulation& simulation = Application::instance().getSimulation();
	if (simulation.isRunning() && simulation.getSpeed() > 0.0f)
	{
		// Adapt to game screen fading
		const Color globalComponentTint = RenderParts::instance().getPaletteManager().getGlobalComponentTintColor();
		const Color globalComponentAdded = RenderParts::instance().getPaletteManager().getGlobalComponentAddedColor();
		targetMultiplier = saturate(globalComponentTint.getGray() + globalComponentAdded.getGray());
	}

	// After fading to black, stay there for a short while in any case
	if (targetMultiplier < 0.1f)
	{
		targetMultiplier = 0.0f;
		if (mHasBlackTimeout)
		{
			mBlackTimeout -= timeElapsed;
		}
		else
		{
			// Start timeout
			mHasBlackTimeout = true;
			mBlackTimeout = 0.6f;
		}
	}
	else
	{
		if (mHasBlackTimeout)
		{
			mBlackTimeout -= timeElapsed;
			if (mBlackTimeout <= 0.0f)
			{
				// Stop timeout
				mHasBlackTimeout = false;
			}
			else
			{
				// Stay black
				targetMultiplier = 0.0f;
			}
		}
	}

	if (targetMultiplier != mColorMultiplier)
	{
		if (targetMultiplier < mColorMultiplier)
			mColorMultiplier = std::max(mColorMultiplier - timeElapsed * 5.0f, targetMultiplier);
		else
			mColorMultiplier = std::min(mColorMultiplier + timeElapsed * 3.0f, targetMultiplier);
	}
}

void BackdropView::render()
{
	const int backdropSetting = Configuration::instance().mBackdrop;
	if (backdropSetting == 0)
		return;

	if (mCachedBackdropSetting != backdropSetting)
	{
		mCachedBackdropSetting = backdropSetting;
		switch (mCachedBackdropSetting)
		{
			case 1:  FileHelper::loadTexture(mBackdropTexture, L"data/images/backdrop/classic1.png");  break;
			case 2:  FileHelper::loadTexture(mBackdropTexture, L"data/images/backdrop/classic2.png");  break;
			case 3:  FileHelper::loadTexture(mBackdropTexture, L"data/images/backdrop/classic3.png");  break;
		}
	}

	if (mCachedScreenRect != FTX::screenRect())
	{
		mCachedScreenRect = FTX::screenRect();
		getDifferenceRects(mRenderRects, FTX::screenRect(), mCachedGameViewRect);
	}

	if (!mRenderRects.empty())
	{
		const Vec2i center = mCachedScreenRect.getPos() + mCachedScreenRect.getSize() / 2;
		const Vec2f textureSize(mBackdropTexture.getSize());
		const float scaling = 2.0f;

		// Add some subtle movement to avoid burn-in on sensitive screens (except with pure software renderer, where this effect is quite annoying due to missing interpolation)
		const Vec2f animationOffset(mAnimationTime * 1.0f, mAnimationTime * 0.1f);

		Drawer& drawer = EngineMain::instance().getDrawer();
		drawer.setSamplingMode(SamplingMode::BILINEAR);
		drawer.setWrapMode(TextureWrapMode::REPEAT);
		for (const Recti& rect : mRenderRects)
		{
			const Vec2f uv0 = (Vec2f(rect.getPos() - center) + animationOffset) / textureSize / scaling;
			const Vec2f uv1 = (Vec2f(rect.getPos() + rect.getSize() - center) + animationOffset) / textureSize / scaling;
			drawer.drawRect(rect, mBackdropTexture, uv0, uv1, Color(mColorMultiplier, mColorMultiplier, mColorMultiplier));
		}
		drawer.setWrapMode(TextureWrapMode::CLAMP);
	}
}
