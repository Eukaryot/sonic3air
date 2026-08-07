/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "rmxmedia_externals.h"


namespace rmx
{
	class OpenGLHelper
	{
	public:
	#ifdef RMX_USE_GLES2
		static const constexpr GLint FORMAT_RGB   = GL_RGBA;				// OpenGL ES 2.0 does not have GL_RGB, so we have to use GL_RGBA instead (and just don't use the alpha channel)
		static const constexpr GLint FORMAT_RGBA  = GL_RGBA;
		static const constexpr GLint FORMAT_DEPTH = GL_DEPTH_COMPONENT16;	// OpenGL ES 2.0 does not have the more general GL_DEPTH_COMPONENT, only GL_DEPTH_COMPONENT16, which is fine for us as well
	#else
		static const constexpr GLint FORMAT_RGB   = GL_RGB8;
		static const constexpr GLint FORMAT_RGBA  = GL_RGBA8;
		static const constexpr GLint FORMAT_DEPTH = GL_DEPTH_COMPONENT;
	#endif
	};
}

#endif
