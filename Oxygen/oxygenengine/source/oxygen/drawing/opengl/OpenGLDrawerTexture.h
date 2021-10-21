/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/drawing/DrawCommand.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"


class OpenGLDrawerTexture final : public DrawerTextureImplementation
{
public:
	void updateFromBitmap(const Bitmap& bitmap) override;
	void setupAsRenderTarget(const Vec2i& size, DrawerTexture& owner) override;
	void writeContentToBitmap(Bitmap& outBitmap) override;
	void refreshImplementation(DrawerTexture& owner, bool setupRenderTarget, const Vec2i& size) override;

public:
	inline GLuint getTextureHandle()  { return mTexture.getHandle(); }
	GLuint getFrameBufferHandle();

public:
	DrawerSamplingMode mSamplingMode = DrawerSamplingMode::POINT;
	DrawerWrapMode mWrapMode = DrawerWrapMode::CLAMP;

private:
	OpenGLTexture mTexture;
	Framebuffer mFrameBuffer;	// Only used if this is a render target
};
