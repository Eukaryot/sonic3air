/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleRectTexturedShader.h"
#include "oxygen/helper/FileHelper.h"


void SimpleRectTexturedShader::initialize(bool supportsTintColor, const char* techname)
{
	mSupportsTintColor = supportsTintColor;
	if (FileHelper::loadShader(mShader, L"data/shader/simple_rect_textured.shader", techname))
	{
		bindShader();

		mLocTransform  = mShader.getUniformLocation("Transform");
		mLocTintColor  = mShader.getUniformLocation("TintColor");
		mLocAddedColor = mShader.getUniformLocation("AddedColor");
		mLocTexture    = mShader.getUniformLocation("Texture");
	}
}

void SimpleRectTexturedShader::setup(const Recti& rect, const Vec2i& gameResolution, GLuint textureHandle, const Color& tintColor, const Color& addedColor)
{
	Vec4f transform;
	transform.x = (float)rect.x / (float)gameResolution.x * 2.0f - 1.0f;
	transform.y = (float)rect.y / (float)gameResolution.y * 2.0f - 1.0f;
	transform.z = rect.width / (float)gameResolution.x * 2.0f;
	transform.w = rect.height / (float)gameResolution.y * 2.0f;

	setup(textureHandle, transform, tintColor, addedColor);
}

void SimpleRectTexturedShader::setup(GLuint textureHandle, const Vec4f& transform, const Color& tintColor, const Color& addedColor)
{
	bindShader();

	// Bind textures
	mShader.setTexture("Texture", textureHandle, GL_TEXTURE_2D);

	// Update uniforms
	glUniform4fv(mLocTransform, 1, transform.data);
	if (mSupportsTintColor)
	{
		glUniform4fv(mLocTintColor, 1, tintColor.data);
		glUniform4fv(mLocAddedColor, 1, addedColor.data);
	}
}

#endif
