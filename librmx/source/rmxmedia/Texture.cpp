/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


Texture::Texture()
{
	initialize();
}

Texture::Texture(const Bitmap& bitmap)
{
	initialize();
	load(bitmap);
}

Texture::Texture(const String& filename)
{
	initialize();
	load(filename);
}

Texture::~Texture()
{
	if (mHandle != 0)
	{
		glDeleteTextures(1, &mHandle);
	}
}

void Texture::initialize()
{
	mHandle = 0;
	mType = 0;
	mFormat = 0;
	mWidth = 0;
	mHeight = 0;
	mFilterLinear = true;
	mHasMipmaps = false;
}

bool Texture::checkHandle() const
{
#ifdef DEBUG
	if (mHandle && !glIsTexture(mHandle))
	{
		RMX_ASSERT(false, "Texture handle is not a texture");
		mHandle = 0;
	}
#endif
	return (mHandle != 0);
}

GLenum Texture::getDefaultDataFormat(GLenum internalFormat)
{
	if (internalFormat == GL_DEPTH_COMPONENT)
		return GL_DEPTH_COMPONENT;
#ifdef ALLOW_LEGACY_OPENGL
	if (internalFormat >= GL_DEPTH_COMPONENT16 && internalFormat <= GL_DEPTH_COMPONENT32)
		return GL_DEPTH_COMPONENT;
#endif
	return GL_RGBA;
}

void Texture::generate()
{
	glGenTextures(1, &mHandle);
}

void Texture::create(GLenum type)
{
	if (!checkHandle())
		glGenTextures(1, &mHandle);

	mType = type;
	mFormat = rmx::OpenGLHelper::FORMAT_RGBA;
	mFilterLinear = false;
	mHasMipmaps = false;

	glBindTexture(mType, mHandle);
	setFilterNearest();
	setWrapRepeat();
}

void Texture::create(GLint format)
{
	if (!checkHandle())
		glGenTextures(1, &mHandle);

	mType = GL_TEXTURE_2D;
	mFormat = format;
	mFilterLinear = true;
	mHasMipmaps = false;

	glBindTexture(mType, mHandle);
	setFilterLinear();
	setWrapRepeat();
}

void Texture::create(int width, int height, GLint format)
{
	if (!checkHandle())
		glGenTextures(1, &mHandle);

	mType = GL_TEXTURE_2D;
	mFormat = format;
	mWidth = width;
	mHeight = height;
	mFilterLinear = true;
	mHasMipmaps = false;

	glBindTexture(mType, mHandle);
	setFilterLinear();
	setWrapRepeat();
	glTexImage2D(mType, 0, mFormat, mWidth, mHeight, 0, getDefaultDataFormat(mFormat), GL_UNSIGNED_BYTE, nullptr);
}

void Texture::createCubemap(GLint format)
{
	if (!checkHandle())
		glGenTextures(1, &mHandle);

	mType = GL_TEXTURE_CUBE_MAP;
	mFormat = format;
	mFilterLinear = true;
	mHasMipmaps = false;

	glBindTexture(mType, mHandle);
	setFilterLinear();
	setWrapClamp();
}

void Texture::createCubemap(int width, int height, GLint format)
{
	if (!checkHandle())
		glGenTextures(1, &mHandle);

	mType = GL_TEXTURE_CUBE_MAP;
	mFormat = format;
	mWidth = width;
	mHeight = height;
	mFilterLinear = true;
	mHasMipmaps = false;

	glBindTexture(mType, mHandle);
	setFilterLinear();
	setWrapClamp();

	for (int side = 0; side < 6; ++side)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, mFormat, mWidth, mHeight, 0, getDefaultDataFormat(mFormat), GL_UNSIGNED_BYTE, nullptr);
	}
}

void Texture::load(const void* data, int width, int height)
{
	if (nullptr == data)
		return;

	create(width, height, rmx::OpenGLHelper::FORMAT_RGBA);
	glTexImage2D(mType, 0, mFormat, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void Texture::load(const Bitmap& bitmap)
{
	load(bitmap.mData, bitmap.mWidth, bitmap.mHeight);
}

void Texture::load(const String& filename)
{
	if (filename.includes("??"))
	{
		loadCubemap(filename);
		return;
	}
	Bitmap bitmap;
	if (bitmap.load(filename.toWString()))
		load(bitmap.mData, bitmap.mWidth, bitmap.mHeight);
}

void Texture::loadCubemap(const String& filename)
{
	createCubemap();
	int pos = -1;
	if (!filename.includes("??", pos))
		return;

	String fname(filename);
	fname.makeDynamic();
	Bitmap bitmap;
	for (int i = 0; i < 6; ++i)
	{
		fname[pos] = (i%2) ? 'n' : 'p';
		fname[pos+1] = 'x' + (i/2);
		if (!bitmap.load(fname.toWString()))
			continue;
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, mFormat, bitmap.mWidth, bitmap.mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mData);
	}
}

void Texture::updateRect(const void* data, const Recti& rect)
{
	if (nullptr == data || rect.empty())
		return;

	glBindTexture(mType, mHandle);
	glTexSubImage2D(mType, 0, rect.x, rect.y, rect.width, rect.height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void Texture::updateRect(const Bitmap& bitmap, int px, int py)
{
	updateRect(bitmap.mData, Recti(px, py, bitmap.mWidth, bitmap.mHeight));
}

void Texture::copyFramebuffer(const Recti& rect)
{
	bind();
	glCopyTexImage2D(mType, 0, mFormat, rect.left, rect.top, rect.left+rect.width, rect.top+rect.height, 0);
}

void Texture::copyFramebufferCubemap(const Recti& rect, int side)
{
	bind();
	glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, mFormat, rect.left, rect.top, rect.left+rect.width, rect.top+rect.height, 0);
}

void Texture::buildMipmaps()
{
	if (mHandle == 0 || mType == 0)
		return;

	bind();
	glTexParameteri(mType, GL_TEXTURE_MIN_FILTER, mFilterLinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
	glGenerateMipmap(mType);
	mHasMipmaps = true;
}

void Texture::bind() const
{
	if (!checkHandle())
		return;
#ifdef PLATFORM_WINDOWS
	glEnable(mType);
#endif
	glBindTexture(mType, mHandle);
}

void Texture::unbind() const
{
	glBindTexture(mType, 0);
}

void Texture::setFilterNearest()
{
	glTexParameteri(mType, GL_TEXTURE_MIN_FILTER, mHasMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(mType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Texture::setFilterLinear()
{
	glTexParameteri(mType, GL_TEXTURE_MIN_FILTER, mHasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(mType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::setWrapClamp()
{
	glTexParameteri(mType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(mType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef ALLOW_LEGACY_OPENGL
	glTexParameteri(mType, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
}

void Texture::setWrapRepeat()
{
	glTexParameteri(mType, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(mType, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifdef ALLOW_LEGACY_OPENGL
	glTexParameteri(mType, GL_TEXTURE_WRAP_R, GL_REPEAT);
#endif
}

void Texture::setWrapRepeatMirror()
{
	glTexParameteri(mType, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(mType, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
#ifdef ALLOW_LEGACY_OPENGL
	glTexParameteri(mType, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
#endif
}
