/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/Renderer.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/parts/SpriteManager.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"

class OpenGLDrawerResources;


class OpenGLRenderer : public Renderer
{
public:
	static constexpr int8 RENDERER_TYPE_ID = 0x20;

public:
	OpenGLRenderer(RenderParts& renderParts, DrawerTexture& outputTexture);
	~OpenGLRenderer();

	virtual void initialize() override;
	virtual void reset() override;
	virtual void setGameResolution(const Vec2i& gameResolution) override;
	virtual void clearGameScreen() override;
	virtual void renderGameScreen(const std::vector<Geometry*>& geometries) override;
	virtual void renderDebugDraw(int debugDrawMode, const Recti& rect) override;

	void blurGameScreen();

private:
	void clearFullscreenBuffer(Framebuffer& buffer);
	void clearFullscreenBuffers(Framebuffer& buffer1, Framebuffer& buffer2);
	void internalRefresh();
	void renderGeometry(const Geometry& geometry);
	void copyGameScreenToProcessingBuffer();

private:
	OpenGLDrawerResources& mDrawerResources;
	OpenGLRenderResources mRenderResources;

	struct Internal;
	Internal& mInternal;

	Vec2i mGameResolution;

	// Buffers & textures
	Framebuffer	  mGameScreenBuffer;
	Renderbuffer  mGameScreenDepth;
	Framebuffer   mProcessingBuffer;
	OpenGLTexture mProcessingTexture;

	// Rendering runtime state
	Geometry::Type mLastRenderedGeometryType = Geometry::Type::UNDEFINED;
	RenderItem::Type mLastRenderedSpriteType = RenderItem::Type::INVALID;
	bool mIsRenderingToProcessingBuffer = false;
};

#endif
