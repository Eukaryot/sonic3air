/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/hardware/shaders/RenderPlaneShader.h"
#include "oxygen/rendering/hardware/HardwareRenderResources.h"
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

void RenderPlaneShader::refresh(const Vec2i& gameResolution, int waterSurfaceHeight, const HardwareRenderResources& resources)
{
	// No alpha blending needed for planes
	glBlendFunc(GL_ONE, GL_ZERO);

	mShader.bind();

	if (!mInitialized)
	{
		mLocActiveRect		  = mShader.getUniformLocation("ActiveRect");
		mLocGameResolution	  = mShader.getUniformLocation("GameResolution");
		mLocPriorityFlag	  = mShader.getUniformLocation("PriorityFlag");
		mLocWaterLevel		  = mShader.getUniformLocation("WaterLevel");
		mLocPlayfieldSize	  = mShader.getUniformLocation("PlayfieldSize");
		mLocPatternCacheTex	  = mShader.getUniformLocation("PatternCacheTexture");
		mLocIndexTex		  = mShader.getUniformLocation("IndexTexture");
		mLocPaletteTex		  = mShader.getUniformLocation("PaletteTexture");

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

	if (mLastWaterSurfaceHeight != waterSurfaceHeight || !mInitialized)
	{
		glUniform1i(mLocWaterLevel, waterSurfaceHeight);
		mLastWaterSurfaceHeight = waterSurfaceHeight;
	}

	mInitialized = true;
}

void RenderPlaneShader::draw(const PlaneGeometry& geometry, RenderParts& renderParts, const HardwareRenderResources& resources)
{
	if (mLastActiveRect != geometry.mActiveRect)
	{
		glUniform4iv(mLocActiveRect, 1, geometry.mActiveRect.mData);
		mLastActiveRect = geometry.mActiveRect;
	}

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

	glDrawArrays(GL_TRIANGLES, 0, 6);
}
