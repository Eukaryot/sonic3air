/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#ifdef RMX_WITH_OPENGL_SUPPORT


/* ----- Renderbuffer ---------------------------------------------------------------------------------------------- */

Renderbuffer::Renderbuffer()
{
}

Renderbuffer::~Renderbuffer()
{
	destroy();
}

void Renderbuffer::create()
{
	if (mHandle && !glIsRenderbuffer(mHandle))
		mHandle = 0;
	if (mHandle != 0)
		return;

	glGenRenderbuffers(1, &mHandle);
	mFormat = 0;
	mWidth = 0;
	mHeight = 0;
}

void Renderbuffer::create(GLenum format, int width, int height)
{
	if (format == mFormat && width == mWidth && height == mHeight)
		return;

	create();
	mFormat = format;
	setSize(width, height);
}

void Renderbuffer::setSize(int width, int height)
{
	if (mHandle == 0 || mFormat == 0)
		return;
	if (width == mWidth && height == mHeight)
		return;

	mWidth = width;
	mHeight = height;
	glBindRenderbuffer(GL_RENDERBUFFER, mHandle);
	glRenderbufferStorage(GL_RENDERBUFFER, mFormat, mWidth, mHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderbuffer::destroy()
{
	if (mHandle != 0)
	{
		glDeleteRenderbuffers(1, &mHandle);
		mHandle = 0;
	}
}



/* ----- Framebuffer ----------------------------------------------------------------------------------------------- */

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
	destroy();
}

void Framebuffer::create()
{
	if (mHandle == 0)
		glGenFramebuffers(1, &mHandle);
}

void Framebuffer::create(int width, int height)
{
	create();
	setSize(width, height);
}

void Framebuffer::finishCreation()
{
	// Just do some final checks
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	RMX_CHECK(status == GL_FRAMEBUFFER_COMPLETE, "Failed to create framebuffer with error: " << rmx::hexString(status, 4) << " (OpenGL error: " << getGLErrorDescription(glGetError()) << ")", );
}

void Framebuffer::destroy()
{
	if (!mRenderbuffers.empty())
	{
		for (const auto& pair : mRenderbuffers)
		{
			delete pair.second;
		}
		mRenderbuffers.clear();
	}
	if (mHandle != 0)
	{
		glDeleteFramebuffers(1, &mHandle);
		mHandle = 0;
	}
}

void Framebuffer::setSize(int width, int height)
{
	if (width == mWidth && height == mHeight)
		return;

	mWidth = width;
	mHeight = height;
	for (const auto& pair : mRenderbuffers)
	{
		if (nullptr != pair.second)
		{
			pair.second->setSize(mWidth, mHeight);
		}
	}
}

void Framebuffer::attachTexture(GLenum attachment, GLuint handle, GLenum texTarget)
{
	deleteAttachedBuffer(attachment);
	bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texTarget, handle, 0);
}

void Framebuffer::attachTexture(GLenum attachment, const Texture* texture, GLenum texTarget)
{
	if (texture)
		attachTexture(attachment, texture->getHandle(), texTarget);
}

void Framebuffer::attachRenderbuffer(GLenum attachment, GLuint handle)
{
	deleteAttachedBuffer(attachment);
	bind();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, handle);
}

void Framebuffer::createRenderbuffer(GLenum attachment, GLenum internalformat)
{
	Renderbuffer* renderbuffer = mapFindOrDefault(mRenderbuffers, attachment, nullptr);
	if (nullptr == renderbuffer)
	{
		renderbuffer = new Renderbuffer();
		mRenderbuffers.emplace(attachment, renderbuffer);
	}
	renderbuffer->create(internalformat, mWidth, mHeight);
	bind();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer->getHandle());
}

void Framebuffer::bind()
{
	if (mHandle && !glIsFramebuffer(mHandle))
		mHandle = 0;
	if (mHandle == 0)
		create();
	glBindFramebuffer(GL_FRAMEBUFFER, mHandle);
}

void Framebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::activate()
{
	bind();
	if (mWidth > 0 && mHeight > 0)
		glViewport(0, 0, mWidth, mHeight);
}

void Framebuffer::activate(GLbitfield clearmask)
{
	activate();
	glClear(clearmask);
}

void Framebuffer::deactivate()
{
	unbind();
}

void Framebuffer::deleteAttachedBuffer(GLenum attachment)
{
	const auto it = mRenderbuffers.find(attachment);
	if (it != mRenderbuffers.end())
	{
		delete it->second;
		mRenderbuffers.erase(it);
	}
}

#endif
