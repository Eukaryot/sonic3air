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


class OpenGLDrawerResources final
{
public:
	static void startup();
	static void shutdown();

	static Shader& getSimpleRectColoredShader();
	static Shader& getSimpleRectVertexColorShader();
	static Shader& getSimpleRectTexturedShader(bool tint, bool alpha);
	static Shader& getSimpleRectTexturedUVShader(bool tint, bool alpha);

	static opengl::VertexArrayObject& getSimpleQuadVAO();

	static BlendMode getBlendMode();
	static void setBlendMode(BlendMode blendMode);
};

#endif
