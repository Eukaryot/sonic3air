/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleCopyScreenShader.h"
#include "oxygen/helper/FileHelper.h"


void SimpleCopyScreenShader::initialize()
{
	if (FileHelper::loadShader(mShader, L"data/shader/simple_copy_screen.shader", "Standard"))
	{
		bindShader();

		mLocMainTexture = mShader.getUniformLocation("MainTexture");
	}
}

void SimpleCopyScreenShader::draw(GLuint textureHandle)
{
	bindShader();

	// Bind texture
	mShader.setTexture(mLocMainTexture, textureHandle, GL_TEXTURE_2D);

	// Draw
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

#endif
