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

class BufferTexture;
class OpenGLTexture;


class SimpleRectIndexedShader : public OpenGLShader
{
public:
	void initialize(bool supportsTintColor, const char* techname);
	void setup(const BufferTexture& texture, const OpenGLTexture& paletteTexture, const Vec4f& transform, const Color& tintColor = Color::WHITE, const Color& addedColor = Color::TRANSPARENT);

private:
	bool mSupportsTintColor = false;

	GLuint mLocTransform = 0;
	GLuint mLocSize = 0;
	GLuint mLocTintColor = 0;
	GLuint mLocAddedColor = 0;
};

#endif
