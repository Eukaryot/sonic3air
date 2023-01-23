/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/opengl/OpenGLTexture.h"


class Upscaler
{
public:
	void startup();
	void shutdown();

	void renderImage(const Rectf& rect, GLuint textureHandle, Vec2i textureResolution);

private:
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
