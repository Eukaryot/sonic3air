/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/Renderer.h"
#include "oxygen/rendering/Geometry.h"
#include "oxygen/rendering/opengl/OpenGLRenderResources.h"
#include "oxygen/rendering/opengl/shaders/DebugDrawPlaneShader.h"
#include "oxygen/rendering/opengl/shaders/RenderPlaneShader.h"
#include "oxygen/rendering/opengl/shaders/RenderVdpSpriteShader.h"
#include "oxygen/rendering/opengl/shaders/RenderPaletteSpriteShader.h"
#include "oxygen/rendering/opengl/shaders/RenderComponentSpriteShader.h"
#include "oxygen/rendering/parts/SpriteManager.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"


class HardwareRenderer : public Renderer
{
public:
	static constexpr int8 RENDERER_TYPE_ID = 0x20;

public:
	HardwareRenderer(RenderParts& renderParts, DrawerTexture& outputTexture);

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
	HardwareRenderResources mResources;

	Vec2i mGameResolution;

	// Buffers & textures
	Framebuffer	  mGameScreenBuffer;
	Renderbuffer  mGameScreenDepth;
	Framebuffer   mProcessingBuffer;
	OpenGLTexture mProcessingTexture;

	// Shaders
	Shader						mSimpleCopyScreenShader;
	Shader						mSimpleRectOverdrawShader;
	Shader						mPostFxBlurShader;
	RenderPlaneShader			mRenderPlaneShader[RenderPlaneShader::_NUM_VARIATIONS][2];	// Using RenderPlaneShader::Variation enumeration, and alpha test off/on for second index
	RenderVdpSpriteShader		mRenderVdpSpriteShader;
	RenderPaletteSpriteShader	mRenderPaletteSpriteShader;
	RenderComponentSpriteShader mRenderComponentSpriteShader;
	DebugDrawPlaneShader		mDebugDrawPlaneShader;

	// Rendering runtime state
	Geometry::Type mLastRenderedGeometryType = Geometry::Type::UNDEFINED;
	RenderPlaneShader* mLastUsedPlaneShader = nullptr;
	SpriteManager::SpriteInfo::Type mLastRenderedSpriteType = SpriteManager::SpriteInfo::Type::INVALID;
	bool mIsRenderingToProcessingBuffer = false;
};
