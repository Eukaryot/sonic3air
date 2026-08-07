/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleRectOverdrawShader.h"
#include "oxygen/helper/FileHelper.h"


void SimpleRectOverdrawShader::initialize()
{
	if (FileHelper::loadShader(mShader, L"data/shader/simple_rect_overdraw.shader", "Standard"))
	{
		bindShader();

		mLocMainTexture = mShader.getUniformLocation("MainTexture");
		mLocRect        = mShader.getUniformLocation("Rect");
	}
}

void SimpleRectOverdrawShader::draw(GLuint textureHandle, const Recti& rect, const Vec2i& gameResolution)
{
	bindShader();

	// Bind textures
	mShader.setTexture(mLocMainTexture, textureHandle, GL_TEXTURE_2D);

	// Update uniforms
	{
		const Rectf rectf((float)rect.x / (float)gameResolution.x,
						  (float)rect.y / (float)gameResolution.y,
						  (float)rect.width / (float)gameResolution.x,
						  (float)rect.height / (float)gameResolution.y);
		mShader.setParam(mLocRect, rectf);
	}

	// Draw
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

#endif
