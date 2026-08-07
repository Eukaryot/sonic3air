/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/RenderVdpSpriteShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/helper/FileHelper.h"


void RenderVdpSpriteShader::initialize()
{
	const std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";
	if (FileHelper::loadShader(mShader, L"data/shader/render_sprite_vdp.shader", "Standard", additionalDefines))
	{
		bindShader();

		mLocGameResolution = mShader.getUniformLocation("GameResolution");
		mLocWaterLevel	   = mShader.getUniformLocation("WaterLevel");
		mLocPosition	   = mShader.getUniformLocation("Position");
		mLocSize		   = mShader.getUniformLocation("Size");
		mLocFirstPattern   = mShader.getUniformLocation("FirstPattern");
		mLocTintColor	   = mShader.getUniformLocation("TintColor");
		mLocAddedColor	   = mShader.getUniformLocation("AddedColor");

		mShader.setParam("PatternCacheTexture", 0);
		mShader.setParam("PaletteTexture", 1);
	}
}

void RenderVdpSpriteShader::draw(const renderitems::VdpSpriteInfo& spriteInfo, const Vec2i& gameResolution, int waterSurfaceHeight, const OpenGLRenderResources& resources)
{
	bindShader();

	// Bind textures
	{
		glActiveTexture(GL_TEXTURE0);
		resources.getPatternCacheTexture().bindTexture();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, resources.getMainPaletteTexture().getHandle());
	}

	// Update uniforms
	{
		if (mLastGameResolution != gameResolution)
		{
			mShader.setParam(mLocGameResolution, gameResolution);
			mLastGameResolution = gameResolution;
		}

		if (mLastWaterSurfaceHeight != waterSurfaceHeight)
		{
			mShader.setParam(mLocWaterLevel, waterSurfaceHeight);
			mLastWaterSurfaceHeight = waterSurfaceHeight;
		}

		const PaletteManager& paletteManager = resources.getRenderParts().getPaletteManager();
		Vec4f tintColor = spriteInfo.mTintColor;
		Vec4f addedColor = spriteInfo.mAddedColor;
		if (spriteInfo.mUseGlobalComponentTint)
		{
			paletteManager.applyGlobalComponentTint(tintColor, addedColor);
		}

		mShader.setParam(mLocPosition, Vec3i(spriteInfo.mInterpolatedPosition.x, spriteInfo.mInterpolatedPosition.y, spriteInfo.mPriorityFlag ? 1 : 0));
		mShader.setParam(mLocSize, spriteInfo.mSize);
		mShader.setParam(mLocFirstPattern, spriteInfo.mFirstPattern);
		mShader.setParam(mLocTintColor, tintColor);
		mShader.setParam(mLocAddedColor, addedColor);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

#endif
