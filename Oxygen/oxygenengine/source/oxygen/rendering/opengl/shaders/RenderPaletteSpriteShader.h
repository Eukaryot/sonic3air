/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"
#include "oxygen/rendering/parts/RenderItem.h"

class OpenGLDrawer;
class OpenGLRenderResources;


class RenderPaletteSpriteShader : public OpenGLShader
{
public:
	void initialize(bool alphaTest);
	void refresh(const Vec2i& gameResolution, int waterSurfaceHeight, const OpenGLRenderResources& resources);
	void draw(const renderitems::PaletteSpriteInfo& spriteInfo, OpenGLRenderResources& resources);

private:
	bool  mInitialized = false;
	Vec2i mLastGameResolution;
	int   mLastWaterSurfaceHeight = 0;

	Shader mShader;
	GLuint mLocGameResolution = 0;
	GLuint mLocWaterLevel = 0;
	GLuint mLocPaletteTex = 0;
	GLuint mLocSpriteTex = 0;
	GLuint mLocPosition = 0;
	GLuint mLocPivotOffset = 0;
	GLuint mLocSize = 0;
	GLuint mLocTransformation = 0;
	GLuint mLocAtex = 0;
	GLuint mLocTintColor = 0;
	GLuint mLocAddedColor = 0;
};

#endif
