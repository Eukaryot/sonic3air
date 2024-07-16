/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/OpenGLRenderer.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/drawing/opengl/OpenGLDrawer.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/opengl/shaders/DebugDrawPlaneShader.h"
#include "oxygen/rendering/opengl/shaders/PostFXBlurShader.h"
#include "oxygen/rendering/opengl/shaders/RenderComponentSpriteShader.h"
#include "oxygen/rendering/opengl/shaders/RenderPaletteSpriteShader.h"
#include "oxygen/rendering/opengl/shaders/RenderPlaneShader.h"
#include "oxygen/rendering/opengl/shaders/RenderVdpSpriteShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleCopyScreenShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectColoredShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectOverdrawShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectTexturedShader.h"
#include "oxygen/simulation/LogDisplay.h"


namespace
{
	OpenGLDrawerResources& getDrawerResources()
	{
		DrawerInterface* drawer = EngineMain::instance().getDrawer().getActiveDrawer();
		RMX_ASSERT(nullptr != drawer, "Set active drawer set");
		RMX_ASSERT(drawer->getType() == Drawer::Type::OPENGL, "Expected OpenGL drawer");
		return static_cast<OpenGLDrawer*>(drawer)->getResources();
	}

	Vec4f calculateBlurKernel(float x)
	{
		// Calculate 3x3 blur kernel, represented by only 4 values A, B, C, D
		// (because of symmetry; actually it could even be reduced to 3, as B and C are always the same):
		//    D | B | D
		//    C | A | C
		//    D | B | D

		// The single input value x is a weight factor for the left and right pixel in the underlying 1-dimensional kernel
		//    x | y | x
		// with y chosen so that the sum adds up to 1.

		// From this, we calculate the 3x3 kernel by applying the 1-dim kernel once in x- and once in y-direction, leading to:
		//    x*x | x*y | x*x
		//    y*x | y*y | y*x
		//    x*x | x*y | x*x

		const float y = 1.0f - 2.0f * x;
		const float A = y * y;
		const float B = y * x;
		const float C = x * y;
		const float D = x * x;
		return Vec4f(A, B, C, D);
	}

	const Vec4f& getBlurKernel(int blurValue)
	{
		static const Vec4f BLUR_KERNELS[] =
		{
			Vec4f(1.0f, 0.0f, 0.0f, 0.0f),
			calculateBlurKernel(16.0f / 256.0f),	// These are the same weights as used in "SoftwareBlur::blurBitmap"
			calculateBlurKernel(32.0f / 256.0f),
			calculateBlurKernel(48.0f / 256.0f),
			calculateBlurKernel(64.0f / 256.0f)
		};
		return BLUR_KERNELS[blurValue % 5];
	}
}


struct OpenGLRenderer::Internal
{
	SimpleCopyScreenShader		mSimpleCopyScreenShader;
	SimpleRectOverdrawShader	mSimpleRectOverdrawShader;
	PostFXBlurShader			mPostFxBlurShader;
	RenderPlaneShader			mRenderPlaneShader[RenderPlaneShader::_NUM_VARIATIONS][2];	// Using RenderPlaneShader::Variation enumeration, and alpha test off/on for second index
	RenderVdpSpriteShader		mRenderVdpSpriteShader;
	RenderPaletteSpriteShader	mRenderPaletteSpriteShader[2];		// Two variations: With or without alpha test
	RenderComponentSpriteShader mRenderComponentSpriteShader[2];
	DebugDrawPlaneShader		mDebugDrawPlaneShader;
};


OpenGLRenderer::OpenGLRenderer(RenderParts& renderParts, DrawerTexture& outputTexture) :
	Renderer(RENDERER_TYPE_ID, renderParts, outputTexture),
	mDrawerResources(getDrawerResources()),
	mRenderResources(renderParts, mDrawerResources),
	mInternal(*new Internal())
{
}

OpenGLRenderer::~OpenGLRenderer()
{
	delete &mInternal;
}

