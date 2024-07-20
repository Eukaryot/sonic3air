/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/Renderer.h"
#include "oxygen/drawing/software/Blitter.h"

class PlaneGeometry;
class SpriteGeometry;
namespace detail
{
	class PixelBlockWriter;
}


class SoftwareRenderer : public Renderer
{
friend class detail::PixelBlockWriter;

public:
	static constexpr int8 RENDERER_TYPE_ID = 0x10;

public:
	SoftwareRenderer(RenderParts& renderParts, DrawerTexture& outputTexture);

	virtual void initialize() override;
	virtual void reset() override;
	virtual void setGameResolution(const Vec2i& gameResolution) override;
	virtual void clearGameScreen() override;
	virtual void renderGameScreen(const std::vector<Geometry*>& geometries) override;
	virtual void renderDebugDraw(int debugDrawMode, const Recti& rect) override;

private:
	void renderGeometry(const Geometry& geometry);
	void renderPlane(const PlaneGeometry& geometry);
	void renderSprite(const SpriteGeometry& geometry);

private:
	Vec2i mGameResolution;
	Bitmap mGameScreenCopy;

	uint8 mDepthBuffer[0x20000] = { 0 };	// 512x256 pixels
	bool mEmptyDepthBuffer = true;			// Stays true until first non-zero depth value was written

	Recti mCurrentViewport;
	bool mFullViewport = true;

	struct BufferedPlaneData
	{
		struct PixelBlock
		{
			Vec2i mStartCoords;
			int mLinearPosition = 0;	// Linear position on screen / in content
			int mNumPixels = 0;			// Pixels of horizontal line
			uint8 mAtex = 0;
			uint8 mPaletteIndex = 0;
		};

		bool mValid = false;
		int mPlaneIndex = 0;
		int mScrollOffsets = 0;
		Recti mActiveRect;

		std::vector<uint8> mContent;
		std::vector<PixelBlock> mPrioBlocks;
		std::vector<PixelBlock> mNonPrioBlocks;
	};
	static const constexpr int MAX_BUFFER_PLANE_DATA = 8;
	BufferedPlaneData mBufferedPlaneData[MAX_BUFFER_PLANE_DATA];

	Blitter mBlitter;
};
