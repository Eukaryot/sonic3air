/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
#include "oxygen/helper/FileHelper.h"


void RenderPlaneShader::initialize(Variation variation)
{
	mHorizontalScrolling = (variation != PS_SIMPLE);
	mVerticalScrolling = (variation == PS_VERTICAL_SCROLLING);
	const bool noRepeat = (variation == PS_NO_REPEAT);

	const std::string techname = noRepeat ? "HorizontalScrollingNoRepeat" : mHorizontalScrolling ? (mVerticalScrolling ? "HorizontalVerticalScrolling" : "HorizontalScrolling") : (mVerticalScrolling ? "VerticalScrolling" : "Standard");
	std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";

	// Always add ALPHA_TEST, there's no variant without
	additionalDefines = (additionalDefines.empty() ? std::string() : (additionalDefines + ",")) + "ALPHA_TEST";

	if (FileHelper::loadShader(mShader, L"data/shader/render_plane.shader", techname, additionalDefines))
	{
		bindShader();

		mLocActiveRect		= mShader.getUniformLocation("ActiveRect");
		mLocGameResolution	= mShader.getUniformLocation("GameResolution");
		mLocPriorityFlag	= mShader.getUniformLocation("PriorityFlag");
		mLocPaletteOffset	= mShader.getUniformLocation("PaletteOffset");
		mLocPlayfieldSize	= mShader.getUniformLocation("PlayfieldSize");

		mShader.setParam("PatternCacheTexture", 0);
		mShader.setParam("PaletteTexture", 1);
		mShader.setParam("IndexTexture", 2);

		if (mHorizontalScrolling)
		{
			mShader.setParam("HScrollOffsetsTexture", 3);
		}
		else
		{
			mLocScrollOffsetX = mShader.getUniformLocation("ScrollOffsetX");
		}

		if (mVerticalScrolling)
		{
			mShader.setParam("VScrollOffsetsTexture", 4);
			mLocVScrollOffsetBias = mShader.getUniformLocation("VScrollOffsetBias");
		}
		else
		{
			mLocScrollOffsetY = mShader.getUniformLocation("ScrollOffsetY");
		}
	}
}

void RenderPlaneShader::draw(const PlaneGeometry& geometry, const Vec2i& gameResolution, int waterSurfaceHeight, RenderParts& renderParts, const OpenGLRenderResources& resources)
{
	bindShader();

	// Bind textures incl. scroll offsets
	{
		glActiveTexture(GL_TEXTURE0);
		resources.getPatternCacheTexture().bindTexture();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, resources.getMainPaletteTexture().getHandle());

		glActiveTexture(GL_TEXTURE2);
		resources.getPlanePatternsTexture(geometry.mPlaneIndex).bindTexture();

		if (mHorizontalScrolling)
		{
			glActiveTexture(GL_TEXTURE3);
			resources.getHScrollOffsetsTexture(geometry.mScrollOffsets).bindTexture();
		}
		else
		{
			if (geometry.mPlaneIndex == PlaneManager::PLANE_W)		// Special handling required here
			{
				mShader.setParam(mLocScrollOffsetX, renderParts.getScrollOffsetsManager().getPlaneWScrollOffset().x);
			}
			else
			{
				mShader.setParam(mLocScrollOffsetX, renderParts.getScrollOffsetsManager().getScrollOffsetsH(geometry.mScrollOffsets)[0]);
			}
		}

		if (mVerticalScrolling)
		{
			glActiveTexture(GL_TEXTURE4);
			resources.getVScrollOffsetsTexture(geometry.mScrollOffsets).bindTexture();

			mShader.setParam(mLocVScrollOffsetBias, renderParts.getScrollOffsetsManager().getVerticalScrollOffsetBias());
		}
		else
		{
			if (geometry.mPlaneIndex == PlaneManager::PLANE_W)		// Special handling required here
			{
				mShader.setParam(mLocScrollOffsetY, renderParts.getScrollOffsetsManager().getPlaneWScrollOffset().y);
			}
			else
			{
				mShader.setParam(mLocScrollOffsetY, renderParts.getScrollOffsetsManager().getScrollOffsetsV(geometry.mScrollOffsets)[0]);
			}
		}
	}

	// Update uniforms
	{
		if (mLastGameResolution != gameResolution)
		{
			mShader.setParam(mLocGameResolution, gameResolution);
			mLastGameResolution = gameResolution;
		}

		const Vec4i playfieldSize = (geometry.mPlaneIndex <= PlaneManager::PLANE_A) ? renderParts.getPlaneManager().getPlayfieldSizeForShaders() : Vec4i(512, 256, 64, 32);
		if (mLastPlayfieldSize != playfieldSize)
		{
			mShader.setParam(mLocPlayfieldSize, playfieldSize);
			mLastPlayfieldSize = playfieldSize;
		}

		if (mLastRenderedPlanePriority != geometry.mPriorityFlag)
		{
			mShader.setParam(mLocPriorityFlag, geometry.mPriorityFlag ? 1 : 0);
			mLastRenderedPlanePriority = geometry.mPriorityFlag;
		}
	}

	// Handle palette split inside the active rect
	Recti rects[2];
	const int numRects = splitRectY(geometry.mActiveRect, waterSurfaceHeight, rects);
	for (int i = 0; i < numRects; ++i)
	{
		if (mLastActiveRect != rects[i])
		{
			mShader.setParam(mLocActiveRect, rects[i]);
			mLastActiveRect = rects[i];
		}

		const int paletteVariant = i;
		if (mLastPaletteVariant != paletteVariant)
		{
			mShader.setParam(mLocPaletteOffset, (float)paletteVariant / 2.0f);
			mLastPaletteVariant = paletteVariant;
		}

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

#endif
