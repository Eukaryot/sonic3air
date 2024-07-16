/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"


class SimpleRectOverdrawShader : public OpenGLShader
{
public:
	void initialize();
	void draw(GLuint textureHandle, const Recti& rect, const Vec2i& gameResolution);

private:
	GLuint mLocMainTexture = 0;
	GLuint mLocRect = 0;
};

#endif
