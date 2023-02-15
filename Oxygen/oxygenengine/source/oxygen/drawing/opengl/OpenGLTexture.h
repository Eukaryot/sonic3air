/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include <rmxmedia.h>


class OpenGLTexture final
{
public:
	~OpenGLTexture();

	inline GLuint getHandle() const	{ return mTextureHandle; }
	inline Vec2i getSize() const	{ return mSize; }

	void loadBitmap(const Bitmap& bitmap);
	void setup(Vec2i size, GLint format);

private:
	GLuint mTextureHandle = 0;
	Vec2i mSize;
};

#endif
