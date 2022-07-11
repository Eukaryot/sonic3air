/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>

class RenderParts;
class OpenGLRenderResources;


class DebugDrawPlaneShader
{
public:
	void initialize();
	void draw(int planeIndex, RenderParts& renderParts, const OpenGLRenderResources& resources);

private:
	bool mInitialized = false;

	Shader mShader;
	GLuint mLocPlayfieldSize = 0;
	GLuint mLocIndexTex = 0;
	GLuint mLocPatternCacheTex = 0;
	GLuint mLocPaletteTex = 0;
	GLuint mLocHighlightPrio = 0;
};
