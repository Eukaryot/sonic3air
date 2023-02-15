/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/RenderPlaneShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


void RenderPlaneShader::initialize(Variation variation, bool alphaTest)
{
	switch (variation)
	{
		case PS_SIMPLE:				  initialize(false, false, false, alphaTest);  break;	// No scroll offsets used, primarily for window plane
		case PS_HORIZONTAL_SCROLLING: initialize(true,  false, false, alphaTest);  break;	// Only horizontal scroll offsets used
		case PS_VERTICAL_SCROLLING:	  initialize(true,  true,  false, alphaTest);  break;	// Horizontal + vertical scroll offsets used
		case PS_NO_REPEAT:			  initialize(true,  false, true,  alphaTest);  break;	// No repeat for horizontal scroll offsets
		default:
			RMX_ASSERT(false, "Unrecognized render plane shader variation " << variation);
	}
}

void RenderPlaneShader::initialize(bool horizontalScrolling, bool verticalScrolling, bool noRepeat, bool alphaTest)
{
	mHorizontalScrolling = horizontalScrolling;
	mVerticalScrolling = verticalScrolling;

	std::string techname = noRepeat ? "HorizontalScrollingNoRepeat" : horizontalScrolling ? (verticalScrolling ? "HorizontalVerticalScrolling" : "HorizontalScrolling") : (verticalScrolling ? "VerticalScrolling" : "Standard");
	std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";
	if (alphaTest)
		additionalDefines = (additionalDefines.empty() ? std::string() : (additionalDefines + ",")) + "ALPHA_TEST";
	FileHelper::loadShader(mShader, L"data/shader/render_plane.shader", techname, additionalDefines);
}

void RenderPlaneShader::refresh(const Vec2i& gameResolution, const OpenGLRenderResources& resources)
{
	// No alpha blending needed for planes
	glBlendFunc(GL_ONE, GL_ZERO);

	mShader.bind();

	if (!mInitialized)
	{
		mLocActiveRect		= mShader.getUniformLocation("ActiveRect");
		mLocGameResolution	= mShader.getUniformLocation("GameResolution");
		mLocPriorityFlag	= mShader.getUniformLocation("PriorityFlag");
		mLocPaletteOffset	= mShader.getUniformLocation("PaletteOffset");
		mLocPlayfieldSize	= mShader.getUniformLocation("PlayfieldSize");
		mLocPatternCacheTex	= mShader.getUniformLocation("PatternCacheTexture");
		mLocIndexTex		= mShader.getUniformLocation("IndexTexture");
		mLocPaletteTex		= mShader.getUniformLocation("PaletteTexture");

		glUniform1i(mLocPatternCacheTex, 0);
		glUniform1i(mLocPaletteTex, 1);
		glUniform1i(mLocIndexTex, 2);

		if (mHorizontalScrolling)
		{
			mLocHScrollOffsetsTex = mShader.getUniformLocation("HScrollOffsetsTexture");
			glUniform1i(mLocHScrollOffsetsTex, 3);
		}
		else
		{
			mLocScrollOffsetX = mShader.getUniformLocation("ScrollOffsetX");
		}

		if (mVerticalScrolling)
		{
			mLocVScrollOffsetsTex = mShader.getUniformLocation("VScrollOffsetsTexture");
			mLocVScrollOffsetBias = mShader.getUniformLocation("VScrollOffsetBias");
			glUniform1i(mLocVScrollOffsetsTex, 4);
		}
		else
		{
			mLocScrollOffsetY = mShader.getUniformLocation("ScrollOffsetY");
		}
	}

	glActiveTexture(GL_TEXTURE0);
	resources.mPatternCacheTexture.bindTexture();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, resources.mPaletteTexture.getHandle());

	if (mLastGameResolution != gameResolution || !mInitialized)
	{
		glUniform2iv(mLocGameResolution, 1, *gameResolution);
		mLastGameResolution = gameResolution;
	}

	mInitialized = true;
}

void RenderPlaneShader::draw(const PlaneGeometry& geometry, int waterSurfaceHeight, RenderParts& renderParts, const OpenGLRenderResources& resources)
{
	const Vec4i playfieldSize = (geometry.mPlaneIndex <= PlaneManager::PLANE_A) ? renderParts.getPlaneManager().getPlayfieldSizeForShaders() : Vec4i(512, 256, 64, 32);
	if (mLastPlayfieldSize != playfieldSize)
	{
		glUniform4iv(mLocPlayfieldSize, 1, playfieldSize.data);
		mLastPlayfieldSize = playfieldSize;
	}

	if (mLastRenderedPlanePriority != geometry.mPriorityFlag)
	{
		glUniform1i(mLocPriorityFlag, geometry.mPriorityFlag ? 1 : 0);
		mLastRenderedPlanePriority = geometry.mPriorityFlag;
	}

	glActiveTexture(GL_TEXTURE2);
	resources.mPlanePatternsTexture[geometry.mPlaneIndex].bindTexture();

	if (mHorizontalScrolling)
	{
		glActiveTexture(GL_TEXTURE3);
		resources.getHScrollOffsetsTexture(geometry.mScrollOffsets).bindTexture();
	}
	else
	{
		if (geometry.mPlaneIndex == PlaneManager::PLANE_W)		// Special handling required here
		{
			glUniform1i(mLocScrollOffsetX, renderParts.getScrollOffsetsManager().getPlaneWScrollOffset().x);
		}
		else
		{
			glUniform1i(mLocScrollOffsetX, renderParts.getScrollOffsetsManager().getScrollOffsetsH(geometry.mScrollOffsets)[0]);
		}
	}

	if (mVerticalScrolling)
	{
		glActiveTexture(GL_TEXTURE4);
		resources.getVScrollOffsetsTexture(geometry.mScrollOffsets).bindTexture();

		glUniform1i(mLocVScrollOffsetBias, renderParts.getScrollOffsetsManager().getVerticalScrollOffsetBias());
	}
	else
	{
		if (geometry.mPlaneIndex == PlaneManager::PLANE_W)		// Special handling required here
		{
			glUniform1i(mLocScrollOffsetY, renderParts.getScrollOffsetsManager().getPlaneWScrollOffset().y);
		}
		else
		{
			glUniform1i(mLocScrollOffsetY, renderParts.getScrollOffsetsManager().getScrollOffsetsV(geometry.mScrollOffsets)[0]);
		}
	}

	// Handle palette split inside the active rect
	Recti rects[2];
	const int numRects = splitRectY(geometry.mActiveRect, waterSurfaceHeight, rects);
	for (int i = 0; i < numRects; ++i)
	{
		if (mLastActiveRect != rects[i])
		{
			glUniform4iv(mLocActiveRect, 1, rects[i].mData);
			mLastActiveRect = rects[i];
		}

		const int paletteVariant = i;
		if (mLastPaletteVariant != paletteVariant || !mInitialized)
		{
			const float paletteOffset = (float)(paletteVariant + 0.5f) / 2.0f;
			glUniform1f(mLocPaletteOffset, paletteOffset);
			mLastPaletteVariant = paletteVariant;
		}

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

#endif
