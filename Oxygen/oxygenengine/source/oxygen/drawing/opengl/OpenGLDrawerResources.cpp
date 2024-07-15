/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/drawing/opengl/OpenGLDrawerResources.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectColoredShader.h"
#include "oxygen/rendering/parts/palette/Palette.h"


namespace openglresources
{
	enum Variant
	{
		VARIANT_STANDARD			= 0,
		VARIANT_TINTCOLOR			= 1,
		VARIANT_ALPHATEST			= 2,
		VARIANT_TINTCOLOR_ALPHATEST = 3
	};
	const char* variantString[4] = { "Standard", "TintColor", "Standard_AlphaTest", "TintColor_AlphaTest" };

	static const Vec2i PALETTE_TEXTURE_SIZE = Vec2i(256, 4);

	struct PaletteData
	{
		Bitmap		  mBitmap;
		OpenGLTexture mTexture;
		uint16		  mChangeCounters[2] = { 0 };
		int			  mUnusedFramesCounter = 0;
	};

	struct Internal
	{
		SimpleRectColoredShader mSimpleRectColoredShader;
		Shader mSimpleRectVertexColorShader;
		Shader mSimpleRectTexturedShader[4];		// Enumerated using enum Variant
		Shader mSimpleRectTexturedUVShader[4];		// Enumerated using enum Variant
		Shader mSimpleRectIndexedShader[4];			// Enumerated using enum Variant
		opengl::VertexArrayObject mSimpleQuadVAO;
		std::unordered_map<uint64, PaletteData> mCustomPalettes;	// Using a key built from a combination of primary and secondary palette keys
	};
	Internal* mInternal = nullptr;

	struct State
	{
		BlendMode mBlendMode = BlendMode::OPAQUE;
	};
	State mState;


	// TODO: The whole palette code was copied from OpenGLRenderResources - this should definitely be refactored!

	bool updatePaletteBitmap(const PaletteBase& palette, Bitmap& bitmap, int offsetY, uint16& changeCounter)
	{
		if (changeCounter == palette.getChangeCounter())
			return false;

		// Copy over the palette data
		uint32* dst = bitmap.getPixelPointer(0, offsetY);
		palette.dumpColors(dst, palette.getSize());

		changeCounter = palette.getChangeCounter();
		return true;
	}

	bool updatePalette(PaletteData& data, const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette)
	{
		data.mUnusedFramesCounter = 0;

		const bool primaryPaletteChanged = updatePaletteBitmap(primaryPalette, data.mBitmap, 0, data.mChangeCounters[0]);
		const bool secondaryPaletteChanged = updatePaletteBitmap(secondaryPalette, data.mBitmap, 2, data.mChangeCounters[1]);
		if (!primaryPaletteChanged && !secondaryPaletteChanged)
			return false;

		if (!data.mTexture.isValid())
			data.mTexture.setup(data.mBitmap.getSize(), rmx::OpenGLHelper::FORMAT_RGBA);

		// Upload changes to the GPU
		glBindTexture(GL_TEXTURE_2D, data.mTexture.getHandle());
		if (secondaryPaletteChanged)
		{
			// Update everything
			glTexImage2D(GL_TEXTURE_2D, 0, rmx::OpenGLHelper::FORMAT_RGBA, data.mBitmap.getWidth(), data.mBitmap.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data.mBitmap.getData());
		}
		else
		{
			// Update only the primary palette
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 2, GL_RGBA, GL_UNSIGNED_BYTE, data.mBitmap.getData());
		}
		return true;
	}
}


