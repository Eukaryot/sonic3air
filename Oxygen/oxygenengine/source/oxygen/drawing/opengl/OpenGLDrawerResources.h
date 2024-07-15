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

class OpenGLTexture;
class PaletteBase;
class SimpleRectColoredShader;
class SimpleRectVertexColorShader;
class SimpleRectTexturedShader;
class SimpleRectTexturedUVShader;
class SimpleRectIndexedShader;


class OpenGLDrawerResources final
{
public:
	static void startup();
	static void shutdown();

	static SimpleRectColoredShader& getSimpleRectColoredShader();
	static SimpleRectVertexColorShader& getSimpleRectVertexColorShader();
	static SimpleRectTexturedShader& getSimpleRectTexturedShader(bool tint, bool alpha);
	static SimpleRectTexturedUVShader& getSimpleRectTexturedUVShader(bool tint, bool alpha);
	static SimpleRectIndexedShader& getSimpleRectIndexedShader(bool tint, bool alpha);

	static opengl::VertexArrayObject& getSimpleQuadVAO();

	static BlendMode getBlendMode();
	static void setBlendMode(BlendMode blendMode);

	static const OpenGLTexture& getCustomPaletteTexture(const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette);
};

#endif
