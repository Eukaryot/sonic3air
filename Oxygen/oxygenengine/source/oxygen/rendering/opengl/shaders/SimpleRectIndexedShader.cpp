/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleRectIndexedShader.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/utils/BufferTexture.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"
#include "oxygen/helper/FileHelper.h"


void SimpleRectIndexedShader::initialize(bool supportsTintColor, const char* techname)
{
	mSupportsTintColor = supportsTintColor;
	if (FileHelper::loadShader(mShader, L"data/shader/simple_rect_indexed.shader", techname))
	{
		bindShader();

		mLocTransform	   = mShader.getUniformLocation("Transform");
		mLocSize		   = mShader.getUniformLocation("Size");
		mLocTintColor	   = mShader.getUniformLocation("TintColor");
		mLocAddedColor	   = mShader.getUniformLocation("AddedColor");

		mShader.setParam("MainTexture", 0);
		mShader.setParam("PaletteTexture", 1);
	}
}

void SimpleRectIndexedShader::setup(const BufferTexture& texture, const OpenGLTexture& paletteTexture, const Vec4f& transform, const Color& tintColor, const Color& addedColor)
{
	bindShader();

	// Bind textures
	{
		glActiveTexture(GL_TEXTURE0);
		texture.bindTexture();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, paletteTexture.getHandle());
	}

	// Update uniforms
	{
		glUniform4fv(mLocTransform, 1, transform.data);
		glUniform2iv(mLocSize, 1, texture.getSize().data);

		if (mSupportsTintColor)
		{
			glUniform4fv(mLocTintColor, 1, tintColor.data);
			glUniform4fv(mLocAddedColor, 1, addedColor.data);
		}
	}
}

#endif
