/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/opengl/shaders/RenderComponentSpriteShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/drawing/opengl/OpenGLSpriteTextureManager.h"
#include "oxygen/helper/FileHelper.h"


void RenderComponentSpriteShader::initialize(bool alphaTest)
{
	FileHelper::loadShader(mShader, L"data/shader/render_sprite_component.shader", alphaTest ? "Standard_AlphaTest" : "Standard");
}

void RenderComponentSpriteShader::refresh(const Vec2i& gameResolution)
{
	mShader.bind();

	if (!mInitialized)
	{
		mLocGameResolution	= mShader.getUniformLocation("GameResolution");
		mLocSpriteTex		= mShader.getUniformLocation("SpriteTexture");
		mLocPosition		= mShader.getUniformLocation("Position");
		mLocPivotOffset		= mShader.getUniformLocation("PivotOffset");
		mLocSize			= mShader.getUniformLocation("Size");
		mLocTransformation	= mShader.getUniformLocation("Transformation");
		mLocTintColor		= mShader.getUniformLocation("TintColor");
		mLocAddedColor		= mShader.getUniformLocation("AddedColor");

		glUniform1i(mLocSpriteTex, 0);
	}

	if (mLastGameResolution != gameResolution || !mInitialized)
	{
		glUniform2iv(mLocGameResolution, 1, *gameResolution);
		mLastGameResolution = gameResolution;
	}

	mInitialized = true;
}

void RenderComponentSpriteShader::draw(const SpriteManager::ComponentSpriteInfo& spriteInfo, OpenGLRenderResources& resources)
{
	glActiveTexture(GL_TEXTURE0);
	const OpenGLTexture* texture = OpenGLSpriteTextureManager::instance().getComponentSpriteTexture(*spriteInfo.mCacheItem);
	if (nullptr == texture)
		return;

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

	glBindTexture(GL_TEXTURE_2D, texture->getHandle());

	glUniform3iv(mLocPosition, 1, *Vec3i(spriteInfo.mInterpolatedPosition.x, spriteInfo.mInterpolatedPosition.y, spriteInfo.mPriorityFlag ? 1 : 0));
	glUniform2iv(mLocPivotOffset, 1, *spriteInfo.mPivotOffset);
	glUniform2iv(mLocSize, 1, *spriteInfo.mSize);
	glUniform4fv(mLocTransformation, 1, *spriteInfo.mTransformation.mMatrix);
	glUniform4fv(mLocTintColor, 1, tintColor.data);
	glUniform4fv(mLocAddedColor, 1, addedColor.data);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}
