/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"


class SimpleRectTexturedShader : public OpenGLShader
{
public:
	void initialize(bool supportsTintColor, const char* techname);
	void setup(const Recti& rect, const Vec2i& gameResolution, GLuint textureHandle, const Color& tintColor = Color::WHITE, const Color& addedColor = Color::TRANSPARENT);
	void setup(GLuint textureHandle, const Vec4f& transform, const Color& tintColor = Color::WHITE, const Color& addedColor = Color::TRANSPARENT);

	inline Shader& getShader()  { return mShader; }

private:
	bool mSupportsTintColor = false;

	GLuint mLocTransform = 0;
	GLuint mLocTintColor = 0;
	GLuint mLocAddedColor = 0;
	GLuint mLocTexture = 0;
};

#endif
