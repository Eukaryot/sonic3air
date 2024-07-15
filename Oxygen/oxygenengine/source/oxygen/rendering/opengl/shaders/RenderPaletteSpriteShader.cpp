/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/RenderPaletteSpriteShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
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

		glActiveTexture(GL_TEXTURE0);
		texture->bindTexture();

		if (mLastUsedPrimaryPalette != spriteInfo.mPrimaryPalette || mLastUsedSecondaryPalette != spriteInfo.mSecondaryPalette)
		{
			const OpenGLTexture& paletteTexture = resources.getPaletteTexture(spriteInfo.mPrimaryPalette, spriteInfo.mSecondaryPalette);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, paletteTexture.getHandle());

			mLastUsedPrimaryPalette = spriteInfo.mPrimaryPalette;
			mLastUsedSecondaryPalette = spriteInfo.mSecondaryPalette;
		}
	}

	// Update uniforms
	{
		if (mLastGameResolution != gameResolution)
		{
			glUniform2iv(mLocGameResolution, 1, *gameResolution);
			mLastGameResolution = gameResolution;
		}

		if (mLastWaterSurfaceHeight != waterSurfaceHeight)
		{
			glUniform1i(mLocWaterLevel, waterSurfaceHeight);
			mLastWaterSurfaceHeight = waterSurfaceHeight;
		}

		const PaletteManager& paletteManager = resources.getRenderParts().getPaletteManager();
		Vec4f tintColor = spriteInfo.mTintColor;
		Vec4f addedColor = spriteInfo.mAddedColor;
		if (spriteInfo.mUseGlobalComponentTint)
		{
			tintColor *= paletteManager.getGlobalComponentTintColor();
			addedColor += paletteManager.getGlobalComponentAddedColor();
		}

		glUniform3iv(mLocPosition, 1, *Vec3i(spriteInfo.mInterpolatedPosition.x, spriteInfo.mInterpolatedPosition.y, spriteInfo.mPriorityFlag ? 1 : 0));
		glUniform2iv(mLocPivotOffset, 1, *spriteInfo.mPivotOffset);
		glUniform2iv(mLocSize, 1, *spriteInfo.mSize);
		glUniform4fv(mLocTransformation, 1, *spriteInfo.mTransformation.mMatrix);
		glUniform1i (mLocAtex, spriteInfo.mAtex);
		glUniform4fv(mLocTintColor, 1, tintColor.data);
		glUniform4fv(mLocAddedColor, 1, addedColor.data);
	}

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

#endif