void OpenGLRenderer::initialize()
{
	mGameResolution = Configuration::instance().mGameScreen;

	mRenderResources.initialize();

	mGameScreenDepth.create(rmx::OpenGLHelper::FORMAT_DEPTH, mGameResolution.x, mGameResolution.y);

	mGameScreenBuffer.create();
	mGameScreenBuffer.attachTexture(GL_COLOR_ATTACHMENT0, mGameScreenTexture.getImplementation<OpenGLDrawerTexture>()->getTextureHandle(), GL_TEXTURE_2D);
	mGameScreenBuffer.attachRenderbuffer(GL_DEPTH_ATTACHMENT, mGameScreenDepth.getHandle());
	mGameScreenBuffer.finishCreation();
	mGameScreenBuffer.unbind();

	mProcessingTexture.setup(mGameResolution, rmx::OpenGLHelper::FORMAT_RGB);

	mProcessingBuffer.create();
	mProcessingBuffer.attachTexture(GL_COLOR_ATTACHMENT0, mProcessingTexture.getHandle(), GL_TEXTURE_2D);
	mProcessingBuffer.finishCreation();
	mProcessingBuffer.unbind();

	mInternal.mSimpleCopyScreenShader.initialize();
	mInternal.mSimpleRectOverdrawShader.initialize();
	mInternal.mPostFxBlurShader.initialize();

	for (int i = 0; i < RenderPlaneShader::_NUM_VARIATIONS; ++i)
	{
		for (int k = 0; k < 2; ++k)
		{
			mInternal.mRenderPlaneShader[i][k].initialize((RenderPlaneShader::Variation)i, k != 0);
		}
	}
	mInternal.mRenderVdpSpriteShader.initialize();
	for (int k = 0; k < 2; ++k)
	{
		mInternal.mRenderPaletteSpriteShader[k].initialize(k == 1);
		mInternal.mRenderComponentSpriteShader[k].initialize(k == 1);
	}

#if !defined(PLATFORM_VITA)
	mInternal.mDebugDrawPlaneShader.initialize();
#endif

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);		// Corresponds to -1.0f inside the depth range [-1.0f, 1.0f]
	glDepthRange(0.0f, 1.0f);
}

void OpenGLRenderer::reset()
{
	clearFullscreenBuffers(mGameScreenBuffer, mProcessingBuffer);
	mRenderResources.clearAllCaches();
	mDrawerResources.clearAllCaches();
}

void OpenGLRenderer::setGameResolution(const Vec2i& gameResolution)
{
	if (mGameResolution != gameResolution)
	{
		mGameResolution = gameResolution;

		mGameScreenBuffer.setSize(mGameResolution.x, mGameResolution.y);
		mGameScreenDepth.create(rmx::OpenGLHelper::FORMAT_DEPTH, mGameResolution.x, mGameResolution.y);

		mProcessingBuffer.setSize(mGameResolution.x, mGameResolution.y);
		mProcessingTexture.setup(mGameResolution, rmx::OpenGLHelper::FORMAT_RGB);
	}
}

void OpenGLRenderer::clearGameScreen()
{
	clearFullscreenBuffer(mGameScreenBuffer);
}

