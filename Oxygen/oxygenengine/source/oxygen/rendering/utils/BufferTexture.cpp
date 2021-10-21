/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/utils/BufferTexture.h"


// If buffer textures are not supported, we use normal textures instead
#if !defined(RMX_USE_GLES2)
	#define SUPPORTS_BUFFER_TEXTURES
#endif


bool BufferTexture::supportsBufferTextures()
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	return true;
#else
	return false;
#endif
}

BufferTexture::BufferTexture()
{
}

BufferTexture::~BufferTexture()
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	if (mTexBuffer != (GLuint)~0)
	{
		glDeleteBuffers(1, &mTexBuffer);
	}
	if (mTextureHandle != 0)
	{
		glDeleteTextures(1, &mTextureHandle);
	}
#endif
}

void BufferTexture::create(PixelFormat pixelFormat, int width, int height, const void* data)
{
	// Only 1 or 2 bytes supported at the moment
	mPixelFormat = pixelFormat;

	if (mTextureHandle == 0)
	{
		glGenTextures(1, &mTextureHandle);
	}

#if defined(SUPPORTS_BUFFER_TEXTURES)
	glGenBuffers(1, &mTexBuffer);
	if (width > 0 && height > 0)
	{
		bufferData(data, width, height);
	}

	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_R8UI : (mPixelFormat == PixelFormat::INT_16) ? GL_R16I : GL_R16UI;
	glBindTexture(GL_TEXTURE_BUFFER, mTextureHandle);
	glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, mTexBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
#else
	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_LUMINANCE : GL_LUMINANCE_ALPHA;
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void BufferTexture::bindBuffer() const
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
#else
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
#endif
}

void BufferTexture::unbindBuffer()
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
#else
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void BufferTexture::bindTexture() const
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindTexture(GL_TEXTURE_BUFFER, mTextureHandle);
#else
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
#endif
}

void BufferTexture::bufferData(const void* data, int width, int height)
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	const int bytesPerPixel = (mPixelFormat == PixelFormat::UINT_8) ? 1 : 2;
	glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
	glBufferData(GL_TEXTURE_BUFFER, width * height * bytesPerPixel, data, GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
#else
	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_LUMINANCE : GL_LUMINANCE_ALPHA;
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}
