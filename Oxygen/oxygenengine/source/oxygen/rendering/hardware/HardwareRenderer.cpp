/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/hardware/HardwareRenderer.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/simulation/LogDisplay.h"


HardwareRenderer::HardwareRenderer(RenderParts& renderParts, DrawerTexture& outputTexture) :
	Renderer(RENDERER_TYPE_ID, renderParts, outputTexture),
	mResources(renderParts)
{
}

void HardwareRenderer::initialize()
{
	mGameResolution = Configuration::instance().mGameScreen;

	mResources.initialize();

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

	FileHelper::loadShader(mSimpleCopyScreenShader,   L"data/shader/simple_copy_screen.shader", "Standard");
	FileHelper::loadShader(mSimpleRectOverdrawShader, L"data/shader/simple_rect_overdraw.shader", "Standard");
	FileHelper::loadShader(mPostFxBlurShader,         L"data/shader/postfx_blur.shader", "Standard");

	for (int i = 0; i < RenderPlaneShader::_NUM_VARIATIONS; ++i)
	{
		for (int k = 0; k < 2; ++k)
		{
			mRenderPlaneShader[i][k].initialize((RenderPlaneShader::Variation)i, k != 0);
		}
	}
	mRenderVdpSpriteShader.initialize();
	mRenderPaletteSpriteShader.initialize();
	mRenderComponentSpriteShader.initialize();
	mDebugDrawPlaneShader.initialize();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);		// Corresponds to -1.0f inside the depth range [-1.0f, 1.0f]
	glDepthRange(0.0f, 1.0f);
}

void HardwareRenderer::reset()
{
	clearFullscreenBuffer(mGameScreenBuffer);
	clearFullscreenBuffer(mProcessingBuffer);
	mResources.setAllPatternsDirty();
}

void HardwareRenderer::setGameResolution(const Vec2i& gameResolution)
{
	if (mGameResolution != gameResolution)
	{
		mGameResolution = gameResolution;

		mGameScreenBuffer.setSize(mGameResolution.x, mGameResolution.y);
		mGameScreenDepth.create(GL_DEPTH_COMPONENT, mGameResolution.x, mGameResolution.y);

		mProcessingBuffer.setSize(mGameResolution.x, mGameResolution.y);
		mProcessingTexture.setup(mGameResolution, rmx::OpenGLHelper::FORMAT_RGB);
	}
}

void HardwareRenderer::clearGameScreen()
{
	clearFullscreenBuffer(mGameScreenBuffer);
}

void HardwareRenderer::renderGameScreen(const std::vector<Geometry*>& geometries)
{
	internalRefresh();

	// Start the actual rendering
	glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glDisable(GL_SCISSOR_TEST);

	// We'll use the same quad vertex data over and over again during rendering, so just bind it once
	OpenGLDrawerResources::getSimpleQuadVAO().bind();

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
		if (geometry->getType() == Geometry::Type::SPRITE && static_cast<const SpriteGeometry*>(geometry)->mSpriteInfo.getType() == SpriteManager::SpriteInfo::Type::MASK)
		{
			usingSpriteMask = true;
			break;
		}
	}

	// Clear the screen if necessary
	if (!mRenderParts.mLayerRendering[0] || !mRenderParts.getActiveDisplay() || mRenderParts.getEnforceClearScreen())
	{
		const Color color = mRenderParts.getPaletteManager().getBackdropColor();
		glClearColor(color.r, color.g, color.b, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	// Render geometries
	mLastRenderedGeometryType = Geometry::Type::UNDEFINED;
	mLastUsedPlaneShader = nullptr;
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

	// Unbind shader
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport_Recti(FTX::screenRect());
	glDisable(GL_SCISSOR_TEST);
}

void HardwareRenderer::renderDebugDraw(int debugDrawMode, const Recti& rect)
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

	mDebugDrawPlaneShader.draw(debugDrawMode, mRenderParts, mResources);
	glViewport_Recti(FTX::screenRect());
}

