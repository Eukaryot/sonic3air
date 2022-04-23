/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/RenderResources.h"
#include "oxygen/rendering/hardware/HardwareRenderer.h"
#include "oxygen/rendering/software/SoftwareRenderer.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"


VideoOut::VideoOut() :
	mRenderResources(*new RenderResources())
{
	mGeometries.reserve(0x100);
}

VideoOut::~VideoOut()
{
	delete mHardwareRenderer;
	delete mSoftwareRenderer;
	delete mRenderParts;
	delete &mRenderResources;
}

void VideoOut::startup()
{
	mGameResolution = Configuration::instance().mGameScreen;

	RMX_LOG_INFO("VideoOut: Setup of game screen");
	EngineMain::instance().getDrawer().createTexture(mGameScreenTexture);
	mGameScreenTexture.setupAsRenderTarget(mGameResolution.x, mGameResolution.y);

	if (nullptr == mRenderParts)
	{
		RMX_LOG_INFO("VideoOut: Creating render parts");
		mRenderParts = new RenderParts();
		mRenderParts->setFullEmulation(Configuration::instance().mFullEmulationRendering);
	}

	createRenderer(false);
}

void VideoOut::shutdown()
{
	clearGeometries();
}

void VideoOut::reset()
{
	mRenderParts->reset();
	mActiveRenderer->reset();

	mDebugDrawRenderingRequested = false;
	mPreviouslyHadOutsideFrameDebugDraws = false;
}

void VideoOut::handleActiveModsChanged()
{
	// Better reset everything, as sprite references might have become invalid and should be removed
	reset();
}

void VideoOut::createRenderer(bool reset)
{
	setActiveRenderer(Configuration::instance().mRenderMethod != Configuration::RenderMethod::OPENGL_FULL, reset);
}

void VideoOut::destroyRenderer()
{
	SAFE_DELETE(mHardwareRenderer);
	SAFE_DELETE(mSoftwareRenderer);
}

void VideoOut::setActiveRenderer(bool useSoftwareRenderer, bool reset)
{
	if (useSoftwareRenderer)
	{
		if (nullptr == mSoftwareRenderer)
		{
			RMX_LOG_INFO("VideoOut: Creating software renderer");
			mSoftwareRenderer = new SoftwareRenderer(*mRenderParts, mGameScreenTexture);

			RMX_LOG_INFO("VideoOut: Renderer initialization");
			mSoftwareRenderer->initialize();
		}
		mActiveRenderer = mSoftwareRenderer;
	}
	else
	{
		if (nullptr == mHardwareRenderer)
		{
			RMX_LOG_INFO("VideoOut: Creating hardware renderer");
			mHardwareRenderer = new HardwareRenderer(*mRenderParts, mGameScreenTexture);

			RMX_LOG_INFO("VideoOut: Renderer initialization");
			mHardwareRenderer->initialize();
		}
		mActiveRenderer = mHardwareRenderer;
	}

	if (reset)
	{
		mActiveRenderer->reset();
		mActiveRenderer->setGameResolution(mGameResolution);
	}
}

void VideoOut::setScreenSize(uint32 width, uint32 height)
{
	mGameResolution.x = width;
	mGameResolution.y = height;

	mGameScreenTexture.setupAsRenderTarget(mGameResolution.x, mGameResolution.y);

	mActiveRenderer->setGameResolution(mGameResolution);
}

Vec2i VideoOut::getInterpolatedWorldSpaceOffset() const
{
	Vec2i offset = mRenderParts->getSpacesManager().getWorldSpaceOffset();
	if (mUsingFrameInterpolation)
	{
		const Vec2f interpolatedDifference = Vec2f(mLastWorldSpaceOffset - offset) * (1.0f - mInterFramePosition);
		offset += Vec2i(roundToInt(interpolatedDifference.x), roundToInt(interpolatedDifference.y));
	}
	return offset;
}

void VideoOut::preFrameUpdate()
{
	mRenderParts->preFrameUpdate();
	mLastWorldSpaceOffset = mRenderParts->getSpacesManager().getWorldSpaceOffset();

	// Skipped frames without rendering?
	if (mFrameState == FrameState::FRAME_READY)
	{
		// Processing of last frame (to avoid e.g. sprites rendered multiple times)
		RefreshParameters refreshParameters;
		refreshParameters.mSkipThisFrame = true;
		mRenderParts->refresh(refreshParameters);
	}

	mFrameState = FrameState::INSIDE_FRAME;
	mDebugDrawRenderingRequested = false;
}

