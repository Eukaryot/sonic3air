/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"


OpenGLTexture::~OpenGLTexture()
{
	if (mTextureHandle != 0)
	{
		glDeleteTextures(1, &mTextureHandle);
	}
}

void OpenGLTexture::loadBitmap(const Bitmap& bitmap)
{
	if (mTextureHandle == 0)
	{
		glGenTextures(1, &mTextureHandle);
	}

	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, rmx::OpenGLHelper::FORMAT_RGBA, bitmap.getWidth(), bitmap.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.getData());
	mSize = bitmap.getSize();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLTexture::setup(Vec2i size, GLint format)
{
	if (mTextureHandle == 0)
	{
		glGenTextures(1, &mTextureHandle);
	}

	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	mSize = size;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
