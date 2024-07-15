/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/SimpleRectVertexColorShader.h"
#include "oxygen/helper/FileHelper.h"


void SimpleRectVertexColorShader::initialize()
{
	if (FileHelper::loadShader(mShader, L"data/shader/simple_rect_vertexcolor.shader", "Standard"))
	{
		bindShader();

		mLocTransform = mShader.getUniformLocation("Transform");
	}
}

void SimpleRectVertexColorShader::setup(const Vec4f& transform)
{
	bindShader();

	// Update uniforms
	glUniform4fv(mLocTransform, 1, *transform);
}

#endif
