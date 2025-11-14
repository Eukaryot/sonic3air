/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLUpscaler.h"
#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectTexturedShader.h"


void OpenGLUpscaler::startup()
{
	mFilterLinear = false;

	switch (mType)
	{
		default:
		case Type::DEFAULT:
			break;

		case Type::SOFT:
		{
			mFilterLinear = true;

			mShaders.resize(2);
			FileHelper::loadShader(mShaders[0], L"data/shader/upscaler_soft.shader", "Standard");
			FileHelper::loadShader(mShaders[1], L"data/shader/upscaler_soft.shader", "Scanlines");
			break;
		}

	#if !defined(PLATFORM_VITA)
		case Type::XBRZ:
		{
			mShaders.resize(2);
			FileHelper::loadShader(mShaders[0], L"data/shader/upscaler_xbrz-freescale-pass0.shader", "Standard");
			FileHelper::loadShader(mShaders[1], L"data/shader/upscaler_xbrz-freescale-pass1.shader", "Standard");
			break;
		}

		case Type::HQX:
		{
			mShaders.resize(3);
			FileHelper::loadShader(mShaders[0], L"data/shader/upscaler_hqx.shader", "Standard_2x");
			FileHelper::loadShader(mShaders[1], L"data/shader/upscaler_hqx.shader", "Standard_3x");
			FileHelper::loadShader(mShaders[2], L"data/shader/upscaler_hqx.shader", "Standard_4x");
			break;
		}
	#endif
	}

	mPass0Texture.setup(Configuration::instance().mGameScreen, rmx::OpenGLHelper::FORMAT_RGBA);

	mPass0Buffer.create();
	mPass0Buffer.attachTexture(GL_COLOR_ATTACHMENT0, mPass0Texture.getHandle(), GL_TEXTURE_2D);
	mPass0Buffer.finishCreation();
	mPass0Buffer.unbind();
}

void OpenGLUpscaler::shutdown()
{
}

void OpenGLUpscaler::renderImage(const Recti& rect, GLuint textureHandle, Vec2i textureResolution)
{
	const int filtering = Configuration::instance().mFiltering;
	const int scanlines = Configuration::instance().mScanlines;

	// Select upscaler
	Shader* upscaleShader = nullptr;
	Shader* pass0Shader = nullptr;
	int lookupTextureIndex = -1;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	switch (mType)
	{
		default:
		case Type::DEFAULT:
			break;

		case Type::SOFT:
		{
			upscaleShader = &mShaders[(scanlines > 0) ? 1 : 0];
			break;
		}

		case Type::XBRZ:
		{
			pass0Shader = &mShaders[0];
			upscaleShader = &mShaders[1];
			break;
		}

		case Type::HQX:
		{
			switch (filtering)
			{
				default:
				case 4:
					lookupTextureIndex = 0;
					break;
				case 5:
					lookupTextureIndex = 1;
					break;
				case 6:
					lookupTextureIndex = 2;
					break;
			}
			upscaleShader = &mShaders[lookupTextureIndex];
			break;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mFilterLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mFilterLinear ? GL_LINEAR : GL_NEAREST);

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

	if (mType == Type::DEFAULT)
	{
		SimpleRectTexturedShader& simpleRectTexturedShader = mResources.getSimpleRectTexturedShader(false, false);
		simpleRectTexturedShader.setup(textureHandle, Vec4f(-1.0f, 1.0f, 2.0f, -2.0f));
	}
	else
	{
		firstShader->bind();
		firstShader->setTexture("MainTexture", textureHandle, GL_TEXTURE_2D);
		firstShader->setParam("GameResolution", Vec2f(textureResolution));

		// Configuration for soft shader
		if (mType == Type::SOFT)
		{
			// PixelFactor is at least 1.0f, which is basically bilinear sampling, infinity would be point sampling
			float pixelFactor = rect.height / (float)textureResolution.y;
			pixelFactor *= (filtering == 1) ? 2.0f : 1.0f;
			firstShader->setParam("PixelFactor", clamp(pixelFactor, 1.0f, 1000.0f));

			if (firstShader == &mShaders[1])
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
	mResources.setBlendMode(BlendMode::OPAQUE);

	opengl::VertexArrayObject& vao = mResources.getSimpleQuadVAO();
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
		secondShader->setParam("OutputSize", Vec2f(rect.getSize()));
		secondShader->setTexture("MainTexture", mPass0Texture.getHandle(), GL_TEXTURE_2D);
		secondShader->setTexture("OrigTexture", textureHandle, GL_TEXTURE_2D);
	}

	// Flip rect in y direction
	glViewport_Recti(Recti(rect.x, FTX::screenHeight() - rect.height - rect.y, rect.width, rect.height));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glViewport_Recti(FTX::screenRect());

	secondShader->unbind();
	OpenGLShader::resetLastUsedShader();
}

#endif
