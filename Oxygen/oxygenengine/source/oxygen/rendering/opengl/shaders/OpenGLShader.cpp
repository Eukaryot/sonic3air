/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"


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