void VideoOut::postFrameUpdate()
{
	mRenderParts->postFrameUpdate();

	// Signal for rendering
	mFrameState = FrameState::FRAME_READY;
	mLastFrameTicks = SDL_GetTicks();
	mDebugDrawRenderingRequested = false;
}

void VideoOut::setInterFramePosition(float position)
{
	mInterFramePosition = position;
}

bool VideoOut::updateGameScreen()
{
	mUsingFrameInterpolation = (Configuration::instance().mFrameSync == Configuration::FrameSyncType::FRAME_INTERPOLATION);

	// Only render something if a frame simulation was completed in the meantime
	const bool hasNewSimulationFrame = (mFrameState == FrameState::FRAME_READY);
	if (!hasNewSimulationFrame && !mUsingFrameInterpolation && !mDebugDrawRenderingRequested)
	{
		// No update
		return false;
	}

	mFrameState = FrameState::OUTSIDE_FRAME;

	RefreshParameters refreshParameters;
	refreshParameters.mSkipThisFrame = false;
	refreshParameters.mHasNewSimulationFrame = hasNewSimulationFrame;
	refreshParameters.mUsingFrameInterpolation = mUsingFrameInterpolation;
	refreshParameters.mInterFramePosition = mInterFramePosition;
	mRenderParts->refresh(refreshParameters);

	// Render a new image
	renderGameScreen();

	// Game screen got updated
	return true;
}

void VideoOut::blurGameScreen()
{
	if (mActiveRenderer == mHardwareRenderer)
	{
		mHardwareRenderer->blurGameScreen();
	}
}

void VideoOut::toggleLayerRendering(int index)
{
	mRenderParts->mLayerRendering[index] = !mRenderParts->mLayerRendering[index];
}

std::string VideoOut::getLayerRenderingDebugString() const
{
	char string[10] = "basc BASC";
	for (int i = 0; i < 8; ++i)
	{
		if (!mRenderParts->mLayerRendering[i])
			string[i + i/4] = '-';
	}
	return string;
}

void VideoOut::getScreenshot(Bitmap& outBitmap)
{
	mGameScreenTexture.writeContentToBitmap(outBitmap);
}

void VideoOut::clearGeometries()
{
	for (Geometry* geometry : mGeometries)
	{
		mGeometryFactory.destroy(*geometry);
	}
	mGeometries.clear();

	// Regularly cleanup old cache items -- it's safe now that no geometry references a texture in there any more
	RenderResources::instance().mPrintedTextCache.regularCleanup();
}

