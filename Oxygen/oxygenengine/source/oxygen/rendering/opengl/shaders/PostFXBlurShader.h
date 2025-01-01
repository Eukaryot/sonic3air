/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"


class PostFXBlurShader : public OpenGLShader
{
public:
	void initialize();
	void draw(GLuint textureHandle, const Vec2f& texelOffset, const Vec4f& kernel);

private:
	GLuint mLocMainTexture = 0;
	GLuint mLocTexelOffset = 0;
	GLuint mLocKernel = 0;
};

#endif