void HardwareRenderer::blurGameScreen()
{
	copyGameScreenToProcessingBuffer();

	glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	Shader& shader = mPostFxBlurShader;
	shader.bind();
	shader.setTexture("Texture", mProcessingTexture.getHandle(), GL_TEXTURE_2D);
	shader.setParam("TexelOffset", Vec2f(1.0f / mGameResolution.x, 1.0f / mGameResolution.y));
	shader.setParam("Kernel", Vec4f(0.8f, 0.08f, 0.02f, 0.005f));	// That's a total of slightly more than one, so the image gets brighter over time
	glDrawArrays(GL_TRIANGLES, 0, 6);

	shader.unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void HardwareRenderer::clearFullscreenBuffer(Framebuffer& buffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, buffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport_Recti(FTX::screenRect());
}

void HardwareRenderer::internalRefresh()
{
	mResources.refresh();
}

void HardwareRenderer::renderGeometry(const Geometry& geometry)
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

			// For backmost layer, ignore alpha completely
			const bool useAlphaTest = (pg.mPlaneIndex != 0 || pg.mPriorityFlag);
			ScrollOffsetsManager& som = mRenderParts.getScrollOffsetsManager();
			const RenderPlaneShader::Variation variation = (pg.mPlaneIndex == PlaneManager::PLANE_W) ? RenderPlaneShader::PS_SIMPLE :
															som.getHorizontalScrollNoRepeat(pg.mScrollOffsets) ? RenderPlaneShader::PS_NO_REPEAT :
															som.getVerticalScrolling() ? RenderPlaneShader::PS_VERTICAL_SCROLLING : RenderPlaneShader::PS_HORIZONTAL_SCROLLING;
			RenderPlaneShader& shader = mRenderPlaneShader[variation][useAlphaTest ? 1 : 0];

			if (mLastRenderedGeometryType != Geometry::Type::PLANE || mLastUsedPlaneShader != &shader)
			{
				shader.refresh(mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mResources);
				mLastRenderedGeometryType = Geometry::Type::PLANE;
				mLastUsedPlaneShader = &shader;
			}

			shader.draw(pg, mRenderParts, mResources);
			break;
		}

		case Geometry::Type::SPRITE:
		{
			const SpriteGeometry& sg = static_cast<const SpriteGeometry&>(geometry);

			const bool needsRefresh = (mLastRenderedGeometryType != Geometry::Type::SPRITE || mLastRenderedSpriteType != sg.mSpriteInfo.getType());
			if (needsRefresh)
			{
				if (sg.mSpriteInfo.getType() != SpriteManager::SpriteInfo::Type::MASK)
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
				case SpriteManager::SpriteInfo::Type::VDP:
				{
					const SpriteManager::VdpSpriteInfo& spriteInfo = static_cast<const SpriteManager::VdpSpriteInfo&>(sg.mSpriteInfo);
					RenderVdpSpriteShader& shader = mRenderVdpSpriteShader;
					if (needsRefresh)
					{
						shader.refresh(mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mResources);
					}
					shader.draw(spriteInfo, mResources);
					break;
				}

				case SpriteManager::SpriteInfo::Type::PALETTE:
				{
					const SpriteManager::PaletteSpriteInfo& spriteInfo = static_cast<const SpriteManager::PaletteSpriteInfo&>(sg.mSpriteInfo);
					RenderPaletteSpriteShader& shader = mRenderPaletteSpriteShader;
					if (needsRefresh)
					{
						shader.refresh(mGameResolution, mRenderParts.getPaletteManager().mSplitPositionY, mResources);
					}
					if (spriteInfo.mSize.x == 0 || spriteInfo.mSize.y == 0)
					{
						// Do not render sprites that you cannot see. Trying to render a sprite with no size
						// breaks the depth buffer on macOS causing any sprites rendered afterward to not appear.
						break;
					}
					shader.draw(spriteInfo, mResources);
					break;
				}

				case SpriteManager::SpriteInfo::Type::COMPONENT:
				{
					const SpriteManager::ComponentSpriteInfo& spriteInfo = static_cast<const SpriteManager::ComponentSpriteInfo&>(sg.mSpriteInfo);
					RenderComponentSpriteShader& shader = mRenderComponentSpriteShader;
					if (needsRefresh)
					{
						shader.refresh(mGameResolution, mResources);
					}
					shader.draw(spriteInfo, mResources);
					break;
				}

				case SpriteManager::SpriteInfo::Type::MASK:
				{
					const SpriteManager::SpriteMaskInfo& mask = static_cast<const SpriteManager::SpriteMaskInfo&>(sg.mSpriteInfo);
					const Vec4f rectf((float)mask.mPosition.x / (float)mGameResolution.x,
									  (float)mask.mPosition.y / (float)mGameResolution.y,
									  (float)mask.mSize.x / (float)mGameResolution.x,
									  (float)mask.mSize.y / (float)mGameResolution.y);

					Shader& shader = mSimpleRectOverdrawShader;
					shader.bind();
					shader.setParam("Rect", rectf);
					shader.setTexture("Texture", mProcessingTexture.getHandle(), GL_TEXTURE_2D);
					glDrawArrays(GL_TRIANGLES, 0, 6);
					break;
				}

				case SpriteManager::SpriteInfo::Type::INVALID:
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
			}

			Vec4f transform;
			transform.x = (float)rg.mRect.x / (float)mGameResolution.x * 2.0f - 1.0f;
			transform.y = (float)rg.mRect.y / (float)mGameResolution.y * 2.0f - 1.0f;
			transform.z = rg.mRect.width / (float)mGameResolution.x * 2.0f;
			transform.w = rg.mRect.height / (float)mGameResolution.y * 2.0f;

			Shader& shader = OpenGLDrawerResources::getSimpleRectColoredShader();
			shader.bind();
			shader.setParam("Color", Vec4f(rg.mColor.data));
			shader.setParam("Transform", transform);
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
			}

			Vec4f transform;
			transform.x = (float)tg.mRect.x / (float)mGameResolution.x * 2.0f - 1.0f;
			transform.y = (float)tg.mRect.y / (float)mGameResolution.y * 2.0f - 1.0f;
			transform.z = tg.mRect.width / (float)mGameResolution.x * 2.0f;
			transform.w = tg.mRect.height / (float)mGameResolution.y * 2.0f;

			Shader& shader = OpenGLDrawerResources::getSimpleRectTexturedShader(false, false);
			shader.bind();
			shader.setParam("Transform", transform);
			shader.setTexture("Texture", texture->getTextureHandle(), GL_TEXTURE_2D);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			break;
		}

		case Geometry::Type::EFFECT_BLUR:
		{
			const EffectBlurGeometry& ebg = static_cast<const EffectBlurGeometry&>(geometry);

			static const Vec4f BLUR_KERNELS[] =
			{
				Vec4f(1.0f, 0.0f,  0.0f,  0.0f),
				Vec4f(0.8f, 0.04f, 0.04f, 0.01f),
				Vec4f(0.6f, 0.08f, 0.08f, 0.02f),
				Vec4f(0.4f, 0.12f, 0.12f, 0.03f),
				Vec4f(0.2f, 0.15f, 0.15f, 0.05f)
			};

			mIsRenderingToProcessingBuffer = false;
			glBindFramebuffer(GL_FRAMEBUFFER, mGameScreenBuffer.getHandle());

			Shader& shader = mPostFxBlurShader;
			shader.bind();
			shader.setTexture("Texture", mProcessingTexture.getHandle(), GL_TEXTURE_2D);
			shader.setParam("TexelOffset", Vec2f(1.0f / mGameResolution.x, 1.0f / mGameResolution.y));
			shader.setParam("Kernel", BLUR_KERNELS[ebg.mBlurValue % 5]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			shader.unbind();

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
}

void HardwareRenderer::copyGameScreenToProcessingBuffer()
{
	GLint oldFramebufferHandle = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebufferHandle);

	glBindFramebuffer(GL_FRAMEBUFFER, mProcessingBuffer.getHandle());
	glViewport(0, 0, mGameResolution.x, mGameResolution.y);

	Shader& shader = mSimpleCopyScreenShader;
	shader.bind();
	shader.setTexture("Texture", mGameScreenTexture.getImplementation<OpenGLDrawerTexture>()->getTextureHandle(), GL_TEXTURE_2D);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	shader.unbind();
	glBindFramebuffer(GL_FRAMEBUFFER, oldFramebufferHandle);
}