void VideoOut::collectGeometries(std::vector<Geometry*>& geometries)
{
	// Add plane geometries
	{
		const PlaneManager& pm = mRenderParts->getPlaneManager();
		const Recti fullscreenRect(0, 0, mGameResolution.x, mGameResolution.y);
		Recti rectForPlaneB = fullscreenRect;
		Recti rectForPlaneA = fullscreenRect;
		Recti rectForPlaneW = fullscreenRect;
		if (pm.isPlaneUsed(PlaneManager::PLANE_W))
		{
			const int splitY = pm.getPlaneAWSplit();
			rectForPlaneA.height = splitY;
			rectForPlaneW.y = splitY;
			rectForPlaneW.height -= splitY;
		}
		else
		{
			rectForPlaneW.height = 0;
		}

		// Plane B non-prio
		if (mRenderParts->mLayerRendering[0] && pm.isDefaultPlaneEnabled(0))
		{
			geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneB, PlaneManager::PLANE_B, false, PlaneManager::PLANE_B, 0x1000));
		}

		// Plane A (and possibly plane W) non-prio
		if (mRenderParts->mLayerRendering[1] && pm.isDefaultPlaneEnabled(1))
		{
			if (rectForPlaneA.height > 0)
			{
				geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneA, PlaneManager::PLANE_A, false, PlaneManager::PLANE_A, 0x2000));
			}
			if (rectForPlaneW.height > 0)
			{
				geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneW, PlaneManager::PLANE_W, false, 0xff, 0x2000));
			}
		}

		// Plane B prio
		if (mRenderParts->mLayerRendering[4] && pm.isDefaultPlaneEnabled(2))
		{
			geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneB, PlaneManager::PLANE_B, true, PlaneManager::PLANE_B, 0x3000));
		}

		// Plane A (and possibly plane W) prio
		if (mRenderParts->mLayerRendering[5] && pm.isDefaultPlaneEnabled(3))
		{
			if (rectForPlaneA.height > 0)
			{
				geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneA, PlaneManager::PLANE_A, true, PlaneManager::PLANE_A, 0x4000));
			}
			if (rectForPlaneW.height > 0)
			{
				geometries.push_back(&mGeometryFactory.createPlaneGeometry(rectForPlaneW, PlaneManager::PLANE_W, true, 0xff, 0x4000));
			}
		}

		if (!pm.getCustomPlanes().empty())
		{
			for (const auto& customPlane : pm.getCustomPlanes())
			{
				geometries.push_back(&mGeometryFactory.createPlaneGeometry(customPlane.mRect, customPlane.mSourcePlane & 0x03, (customPlane.mSourcePlane & 0x10) != 0, customPlane.mScrollOffsets, customPlane.mRenderQueue));
			}
		}
	}

	// Add sprite geometries
	SpriteManager& spriteManager = mRenderParts->getSpriteManager();
	{
		const Vec2i worldSpaceOffset = mRenderParts->getSpacesManager().getWorldSpaceOffset();
		const auto& sprites = spriteManager.getSprites();
		for (auto spriteIterator = sprites.begin(); spriteIterator != sprites.end(); ++spriteIterator)
		{
			SpriteManager::SpriteInfo& sprite = **spriteIterator;
			bool accept = true;
			switch (sprite.getType())
			{
				case SpriteManager::SpriteInfo::Type::VDP:
				{
					accept = (mRenderParts->mLayerRendering[sprite.mPriorityFlag ? 6 : 2]);
					break;
				}

				case SpriteManager::SpriteInfo::Type::PALETTE:
				case SpriteManager::SpriteInfo::Type::COMPONENT:
				{
					accept = (mRenderParts->mLayerRendering[sprite.mPriorityFlag ? 7 : 3]);
					break;
				}

				default:
					// Accept everything else
					break;
			}

			if (accept)
			{
				sprite.mInterpolatedPosition = sprite.mPosition;
				if (mUsingFrameInterpolation)
				{
					Vec2i difference;
					if (sprite.mHasLastPosition)
					{
						difference = sprite.mLastPositionChange;
					}
					else if (sprite.mLogicalSpace == SpriteManager::Space::WORLD)
					{
						// Assume sprite is standing still in world space, i.e. moving entirely with camera
						difference = mLastWorldSpaceOffset - worldSpaceOffset;
					}
					else
					{
						// Assume sprite is standing still in screen space, i.e. not moving on the screen
					}

					if ((difference.x != 0 || difference.y != 0) && (abs(difference.x) < 0x40 && abs(difference.y) < 0x40))
					{
						const Vec2f interpolatedDifference = Vec2f(difference) * (1.0f - mInterFramePosition);
						sprite.mInterpolatedPosition -= Vec2i(roundToInt(interpolatedDifference.x), roundToInt(interpolatedDifference.y));
					}
				}

				SpriteGeometry& spriteGeometry = mGeometryFactory.createSpriteGeometry(sprite);
				spriteGeometry.mRenderQueue = sprite.mRenderQueue;
				geometries.push_back(&spriteGeometry);
			}
		}
	}

	// Insert blur effect geometry at the right position
	if (Configuration::instance().mBackgroundBlur > 0)
	{
		constexpr uint16 BLUR_RENDER_QUEUE = 0x1800;

		// Anything there to blur at all?
		//  -> There might be no blurred background at all (e.g. in S3K Sky Sanctuary upper levels)
		bool blurNeeded = false;
		for (const Geometry* geometry : geometries)
		{
			if (geometry->mRenderQueue < BLUR_RENDER_QUEUE)
			{
				blurNeeded = true;
				break;
			}
		}

		if (blurNeeded)
		{
			Geometry& geometry = mGeometryFactory.createEffectBlurGeometry(Configuration::instance().mBackgroundBlur);
			geometry.mRenderQueue = BLUR_RENDER_QUEUE - 1;
			geometries.push_back(&geometry);
		}
	}

	// Insert viewports
	for (const RenderParts::Viewport& viewport : mRenderParts->getViewports())
	{
		Geometry& geometry = mGeometryFactory.createViewportGeometry(viewport.mRect);
		geometry.mRenderQueue = viewport.mRenderQueue;
		geometries.push_back(&geometry);
	}

	// Insert debug draw rects
	{
		const OverlayManager& overlayManager = RenderParts::instance().getOverlayManager();
		const Vec2i worldSpaceOffset = getInterpolatedWorldSpaceOffset();
		for (int i = 0; i < OverlayManager::NUM_CONTEXTS; ++i)
		{
			const std::vector<OverlayManager::DebugDrawRect>& debugDrawRects = overlayManager.getDebugDrawRects((OverlayManager::Context)i);
			for (const OverlayManager::DebugDrawRect& debugDrawRect : debugDrawRects)
			{
				// Translate rect
				Recti screenRect = debugDrawRect.mRect;
				if (debugDrawRect.mSpace == SpacesManager::Space::WORLD)
				{
					screenRect -= worldSpaceOffset;
				}

				Geometry& geometry = mGeometryFactory.createRectGeometry(screenRect, debugDrawRect.mColor);
				geometry.mRenderQueue = 0xffff;		// Always on top
				geometries.push_back(&geometry);
			}

			const std::vector<OverlayManager::Text>& texts = overlayManager.getTexts((OverlayManager::Context)i);
			for (const OverlayManager::Text& text : texts)
			{
				// Translate position
				Vec2i screenPosition = text.mPosition;
				if (text.mSpace == SpacesManager::Space::WORLD)
				{
					screenPosition -= worldSpaceOffset;
				}

				Font* font = ResourcesCache::instance().getFontByKey(text.mFontKeyString, text.mFontKeyHash);
				if (nullptr != font)
				{
					const PrintedTextCache::Key key(text.mFontKeyHash, text.mTextHash, text.mSpacing);
					PrintedTextCache& cache = RenderResources::instance().mPrintedTextCache;
					PrintedTextCache::CacheItem* cacheItem = cache.getCacheItem(key);
					if (nullptr == cacheItem)
					{
						cacheItem = &cache.addCacheItem(key, *font, text.mTextString);
					}
					const Vec2i drawPosition = Font::applyAlignment(Recti(screenPosition, Vec2i(0, 0)), cacheItem->mInnerRect, text.mAlignment);
					const Recti rect(drawPosition, cacheItem->mTexture.getSize());

					Geometry& geometry = mGeometryFactory.createTexturedRectGeometry(rect, cacheItem->mTexture, text.mColor);
					geometry.mRenderQueue = text.mRenderQueue;
					geometries.push_back(&geometry);
				}
			}
		}
	}

	// Sort everything by render queue
	std::stable_sort(geometries.begin(), geometries.end(),
					 [](const Geometry* a, const Geometry* b) { return a->mRenderQueue < b->mRenderQueue; });
}

