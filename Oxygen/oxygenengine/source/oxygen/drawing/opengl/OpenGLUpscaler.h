/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLTexture.h"

class OpenGLDrawerResources;


class OpenGLUpscaler
{
public:
	enum class Type
	{
		DEFAULT,
		SOFT,
		XBRZ,
		HQX
	};

public:
	OpenGLUpscaler(Type type, OpenGLDrawerResources& resources) : mType(type), mResources(resources) {}

	void startup();
	void shutdown();

	void renderImage(const Recti& rect, GLuint textureHandle, Vec2i textureResolution);

private:
	const Type mType = Type::DEFAULT;
	OpenGLDrawerResources& mResources;

	std::vector<Shader> mShaders;
	Framebuffer mPass0Buffer;
	OpenGLTexture mPass0Texture;
	OpenGLTexture mLookupTexture[3];

	bool mFilterLinear = false;
};

#endif
