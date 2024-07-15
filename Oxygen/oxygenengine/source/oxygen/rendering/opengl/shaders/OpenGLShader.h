/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include <rmxmedia.h>


class OpenGLShader
{
public:
	static void resetLastUsedShader();

protected:
	bool bindShader();

	static int splitRectY(const Recti& inputRect, int splitY, Recti* outputRects);

protected:
	static inline OpenGLShader* mLastUsedShader = nullptr;

protected:
	Shader mShader;
};

#endif
