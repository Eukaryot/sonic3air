/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Texture
*		OpenGL texture (2D or cubemap) wrapper class.
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "OpenGLHelper.h"

class Texture
{
public:
	Texture();
	Texture(const Bitmap& bitmap);
	Texture(const String& filename);
	~Texture();

	void generate();	// Just calls "glGenTextures", nothing else

	void create(GLenum type = GL_TEXTURE_2D);
	void create(GLint format = rmx::OpenGLHelper::FORMAT_RGBA);
	void create(int width, int height, GLint format = rmx::OpenGLHelper::FORMAT_RGBA);
	void createCubemap(GLint format = rmx::OpenGLHelper::FORMAT_RGBA);
	void createCubemap(int width, int height, GLint format = rmx::OpenGLHelper::FORMAT_RGBA);

	void load(const void* data, int width, int height);
	void load(const Bitmap& bitmap);
	void load(const String& filename);
	void loadCubemap(const String& filename);

	void updateRect(const void* data, const Recti& rect);
	void updateRect(const Bitmap& bitmap, int px, int py);

	void copyFramebuffer(const Recti& rect);
	void copyFramebufferCubemap(const Recti& rect, int side);

	void buildMipmaps();

	inline GLuint getHandle() const	{ return mHandle; }
	inline GLenum getType() const	{ return mType; }
	inline GLint getFormat() const	{ return mFormat; }
	inline int getWidth() const		{ return mWidth; }
	inline int getHeight() const	{ return mHeight; }
	inline Recti getRect() const	{ return Recti(0, 0, mWidth, mHeight); }
	inline float getAspectRatio() const  { return (float)mWidth / (float)mHeight; }

	void bind() const;
	void unbind() const;

	void setFilterNearest();
	void setFilterLinear();
	void setWrapClamp();
	void setWrapRepeat();
	void setWrapRepeatMirror();

	inline GLuint operator*() const  { return mHandle; }

	static GLenum getDefaultDataFormat(GLenum internalFormat);

private:
	void initialize();
	bool checkHandle() const;

private:
	mutable GLuint mHandle = 0;
	GLenum mType = 0;
	GLint  mFormat = 0;
	int    mWidth = 0;
	int    mHeight = 0;
	bool   mFilterLinear = true;
	bool   mHasMipmaps = false;
};

#endif
