/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include <rmxmedia.h>


class BufferTexture
{
public:
	static bool supportsBufferTextures();

public:
	enum class PixelFormat
	{
		UINT_8,
		INT_16,
		UINT_16
	};

public:
	BufferTexture();
	~BufferTexture();

	void create(PixelFormat pixelFormat, int width = 0, int height = 1, const void* data = nullptr);
	void create(PixelFormat pixelFormat, Vec2i size, const void* data = nullptr);

	inline bool isValid() const				{ return mTexBuffer != (GLuint)~0; }
	inline GLuint getTextureHandle() const	{ return mTextureHandle; }
	inline GLuint getBufferHandle() const	{ return mTexBuffer; }

	void bindBuffer() const;
	static void unbindBuffer();

	void bindTexture() const;

	void bufferData(const void* data, int width, int height);

private:
	GLuint mTextureHandle = 0;
	GLuint mTexBuffer = (GLuint)~0;
	PixelFormat mPixelFormat = PixelFormat::UINT_8;
};

#endif
