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
#include "oxygen/helper/FileHelper.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectColoredShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectIndexedShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectTexturedShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectTexturedUVShader.h"
#include "oxygen/rendering/opengl/shaders/SimpleRectVertexColorShader.h"
#include "oxygen/rendering/parts/palette/PaletteManager.h"


namespace
{
	static const Vec2i PALETTE_TEXTURE_SIZE = Vec2i(256, PaletteManager::MAIN_PALETTE_SIZE / 256 * 2);
}


struct OpenGLDrawerResources::Internal
{
	// Shaders
	SimpleRectColoredShader		mSimpleRectColoredShader;
	SimpleRectVertexColorShader	mSimpleRectVertexColorShader;
	SimpleRectTexturedShader	mSimpleRectTexturedShader[4];		// Enumerated using enum Variant
	SimpleRectTexturedUVShader	mSimpleRectTexturedUVShader[4];		// Enumerated using enum Variant
	SimpleRectIndexedShader		mSimpleRectIndexedShader[4];		// Enumerated using enum Variant

	// Vertex array objects
	opengl::VertexArrayObject mSimpleQuadVAO;
};


OpenGLDrawerResources::OpenGLDrawerResources() :
	mInternal(*new Internal())
{
}

OpenGLDrawerResources::~OpenGLDrawerResources()
{
	delete &mInternal;
}

void OpenGLDrawerResources::startup()
{
	// Load shaders
	{
		const char* variantString[4] = { "Standard", "TintColor", "Standard_AlphaTest", "TintColor_AlphaTest" };

		mInternal.mSimpleRectColoredShader.initialize();
		mInternal.mSimpleRectVertexColorShader.initialize();

		for (int k = 0; k < 4; ++k)
		{
			const bool supportsTintColor = (k % 2) == 1;
			mInternal.mSimpleRectTexturedShader[k]  .initialize(supportsTintColor, variantString[k]);
			mInternal.mSimpleRectTexturedUVShader[k].initialize(supportsTintColor, variantString[k]);
			mInternal.mSimpleRectIndexedShader[k]   .initialize(supportsTintColor, variantString[k]);
		}
	}

	// Setup simple quad VAO, consisting of two triangles
	{
		opengl::VertexArrayObject& vao = mInternal.mSimpleQuadVAO;
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
}

void OpenGLDrawerResources::clearAllCaches()
{
	mCustomPalettes.clear();
}

void OpenGLDrawerResources::refresh(float deltaSeconds)
{
	// Remove cached data for custom palettes after a short time
	mSecondsSinceLastPaletteCleanup += clamp(deltaSeconds, 0.0f, 0.1f);
	if (mSecondsSinceLastPaletteCleanup >= 1.0f)
	{
		for (auto it = mCustomPalettes.begin(); it != mCustomPalettes.end(); )
		{
			PaletteData& data = it->second;
			data.mSecondsSinceLastUse += mSecondsSinceLastPaletteCleanup;
			if (data.mSecondsSinceLastUse >= 5.0f)
				it = mCustomPalettes.erase(it);
			else
				++it;
		}
		mSecondsSinceLastPaletteCleanup = 0.0f;
	}
}

void OpenGLDrawerResources::setBlendMode(BlendMode blendMode)
{
	if (mState.mBlendMode == blendMode)
		return;

	mState.mBlendMode = blendMode;
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

SimpleRectColoredShader& OpenGLDrawerResources::getSimpleRectColoredShader()
{
	return mInternal.mSimpleRectColoredShader;
}

SimpleRectVertexColorShader& OpenGLDrawerResources::getSimpleRectVertexColorShader()
{
	return mInternal.mSimpleRectVertexColorShader;
}

SimpleRectTexturedShader& OpenGLDrawerResources::getSimpleRectTexturedShader(bool tint, bool alpha)
{
	return mInternal.mSimpleRectTexturedShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

SimpleRectTexturedUVShader& OpenGLDrawerResources::getSimpleRectTexturedUVShader(bool tint, bool alpha)
{
	return mInternal.mSimpleRectTexturedUVShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

SimpleRectIndexedShader& OpenGLDrawerResources::getSimpleRectIndexedShader(bool tint, bool alpha)
{
	return mInternal.mSimpleRectIndexedShader[(tint ? 1 : 0) + (alpha ? 2 : 0)];
}

opengl::VertexArrayObject& OpenGLDrawerResources::getSimpleQuadVAO()
{
	return mInternal.mSimpleQuadVAO;
}

const OpenGLTexture& OpenGLDrawerResources::getCustomPaletteTexture(const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette)
{
	const uint64 combinedKey = primaryPalette.getKey() ^ (secondaryPalette.getKey() << 32) ^ (secondaryPalette.getKey() >> 32);
	PaletteData& data = mCustomPalettes[combinedKey];

	if (data.mBitmap.getSize() != PALETTE_TEXTURE_SIZE)
		data.mBitmap.create(PALETTE_TEXTURE_SIZE);	// The shaders expect this exact texture size, no matter how many colors are actually used

	updatePalette(data, primaryPalette, secondaryPalette);
	return data.mTexture;
}

bool OpenGLDrawerResources::updatePalette(PaletteData& data, const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette)
{
	data.mSecondsSinceLastUse = 0.0f;

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

const Vec2i& OpenGLDrawerResources::getPaletteTextureSize() const
{
	return PALETTE_TEXTURE_SIZE;
}

bool OpenGLDrawerResources::updatePaletteBitmap(const PaletteBase& palette, Bitmap& bitmap, int offsetY, uint16& changeCounter)
{
	if (changeCounter == palette.getChangeCounter())
		return false;

	// Copy over the palette data
	uint32* dst = bitmap.getPixelPointer(0, offsetY);
	palette.dumpColors(dst, palette.getSize());

	changeCounter = palette.getChangeCounter();
	return true;
}

#endif
