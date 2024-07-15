/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleRectColoredShader.h"
#include "oxygen/helper/FileHelper.h"


void SimpleRectColoredShader::initialize()
{
	if (FileHelper::loadShader(mShader, L"data/shader/simple_rect_colored.shader", "Standard"))
	{
		bindShader();

		mLocColor	  = mShader.getUniformLocation("Color");
		mLocTransform = mShader.getUniformLocation("Transform");
	}
}

void SimpleRectColoredShader::setup(const Recti& rect, const Vec2i& gameResolution, const Color& color)
{
	Vec4f transform;
	transform.x = (float)rect.x / (float)gameResolution.x * 2.0f - 1.0f;
	transform.y = (float)rect.y / (float)gameResolution.y * 2.0f - 1.0f;
	transform.z = rect.width / (float)gameResolution.x * 2.0f;
	transform.w = rect.height / (float)gameResolution.y * 2.0f;

	setup(color, transform);
}

void SimpleRectColoredShader::setup(const Color& color, const Vec4f& transform)
{
	bindShader();

	// Update uniforms
	glUniform4fv(mLocColor, 1, *Vec4f(color));
	glUniform4fv(mLocTransform, 1, *transform);
}

#endif
