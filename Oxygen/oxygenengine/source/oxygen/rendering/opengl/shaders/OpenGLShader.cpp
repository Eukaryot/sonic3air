/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"


void OpenGLShader::resetLastUsedShader()
{
	mLastUsedShader = nullptr;
}

bool OpenGLShader::bindShader()
{
	if (mLastUsedShader == this)
	{
		mShader.resetTextureCount();	// To ensure that the following calls to "Shader::setTexture" use the correct texture index
		return false;
	}

	mShader.bind();
	mLastUsedShader = this;
	return true;
}

int OpenGLShader::splitRectY(const Recti& inputRect, int splitY, Recti* outputRects)
{
	if (splitY > inputRect.y && splitY < inputRect.y + inputRect.height)
	{
		// Upper part
		outputRects[0] = inputRect;
		outputRects[0].height = splitY - inputRect.y;

		// Lower part
		outputRects[1] = inputRect;
		outputRects[1].y = splitY;
		outputRects[1].height = inputRect.y + inputRect.height - splitY;
		return 2;
	}
	else
	{
		outputRects[0] = inputRect;
		return 1;
	}
}

#endif
