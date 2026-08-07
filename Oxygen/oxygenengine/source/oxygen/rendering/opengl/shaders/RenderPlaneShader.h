/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/opengl/shaders/OpenGLShader.h"

class RenderParts;
class PlaneGeometry;
class OpenGLRenderResources;


class RenderPlaneShader : public OpenGLShader
{
public:
	enum Variation
	{
		PS_SIMPLE = 0,				// No scroll offsets used, primarily for window plane
		PS_HORIZONTAL_SCROLLING,	// Only horizontal scroll offsets used
		PS_VERTICAL_SCROLLING,		// Horizontal + vertical scroll offsets used
		PS_NO_REPEAT,				// No repeat for horizontal scroll offsets
		_NUM_VARIATIONS
	};

public:
	void initialize(Variation variation);
	void draw(const PlaneGeometry& geometry, const Vec2i& gameResolution, int waterSurfaceHeight, RenderParts& renderParts, const OpenGLRenderResources& resources);

private:
	bool  mHorizontalScrolling = false;
	bool  mVerticalScrolling = false;
	bool  mLastRenderedPlanePriority = false;
	Recti mLastActiveRect;
	Vec2i mLastGameResolution;
	int   mLastPaletteVariant = -1;
	Vec4i mLastPlayfieldSize;

	GLuint mLocActiveRect = 0;
	GLuint mLocGameResolution = 0;
	GLuint mLocPriorityFlag = 0;
	GLuint mLocPaletteOffset = 0;
	GLuint mLocPlayfieldSize = 0;
	GLuint mLocVScrollOffsetBias = 0;
	GLuint mLocScrollOffsetX = 0;
	GLuint mLocScrollOffsetY = 0;
};

#endif
