/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/Upscaler.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"


void Upscaler::startup()
{
	FileHelper::loadShader(mUpscalerSoftShader,             L"data/shader/upscaler_soft.shader", "Standard");
	FileHelper::loadShader(mUpscalerSoftShaderScanlines,    L"data/shader/upscaler_soft.shader", "Scanlines");
	FileHelper::loadShader(mUpscalerXBRZMultipassShader[0], L"data/shader/upscaler_xbrz-freescale-pass0.shader", "Standard");
	FileHelper::loadShader(mUpscalerXBRZMultipassShader[1], L"data/shader/upscaler_xbrz-freescale-pass1.shader", "Standard");
	FileHelper::loadShader(mUpscalerHQ2xShader,             L"data/shader/upscaler_hqx.shader", "Standard_2x");
	FileHelper::loadShader(mUpscalerHQ3xShader,             L"data/shader/upscaler_hqx.shader", "Standard_3x");
	FileHelper::loadShader(mUpscalerHQ4xShader,             L"data/shader/upscaler_hqx.shader", "Standard_4x");

	mPass0Texture.setup(Configuration::instance().mGameScreen, rmx::OpenGLHelper::FORMAT_RGBA);

	mPass0Buffer.create();
	mPass0Buffer.attachTexture(GL_COLOR_ATTACHMENT0, mPass0Texture.getHandle(), GL_TEXTURE_2D);
	mPass0Buffer.finishCreation();
	mPass0Buffer.unbind();
}

void Upscaler::shutdown()
{
}

void Upscaler::renderImage(const Rectf& rect, GLuint textureHandle, Vec2i textureResolution)
{
	const int filtering = Configuration::instance().mFiltering;
	const int scanlines = Configuration::instance().mScanlines;

	// Select upscaler
	Shader& simpleRectTexturedShader = OpenGLDrawerResources::getSimpleRectTexturedShader(false, false);
	Shader* upscaleShader = &simpleRectTexturedShader;		// Fallback: Simple rendering
	Shader* pass0Shader = nullptr;
	bool filterLinear = false;
	int lookupTextureIndex = -1;

	glBindTexture(GL_TEXTURE_2D, textureHandle);
	if (scanlines > 0 && filtering < 3)
	{
		filterLinear = true;
		upscaleShader = &mUpscalerSoftShaderScanlines;
	}
	else
	{
		switch (filtering)
		{
			case 0:
				upscaleShader = &simpleRectTexturedShader;
				break;

			case 1:
			case 2:
				filterLinear = true;
				upscaleShader = &mUpscalerSoftShader;
				break;

			case 3:
				pass0Shader = &mUpscalerXBRZMultipassShader[0];
				upscaleShader = &mUpscalerXBRZMultipassShader[1];
				break;

			case 4:
				upscaleShader = &mUpscalerHQ2xShader;
				lookupTextureIndex = 0;
				break;
			case 5:
				upscaleShader = &mUpscalerHQ3xShader;
				lookupTextureIndex = 1;
				break;
			case 6:
				upscaleShader = &mUpscalerHQ4xShader;
				lookupTextureIndex = 2;
				break;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterLinear ? GL_LINEAR : GL_NEAREST);

	// Multipass rendering?
	const bool isMultiPass = (nullptr != pass0Shader);
	Shader* firstShader = isMultiPass ? pass0Shader : upscaleShader;
	Shader* secondShader = upscaleShader;

	// Need a lookup texture?
	if (lookupTextureIndex != -1)
	{
		if (mLookupTexture[lookupTextureIndex].getHandle() == 0)
		{
			const wchar_t* textureFilename = (lookupTextureIndex == 0) ? L"hq2x.png" : (lookupTextureIndex == 1) ? L"hq3x.png" : L"hq4x.png";
			Bitmap bitmap;
			if (FileHelper::loadBitmap(bitmap, std::wstring(L"data/shader/") + textureFilename))
			{
				mLookupTexture[lookupTextureIndex].loadBitmap(bitmap);
			}
		}
	}

	firstShader->bind();
	firstShader->setTexture("Texture", textureHandle, GL_TEXTURE_2D);

	if (firstShader == &simpleRectTexturedShader)
	{
		firstShader->setParam("Transform", Vec4f(-1.0f, 1.0f, 2.0f, -2.0f));
	}
	else
	{
		firstShader->setParam("GameResolution", Vec2f(textureResolution));

		// Configuration for soft shader
		if (firstShader == &mUpscalerSoftShader || firstShader == &mUpscalerSoftShaderScanlines)
		{
			// PixelFactor is at least 1.0f, which is basically bilinear sampling, infinity would be point sampling
			float pixelFactor = rect.height / (float)textureResolution.y;
			pixelFactor *= (filtering == 1) ? 2.0f : 1.0f;
			firstShader->setParam("PixelFactor", clamp(pixelFactor, 1.0f, 1000.0f));

			if (firstShader == &mUpscalerSoftShaderScanlines)
			{
				firstShader->setParam("ScanlinesIntensity", (float)scanlines * 0.25f);
			}
		}

		// Configuration for shaders that need lookup textures
		if (lookupTextureIndex != -1)
		{
			firstShader->setTexture("LUT", mLookupTexture[lookupTextureIndex].getHandle(), GL_TEXTURE_2D);
		}
	}

	// Disable blending (though it shouldn't be necessary, as upscaling shaders usually do this already)
	glDisable(GL_BLEND);

	opengl::VertexArrayObject& vao = OpenGLDrawerResources::getSimpleQuadVAO();
	vao.bind();

	if (isMultiPass)
	{
		// Render first pass
		glBindFramebuffer(GL_FRAMEBUFFER, mPass0Buffer.getHandle());
		glViewport(0, 0, textureResolution.x, textureResolution.y);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// And then the second pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (mPass0Texture.getSize() != textureResolution)
		{
			mPass0Texture.setup(textureResolution, rmx::OpenGLHelper::FORMAT_RGBA);
		}

		secondShader->bind();
		secondShader->setParam("GameResolution", Vec2f(textureResolution));
		secondShader->setParam("OutputSize", Vec2f(rect.width, rect.height));
		secondShader->setTexture("Texture", mPass0Texture.getHandle(), GL_TEXTURE_2D);
		secondShader->setTexture("OrigTexture", textureHandle, GL_TEXTURE_2D);
	}

	glViewport_Recti(rect);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glViewport_Recti(FTX::screenRect());

	secondShader->unbind();
}

#endif
