/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/RenderPaletteSpriteShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/drawing/opengl/OpenGLSpriteTextureManager.h"
#include "oxygen/helper/FileHelper.h"


void RenderPaletteSpriteShader::initialize(bool alphaTest)
{
	const std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";
	if (FileHelper::loadShader(mShader, L"data/shader/render_sprite_palette.shader", alphaTest ? "Standard_AlphaTest" : "Standard", additionalDefines))
	{
		bindShader();

		mLocGameResolution	= mShader.getUniformLocation("GameResolution");
		mLocWaterLevel		= mShader.getUniformLocation("WaterLevel");
		mLocPosition		= mShader.getUniformLocation("Position");
		mLocPivotOffset		= mShader.getUniformLocation("PivotOffset");
		mLocSize			= mShader.getUniformLocation("Size");
		mLocTransformation	= mShader.getUniformLocation("Transformation");
		mLocAtex			= mShader.getUniformLocation("Atex");
		mLocTintColor		= mShader.getUniformLocation("TintColor");
		mLocAddedColor		= mShader.getUniformLocation("AddedColor");

		mShader.setParam("SpriteTexture", 0);
		mShader.setParam("PaletteTexture", 1);
	}
}

void RenderPaletteSpriteShader::draw(const renderitems::PaletteSpriteInfo& spriteInfo, const Vec2i& gameResolution, int waterSurfaceHeight, OpenGLRenderResources& resources)
{
	if (nullptr == spriteInfo.mCacheItem)
		return;

	bindShader();

	// Bind textures
	{
		const BufferTexture* texture = OpenGLSpriteTextureManager::instance().getPaletteSpriteTexture(*spriteInfo.mCacheItem, spriteInfo.mUseUpscaledSprite);
		if (nullptr == texture)
			return;

		// Note that this call can internally bind a texture as well, which can mess with previous texture bindings - so we do this before the bindings below
		const OpenGLTexture& paletteTexture = resources.getPaletteTexture(spriteInfo.mPrimaryPalette, spriteInfo.mSecondaryPalette);

		glActiveTexture(GL_TEXTURE0);
		texture->bindTexture();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, paletteTexture.getHandle());
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
		mShader.setParam(mLocPivotOffset, spriteInfo.mPivotOffset);
		mShader.setParam(mLocSize, spriteInfo.mSize);
		mShader.setParam(mLocTransformation, spriteInfo.mTransformation.mMatrix);
		mShader.setParam(mLocAtex, spriteInfo.mAtex);
		mShader.setParam(mLocTintColor, tintColor);
		mShader.setParam(mLocAddedColor, addedColor);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

#endif
