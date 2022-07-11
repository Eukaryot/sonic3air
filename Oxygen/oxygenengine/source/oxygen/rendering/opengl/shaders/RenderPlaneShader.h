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
class PlaneGeometry;
class OpenGLRenderResources;


class RenderPlaneShader
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
	void initialize(Variation variation, bool alphaTest);
	void initialize(bool horizontalScrolling, bool verticalScrolling, bool noRepeat, bool alphaTest);
	void refresh(const Vec2i& gameResolution, int waterSurfaceHeight, const OpenGLRenderResources& resources);
	void draw(const PlaneGeometry& geometry, RenderParts& renderParts, const OpenGLRenderResources& resources);

private:
	bool  mInitialized = false;
	bool  mHorizontalScrolling = false;
	bool  mVerticalScrolling = false;
	bool  mLastRenderedPlanePriority = false;
	Recti mLastActiveRect;
	Vec2i mLastGameResolution;
	int   mLastWaterSurfaceHeight = 0;
	Vec4i mLastPlayfieldSize;

	Shader mShader;
	GLuint mLocActiveRect = 0;
	GLuint mLocGameResolution = 0;
	GLuint mLocPriorityFlag = 0;
	GLuint mLocWaterLevel = 0;
	GLuint mLocPlayfieldSize = 0;
	GLuint mLocPatternCacheTex = 0;
	GLuint mLocIndexTex = 0;
	GLuint mLocHScrollOffsetsTex = 0;
	GLuint mLocVScrollOffsetsTex = 0;
	GLuint mLocVScrollOffsetBias = 0;
	GLuint mLocScrollOffsetX = 0;
	GLuint mLocScrollOffsetY = 0;
	GLuint mLocPaletteTex = 0;
};
