/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/drawing/DrawCommand.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"


class OpenGLDrawerTexture final : public DrawerTextureImplementation
{
public:
	inline explicit OpenGLDrawerTexture(DrawerTexture& owner) : DrawerTextureImplementation(owner) {}

	void updateFromBitmap(const Bitmap& bitmap) override;
	void setupAsRenderTarget(const Vec2i& size) override;
	void writeContentToBitmap(Bitmap& outBitmap) override;
	void refreshImplementation(bool setupRenderTarget, const Vec2i& size) override;

public:
	inline OpenGLTexture& getTexture()  { return mTexture; }
	inline GLuint getTextureHandle()    { return mTexture.getHandle(); }
	GLuint getFrameBufferHandle();

public:
	SamplingMode mSamplingMode = SamplingMode::POINT;
	TextureWrapMode mWrapMode = TextureWrapMode::CLAMP;

private:
	OpenGLTexture mTexture;
	Framebuffer mFrameBuffer;	// Only used if this is a render target
};

#endif
