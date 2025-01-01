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


class Upscaler
{
public:
	explicit Upscaler(OpenGLDrawerResources& resources) : mResources(resources) {}

	void startup();
	void shutdown();

	void renderImage(const Recti& rect, GLuint textureHandle, Vec2i textureResolution);

private:
	OpenGLDrawerResources& mResources;

	Shader mUpscalerSoftShader;
	Shader mUpscalerSoftShaderScanlines;
	Shader mUpscalerXBRZMultipassShader[2];
	Shader mUpscalerHQ2xShader;
	Shader mUpscalerHQ3xShader;
	Shader mUpscalerHQ4xShader;

	Framebuffer mPass0Buffer;
	OpenGLTexture mPass0Texture;
	OpenGLTexture mLookupTexture[3];
};

#endif
