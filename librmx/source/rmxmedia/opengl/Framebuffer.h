/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Framebuffer
*		OpenGL framebuffer wrapper classes.
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT


class API_EXPORT Renderbuffer
{
public:
	Renderbuffer();
	~Renderbuffer();

	void create();
	void create(GLenum format, int width, int height);
	void setSize(int width, int height);
	void destroy();

	inline GLuint getHandle() const  { return mHandle; }

private:
	GLuint mHandle = 0;
	GLenum mFormat = 0;
	int mWidth = 0;
	int mHeight = 0;
};


class API_EXPORT Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	void create();
	void create(int width, int height);
	void finishCreation();
	void destroy();
	void setSize(int width, int height);

	void attachTexture(GLenum attachment, GLuint handle, GLenum texTarget = GL_TEXTURE_2D);
	void attachTexture(GLenum attachment, const Texture* texture, GLenum texTarget = GL_TEXTURE_2D);

	void attachRenderbuffer(GLenum attachment, GLuint handle);
	void createRenderbuffer(GLenum attachment, GLenum internalformat);

	void bind();
	void unbind();

	void activate();
	void activate(GLbitfield clearmask);
	void deactivate();

	inline GLuint getHandle() const   { return mHandle; }
	inline Recti getViewport() const  { return Recti(0, 0, mWidth, mHeight); }

private:
	void deleteAttachedBuffer(GLenum attachment);

private:
	GLuint mHandle = 0;
	int mWidth = 0;
	int mHeight = 0;
	std::map<GLenum, Renderbuffer*> mRenderbuffers;
};

#endif