void VideoOut::renderGameScreen()
{
	// Collect geometries to render
	clearGeometries();
	if (mRenderParts->getActiveDisplay())
	{
		collectGeometries(mGeometries);
	}

	// Render them
	mActiveRenderer->renderGameScreen(mGeometries);
}

void VideoOut::preRefreshDebugging()
{
	mRenderParts->getOverlayManager().clearContext(OverlayManager::Context::OUTSIDE_FRAME);
}

void VideoOut::postRefreshDebugging()
{
	const bool hasOutsideFrameDebugDraws = !mRenderParts->getOverlayManager().getDebugDrawRects(OverlayManager::Context::OUTSIDE_FRAME).empty();
	mDebugDrawRenderingRequested = hasOutsideFrameDebugDraws || mPreviouslyHadOutsideFrameDebugDraws;
	mPreviouslyHadOutsideFrameDebugDraws = hasOutsideFrameDebugDraws;
}

void VideoOut::renderDebugDraw(int debugDrawMode, const Recti& rect)
{
	mActiveRenderer->renderDebugDraw(debugDrawMode, rect);
}

void VideoOut::dumpDebugDraw(int debugDrawMode)
{
	if (debugDrawMode < 2)
	{
		mRenderParts->dumpPlaneContent(debugDrawMode);
	}
	else
	{
		mRenderParts->dumpPatternsContent();
	}
}
