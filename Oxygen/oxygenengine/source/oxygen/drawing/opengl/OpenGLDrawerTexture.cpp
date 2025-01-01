/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"


void OpenGLDrawerTexture::updateFromBitmap(const Bitmap& bitmap)
{
	mTexture.loadBitmap(bitmap);

	// Need to update these, as "loadBitmap" changed the OpenGL texture parameters
	mSamplingMode = SamplingMode::POINT;
	mWrapMode = TextureWrapMode::CLAMP;
}

void OpenGLDrawerTexture::setupAsRenderTarget(const Vec2i& size)
{
	mTexture.setup(size, rmx::OpenGLHelper::FORMAT_RGB);

	if (mFrameBuffer.getHandle() == 0)
	{
		mFrameBuffer.create();
		mFrameBuffer.attachTexture(GL_COLOR_ATTACHMENT0, mTexture.getHandle(), GL_TEXTURE_2D);
		mFrameBuffer.finishCreation();
		mFrameBuffer.unbind();
	}
}

void OpenGLDrawerTexture::writeContentToBitmap(Bitmap& outBitmap)
{
	outBitmap.create(mTexture.getSize().x, mTexture.getSize().y);

#if !defined(RMX_USE_GLES2)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexture.getHandle());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, outBitmap.getData());
	glBindTexture(GL_TEXTURE_2D, 0);
#else
	RMX_ASSERT(false, "Not supported");
#endif
}

void OpenGLDrawerTexture::refreshImplementation(bool setupRenderTarget, const Vec2i& size)
{
	if (setupRenderTarget)
	{
		setupAsRenderTarget(size);
	}
	else
	{
		if (!mOwner.accessBitmap().empty())
		{
			updateFromBitmap(mOwner.accessBitmap());
		}
	}
}

GLuint OpenGLDrawerTexture::getFrameBufferHandle()
{
	return mFrameBuffer.getHandle();
}

#endif
