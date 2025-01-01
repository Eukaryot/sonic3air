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


class SimpleRectColoredShader : public OpenGLShader
{
public:
	void initialize();
	void setup(const Recti& rect, const Vec2i& gameResolution, const Color& color);
	void setup(const Color& color, const Vec4f& transform);

private:
	GLuint mLocColor = 0;
	GLuint mLocTransform = 0;
};

#endif
