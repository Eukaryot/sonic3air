/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/opengl/shaders/RenderVdpSpriteShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


void RenderVdpSpriteShader::initialize()
{
	const std::string additionalDefines = BufferTexture::supportsBufferTextures() ? "USE_BUFFER_TEXTURES" : "";
	FileHelper::loadShader(mShader, L"data/shader/render_sprite_vdp.shader", "Standard", additionalDefines);
}

void RenderVdpSpriteShader::refresh(const Vec2i& gameResolution, int waterSurfaceHeight, const OpenGLRenderResources& resources)
{
	mShader.bind();

	if (!mInitialized)
	{
		mLocPatternCacheTex	= mShader.getUniformLocation("PatternCacheTexture");
		mLocGameResolution	= mShader.getUniformLocation("GameResolution");
		mLocWaterLevel		= mShader.getUniformLocation("WaterLevel");
		mLocPaletteTex		= mShader.getUniformLocation("PaletteTexture");
		mLocPosition		= mShader.getUniformLocation("Position");
		mLocSize			= mShader.getUniformLocation("Size");
		mLocFirstPattern	= mShader.getUniformLocation("FirstPattern");
		mLocTintColor		= mShader.getUniformLocation("TintColor");
		mLocAddedColor		= mShader.getUniformLocation("AddedColor");

		glUniform1i(mLocPatternCacheTex, 0);
		glUniform1i(mLocPaletteTex, 1);
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

void RenderVdpSpriteShader::draw(const SpriteManager::VdpSpriteInfo& spriteInfo, const OpenGLRenderResources& resources)
{
	const PaletteManager& paletteManager = resources.mRenderParts.getPaletteManager();
	Vec4f tintColor = spriteInfo.mTintColor;
	Vec4f addedColor = spriteInfo.mAddedColor;
	if (spriteInfo.mUseGlobalComponentTint)
	{
		tintColor.r *= paletteManager.getGlobalComponentTintColor().r;
		tintColor.g *= paletteManager.getGlobalComponentTintColor().g;
		tintColor.b *= paletteManager.getGlobalComponentTintColor().b;
		tintColor.a *= paletteManager.getGlobalComponentTintColor().a;
		addedColor += paletteManager.getGlobalComponentAddedColor();
	}

	glUniform3iv(mLocPosition, 1, *Vec3i(spriteInfo.mInterpolatedPosition.x, spriteInfo.mInterpolatedPosition.y, spriteInfo.mPriorityFlag ? 1 : 0));
	glUniform2iv(mLocSize, 1, *spriteInfo.mSize);
	glUniform1i(mLocFirstPattern, spriteInfo.mFirstPattern);
	glUniform4fv(mLocTintColor, 1, tintColor.data);
	glUniform4fv(mLocAddedColor, 1, addedColor.data);
	glUniform4fv(mLocTintColor, 1, spriteInfo.mTintColor.data);
	glUniform4fv(mLocAddedColor, 1, spriteInfo.mAddedColor.data);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}
