/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Framebuffer
*		OpenGL framebuffer wrapper classes.
*/

#pragma once


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
	GLuint mHandle;
	GLenum mFormat;
	int mWidth, mHeight;
};


class API_EXPORT Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	inline GLuint getHandle() const  { return mHandle; }

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

	inline Recti getViewport() const  { return Recti(0, 0, mWidth, mHeight); }

private:
	void deleteAttachedBuffer(GLenum attachment);

private:
	GLuint mHandle;
	int mWidth, mHeight;

	typedef std::map<GLenum, Renderbuffer*> RenderbufferMap;
	RenderbufferMap mRenderbuffers;
};