void OpenGLDrawerResources::startup()
{
	if (nullptr != openglresources::mInternal)
		return;

	openglresources::mInternal = new openglresources::Internal();
	openglresources::State mState = openglresources::State();

	// Load shaders
	openglresources::mInternal->mSimpleRectColoredShader.initialize();

	FileHelper::loadShader(openglresources::mInternal->mSimpleRectVertexColorShader, L"data/shader/simple_rect_vertexcolor.shader", "Standard");
	for (int k = 0; k < 4; ++k)
	{
		FileHelper::loadShader(openglresources::mInternal->mSimpleRectTexturedShader[k],   L"data/shader/simple_rect_textured.shader",    openglresources::variantString[k]);
		FileHelper::loadShader(openglresources::mInternal->mSimpleRectTexturedUVShader[k], L"data/shader/simple_rect_textured_uv.shader", openglresources::variantString[k]);
		FileHelper::loadShader(openglresources::mInternal->mSimpleRectIndexedShader[k],    L"data/shader/simple_rect_indexed.shader",     openglresources::variantString[k]);
	}

	// Setup simple quad VAO, consisting of two triangles
	{
		opengl::VertexArrayObject& vao = openglresources::mInternal->mSimpleQuadVAO;
		const float vertexData[] =
		{
			0.0f, 0.0f,		// Upper left
			0.0f, 1.0f,		// Lower left
			1.0f, 1.0f,		// Lower right
			1.0f, 1.0f,		// Lower right
			1.0f, 0.0f,		// Upper right
			0.0f, 0.0f		// Upper left
		};
		vao.setup(opengl::VertexArrayObject::Format::P2);
		vao.updateVertexData(vertexData, 6);
	}
}

void OpenGLDrawerResources::shutdown()
{
	SAFE_DELETE(openglresources::mInternal);
}

SimpleRectColoredShader& OpenGLDrawerResources::getSimpleRectColoredShader()
{
	return openglresources::mInternal->mSimpleRectColoredShader;
}

Shader& OpenGLDrawerResources::getSimpleRectVertexColorShader()
{
	return openglresources::mInternal->mSimpleRectVertexColorShader;
}

Shader& OpenGLDrawerResources::getSimpleRectTexturedShader(bool tint, bool alpha)
{
	return openglresources::mInternal->mSimpleRectTexturedShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

Shader& OpenGLDrawerResources::getSimpleRectTexturedUVShader(bool tint, bool alpha)
{
	return openglresources::mInternal->mSimpleRectTexturedUVShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

Shader& OpenGLDrawerResources::getSimpleRectIndexedShader(bool tint, bool alpha)
{
	return openglresources::mInternal->mSimpleRectIndexedShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

opengl::VertexArrayObject& OpenGLDrawerResources::getSimpleQuadVAO()
{
	return openglresources::mInternal->mSimpleQuadVAO;
}

BlendMode OpenGLDrawerResources::getBlendMode()
{
	return openglresources::mState.mBlendMode;
}

void OpenGLDrawerResources::setBlendMode(BlendMode blendMode)
{
	if (openglresources::mState.mBlendMode == blendMode)
		return;

	openglresources::mState.mBlendMode = blendMode;
	switch (blendMode)
	{
		case BlendMode::OPAQUE:
		{
			glDisable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ZERO);
			break;
		}

		case BlendMode::ALPHA:
		case BlendMode::ONE_BIT:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		}

		case BlendMode::ADDITIVE:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		}

		case BlendMode::SUBTRACTIVE:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		}

		case BlendMode::MULTIPLICATIVE:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);		// No support for src alpha consideration
			break;
		}

		case BlendMode::MINIMUM:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_MIN);
			glBlendFunc(GL_ONE, GL_ONE);			// No support for src alpha consideration
			break;
		}

		case BlendMode::MAXIMUM:
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_MAX);
			glBlendFunc(GL_ONE, GL_ONE);			// No support for src alpha consideration
			break;
		}
	}
}

const OpenGLTexture& OpenGLDrawerResources::getCustomPaletteTexture(const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette)
{
	const uint64 combinedKey = primaryPalette.getKey() ^ (secondaryPalette.getKey() << 32) ^ (secondaryPalette.getKey() >> 32);
	openglresources::PaletteData& data = openglresources::mInternal->mCustomPalettes[combinedKey];

	if (data.mBitmap.getSize() != openglresources::PALETTE_TEXTURE_SIZE)
		data.mBitmap.create(openglresources::PALETTE_TEXTURE_SIZE);	// The shader expects this exact texture size, no matter how many colors are actually used

	openglresources::updatePalette(data, primaryPalette, secondaryPalette);
	return data.mTexture;
}

#endif