void OpenGLRenderer::renderGameScreen(const std::vector<Geometry*>& geometries)
{
	internalRefresh();

	// Start the actual rendering
	glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	// Clear the screen
	{
		const Color color = mRenderParts.getPaletteManager().getBackdropColor();
		glDepthMask(GL_TRUE);
		glClearColor(color.r, color.g, color.b, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glDisable(GL_SCISSOR_TEST);
	}

	// We'll use the same quad vertex data over and over again during rendering, so just bind it once
	mDrawerResources.getSimpleQuadVAO().bind();

	// Check if background blur needed
	mIsRenderingToProcessingBuffer = false;
	if (Configuration::instance().mBackgroundBlur > 0)
	{
		for (Geometry* geometry : geometries)
		{
			if (geometry->getType() == Geometry::Type::EFFECT_BLUR)
			{
				// For blur effect, we need to render everything in background into the processing buffer
				mIsRenderingToProcessingBuffer = true;
				glBindFramebuffer(GL_FRAMEBUFFER, mProcessingBuffer.getHandle());
				break;
			}
		}
	}

	// Check if sprite masking needed
	bool usingSpriteMask = false;
	for (Geometry* geometry : geometries)
	{
		if (geometry->getType() == Geometry::Type::SPRITE && static_cast<const SpriteGeometry*>(geometry)->mSpriteInfo.getType() == RenderItem::Type::SPRITE_MASK)
		{
			usingSpriteMask = true;
			break;
		}
	}

	// Render geometries
	mLastRenderedGeometryType = Geometry::Type::UNDEFINED;
	OpenGLShader::resetLastUsedShader();
	{
		uint16 lastRenderQueue = 0xffff;
		for (size_t i = 0; i < geometries.size(); ++i)
		{
			const uint16 renderQueue = geometries[i]->mRenderQueue;
			if (usingSpriteMask && lastRenderQueue < 0x8000 && renderQueue >= 0x8000)
			{
				// Copy planes to processing buffer for sprite masking
				copyGameScreenToProcessingBuffer();
			}

			renderGeometry(*geometries[i]);
			lastRenderQueue = renderQueue;
		}
	}

	// Disable depth test for UI
	glDisable(GL_DEPTH_TEST);
	mDrawerResources.setBlendMode(BlendMode::ALPHA);

	// Unbind shader
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport_Recti(FTX::screenRect());
	glDisable(GL_SCISSOR_TEST);
}

void OpenGLRenderer::renderDebugDraw(int debugDrawMode, const Recti& rect)
{
	// Debug rendering
	if (debugDrawMode <= PlaneManager::PLANE_A)
	{
		const Vec2i playfieldSize = mRenderParts.getPlaneManager().getPlayfieldSizeInPixels();
		glViewport_Recti(RenderUtils::getLetterBoxRect(rect, (float)playfieldSize.x / (float)playfieldSize.y));
	}
	else
	{
		glViewport_Recti(RenderUtils::getLetterBoxRect(rect, 2.0f));
	}

	mInternal.mDebugDrawPlaneShader.draw(debugDrawMode, mRenderParts, mRenderResources);
	glViewport_Recti(FTX::screenRect());
}

void OpenGLRenderer::blurGameScreen()
{
	copyGameScreenToProcessingBuffer();

	glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	const Vec2f texelOffset(1.0f / mGameResolution.x, 1.0f / mGameResolution.y);
	const Vec4f kernel(0.8f, 0.08f, 0.02f, 0.005f);		// That's a total of slightly more than one, so the image gets brighter over time
	mInternal.mPostFxBlurShader.draw(mProcessingTexture.getHandle(), texelOffset, kernel);

	glUseProgram(0);	// Unbind shader again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::clearFullscreenBuffer(Framebuffer& buffer)
{
	// Save current frame buffer and viewport
	GLint previousFramebuffer = 0;
	GLint previousViewport[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	glGetIntegerv(GL_VIEWPORT, previousViewport);

	// Now for the actual clearing
	{
		glBindFramebuffer(GL_FRAMEBUFFER, buffer.getHandle());
		glViewport(0, 0, mGameResolution.x, mGameResolution.y);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// Restore frame buffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
	glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void OpenGLRenderer::clearFullscreenBuffers(Framebuffer& buffer1, Framebuffer& buffer2)
{
	// Save current frame buffer and viewport
	GLint previousFramebuffer = 0;
	GLint previousViewport[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
	glGetIntegerv(GL_VIEWPORT, previousViewport);

	// Now for the actual clearing
	{
		glBindFramebuffer(GL_FRAMEBUFFER, buffer1.getHandle());
		glViewport(0, 0, mGameResolution.x, mGameResolution.y);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	{
		glBindFramebuffer(GL_FRAMEBUFFER, buffer2.getHandle());
		glViewport(0, 0, mGameResolution.x, mGameResolution.y);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// Restore frame buffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
	glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void OpenGLRenderer::internalRefresh()
{
	mRenderResources.refresh();
}

void OpenGLRenderer::renderGeometry(const Geometry& geometry)
{
	switch (geometry.getType())
	{
		case Geometry::Type::UNDEFINED:
			break;	// This should never happen anyways

		case Geometry::Type::PLANE:
		{
			const PlaneGeometry& pg = static_cast<const PlaneGeometry&>(geometry);

			if (!mIsRenderingToProcessingBuffer)
			{
				// Write depth only for planes with priority flag
				if (pg.mPriorityFlag)
				{
					glEnable(GL_DEPTH_TEST);	// Needed for depth write to work at all
					glDepthFunc(GL_ALWAYS);		// Depth test never rejects anything
					glDepthMask(GL_TRUE);		// Depth write is enabled
				}
				else
				{
					glDisable(GL_DEPTH_TEST);	// Disabled both depth test and depth write
				}
			}

			mDrawerResources.setBlendMode(BlendMode::OPAQUE);

			// For backmost layer, ignore alpha completely
			const bool useAlphaTest = (pg.mPlaneIndex != 0 || pg.mPriorityFlag);
			mDrawerResources.setBlendMode(BlendMode::ONE_BIT);
			ScrollOffsetsManager& som = mRenderParts.getScrollOffsetsManager();
			const RenderPlaneShader::Variation variation = (pg.mPlaneIndex == PlaneManager::PLANE_W) ? RenderPlaneShader::PS_SIMPLE :
															som.getHorizontalScrollNoRepeat(pg.mScrollOffsets) ? RenderPlaneShader::PS_NO_REPEAT :
															som.getVerticalScrolling() ? RenderPlaneShader::PS_VERTICAL_SCROLLING : RenderPlaneShader::PS_HORIZONTAL_SCROLLING;
			RenderPlaneShader& shader = mInternal.mRenderPlaneShader[variation][useAlphaTest ? 1 : 0];

			shader.draw(pg, mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mRenderParts, mRenderResources);
			mLastRenderedGeometryType = Geometry::Type::PLANE;
			break;
		}

		case Geometry::Type::SPRITE:
		{
			const SpriteGeometry& sg = static_cast<const SpriteGeometry&>(geometry);

			const bool needsRefresh = (mLastRenderedGeometryType != Geometry::Type::SPRITE || mLastRenderedSpriteType != sg.mSpriteInfo.getType());
			if (needsRefresh)
			{
				if (sg.mSpriteInfo.getType() != RenderItem::Type::SPRITE_MASK)
				{
					glEnable(GL_DEPTH_TEST);	// Enable depth test
					glDepthFunc(GL_GEQUAL);		// Lower depth values get rejected
					glDepthMask(GL_FALSE);		// Disable depth write
				}
				else
				{
					glDisable(GL_DEPTH_TEST);	// Disable depth test
				}

				mLastRenderedGeometryType = Geometry::Type::SPRITE;
				mLastRenderedSpriteType = sg.mSpriteInfo.getType();
			}

			switch (sg.mSpriteInfo.getType())
			{
				case RenderItem::Type::VDP_SPRITE:
				{
					const renderitems::VdpSpriteInfo& spriteInfo = static_cast<const renderitems::VdpSpriteInfo&>(sg.mSpriteInfo);
					mDrawerResources.setBlendMode(spriteInfo.mBlendMode);

					RenderVdpSpriteShader& shader = mInternal.mRenderVdpSpriteShader;
					shader.draw(spriteInfo, mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mRenderResources);
					break;
				}

				case RenderItem::Type::PALETTE_SPRITE:
				{
					const renderitems::PaletteSpriteInfo& spriteInfo = static_cast<const renderitems::PaletteSpriteInfo&>(sg.mSpriteInfo);
					if (spriteInfo.mSize.x == 0 || spriteInfo.mSize.y == 0)
					{
						// Do not render sprites that you cannot see. Trying to render a sprite with no size
						// breaks the depth buffer on macOS causing any sprites rendered afterward to not appear.
						break;
					}

					mDrawerResources.setBlendMode(spriteInfo.mBlendMode);
					const bool useAlphaTest = (spriteInfo.mBlendMode != BlendMode::OPAQUE);

					RenderPaletteSpriteShader& shader = mInternal.mRenderPaletteSpriteShader[useAlphaTest ? 1 : 0];
					shader.draw(spriteInfo, mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mRenderResources);
					break;
				}

				case RenderItem::Type::COMPONENT_SPRITE:
				{
					const renderitems::ComponentSpriteInfo& spriteInfo = static_cast<const renderitems::ComponentSpriteInfo&>(sg.mSpriteInfo);
					mDrawerResources.setBlendMode(spriteInfo.mBlendMode);
					const bool useAlphaTest = (spriteInfo.mBlendMode != BlendMode::OPAQUE);

					RenderComponentSpriteShader& shader = mInternal.mRenderComponentSpriteShader[useAlphaTest ? 1 : 0];
					shader.draw(spriteInfo, mGameResolution, mRenderResources);
					break;
				}

				case RenderItem::Type::SPRITE_MASK:
				{
					const renderitems::SpriteMaskInfo& mask = static_cast<const renderitems::SpriteMaskInfo&>(sg.mSpriteInfo);
					mDrawerResources.setBlendMode(BlendMode::OPAQUE);

					mInternal.mSimpleRectOverdrawShader.draw(mProcessingTexture.getHandle(), Recti(mask.mPosition, mask.mSize), mGameResolution);
					break;
				}

				case RenderItem::Type::RECTANGLE:
				case RenderItem::Type::TEXT:
				case RenderItem::Type::VIEWPORT:
				case RenderItem::Type::INVALID:
					break;
			}

			break;
		}

		case Geometry::Type::RECT:
		{
			const RectGeometry& rg = static_cast<const RectGeometry&>(geometry);

			const bool needsRefresh = (mLastRenderedGeometryType != Geometry::Type::RECT);
			if (needsRefresh)
			{
				glDisable(GL_DEPTH_TEST);
				mLastRenderedGeometryType = Geometry::Type::RECT;
			}
			mDrawerResources.setBlendMode(BlendMode::ALPHA);

			SimpleRectColoredShader& shader = mDrawerResources.getSimpleRectColoredShader();
			shader.setup(rg.mRect, mGameResolution, rg.mColor);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			break;
		}

		case Geometry::Type::TEXTURED_RECT:
		{
			const TexturedRectGeometry& tg = static_cast<const TexturedRectGeometry&>(geometry);

			OpenGLDrawerTexture* texture = tg.mDrawerTexture.getImplementation<OpenGLDrawerTexture>();
			if (nullptr == texture)
				break;

			const bool needsRefresh = (mLastRenderedGeometryType != Geometry::Type::TEXTURED_RECT);
			if (needsRefresh)
			{
				glDisable(GL_DEPTH_TEST);
				mLastRenderedGeometryType = Geometry::Type::TEXTURED_RECT;
			}
			mDrawerResources.setBlendMode(BlendMode::ALPHA);

			SimpleRectTexturedShader& shader = mDrawerResources.getSimpleRectTexturedShader(true, true);
			shader.setup(tg.mRect, mGameResolution, texture->getTextureHandle(), tg.mTintColor, tg.mAddedColor);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			break;
		}

		case Geometry::Type::EFFECT_BLUR:
		{
			const EffectBlurGeometry& ebg = static_cast<const EffectBlurGeometry&>(geometry);

			mIsRenderingToProcessingBuffer = false;
			glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());
			mDrawerResources.setBlendMode(BlendMode::OPAQUE);

			const Vec2f texelOffset(1.0f / mGameResolution.x, 1.0f / mGameResolution.y);
			const Vec4f kernel = getBlurKernel(ebg.mBlurValue);
			mInternal.mPostFxBlurShader.draw(mProcessingTexture.getHandle(), texelOffset, kernel);

			mLastRenderedGeometryType = Geometry::Type::UNDEFINED;
			break;
		}

		case Geometry::Type::VIEWPORT:
		{
			const ViewportGeometry& vg = static_cast<const ViewportGeometry&>(geometry);
			glEnable(GL_SCISSOR_TEST);
			glScissor(vg.mRect.x, vg.mRect.y, vg.mRect.width, vg.mRect.height);
			break;
		}
	}

#ifdef DEBUG
	const GLenum err = glGetError();
	RMX_ASSERT(err == GL_NO_ERROR, "OpenGL error: " << (int)err);
#endif
}

void OpenGLRenderer::copyGameScreenToProcessingBuffer()
{
	GLint oldFramebufferHandle = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebufferHandle);

	glBindFramebuffer(GL_FRAMEBUFFER, mProcessingBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	mInternal.mSimpleCopyScreenShader.draw(mGameScreenTexture.getImplementation<OpenGLDrawerTexture>()->getTextureHandle());

	glUseProgram(0);	// Unbind shader again
	glBindFramebuffer(GL_FRAMEBUFFER, oldFramebufferHandle);
}

#endif
