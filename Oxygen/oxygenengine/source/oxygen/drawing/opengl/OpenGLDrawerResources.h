/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/parts/palette/Palette.h"
#include "oxygen/drawing/opengl/OpenGLTexture.h"

class SimpleRectColoredShader;
class SimpleRectVertexColorShader;
class SimpleRectTexturedShader;
class SimpleRectTexturedUVShader;
class SimpleRectIndexedShader;


class OpenGLDrawerResources final
{
friend class OpenGLRenderResources;		// For access to updatePalette

public:
	OpenGLDrawerResources();
	~OpenGLDrawerResources();

	void startup();
	void shutdown();

	void clearAllCaches();
	void refresh(float deltaSeconds);

	inline BlendMode getBlendMode() const  { return mState.mBlendMode; }
	void setBlendMode(BlendMode blendMode);

	SimpleRectColoredShader& getSimpleRectColoredShader();
	SimpleRectVertexColorShader& getSimpleRectVertexColorShader();
	SimpleRectTexturedShader& getSimpleRectTexturedShader(bool tint, bool alpha);
	SimpleRectTexturedUVShader& getSimpleRectTexturedUVShader(bool tint, bool alpha);
	SimpleRectIndexedShader& getSimpleRectIndexedShader(bool tint, bool alpha);

	opengl::VertexArrayObject& getSimpleQuadVAO();

	const OpenGLTexture& getCustomPaletteTexture(const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette);
	const Vec2i& getPaletteTextureSize() const;

private:
	struct State
	{
		BlendMode mBlendMode = BlendMode::OPAQUE;
	};

	struct PaletteData
	{
		Bitmap		  mBitmap;
		OpenGLTexture mTexture;
		uint16		  mChangeCounters[2] = { 0 };
		float		  mSecondsSinceLastUse = 0.0f;
	};

private:
	bool updatePalette(PaletteData& data, const PaletteBase& primaryPalette, const PaletteBase& secondaryPalette);
	bool updatePaletteBitmap(const PaletteBase& palette, Bitmap& bitmap, int offsetY, uint16& changeCounter);

private:
	State mState;

	// Internal (shaders etc.)
	struct Internal;
	Internal& mInternal;

	// Paletteb cache
	std::unordered_map<uint64, PaletteData> mCustomPalettes;	// Using a key built from a combination of primary and secondary palette keys
	float mSecondsSinceLastPaletteCleanup = 0.0f;
};

#endif
