/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/sprite/PaletteSprite.h"


namespace
{
	Vec2i roundUpToMultipleOf4(const Vec2i& vec)
	{
		// Assuming positive values for x and y
		return Vec2i((vec.x + 3) & 0xfffffffc, (vec.y + 3) & 0xfffffffc);
	}

	void applyPalette(Bitmap& output, int& outputReservedSize, const PaletteBitmap& input, const uint32* palette)
	{
		output.createReusingMemory(input.mWidth, input.mHeight, outputReservedSize);
		for (int i = 0; i < output.getPixelCount(); ++i)
		{
			output.getData()[i] = palette[input.mData[i]];
		}
	}

	void applyScale3x(PaletteBitmap& output, const PaletteBitmap& input)
	{
		// Based on https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms#Scale3%C3%97/AdvMAME3%C3%97_and_ScaleFX
		output.create(input.mWidth * 3, input.mHeight * 3);
		for (uint32 y = 0; y < input.mHeight; ++y)
		{
			const uint32 y0 = std::max<uint32>(y, 1) - 1;
			const uint32 y1 = y;
			const uint32 y2 = std::min<uint32>(y + 1, input.mHeight - 1);

			const uint8* inputLine0 = &input.mData[y0 * input.mWidth];
			const uint8* inputLine1 = &input.mData[y1 * input.mWidth];
			const uint8* inputLine2 = &input.mData[y2 * input.mWidth];

			uint8* outputLine0 = &output.mData[(y * 3) * output.mWidth];
			uint8* outputLine1 = &output.mData[(y * 3 + 1) * output.mWidth];
			uint8* outputLine2 = &output.mData[(y * 3 + 2) * output.mWidth];

			for (uint32 x = 0; x < input.mWidth; ++x)
			{
				const uint32 x0 = std::max<uint32>(x, 1) - 1;
				const uint32 x1 = x;
				const uint32 x2 = std::min<uint32>(x + 1, input.mWidth - 1);

				const uint8 A = inputLine0[x0];
				const uint8 B = inputLine0[x1];
				const uint8 C = inputLine0[x2];
				const uint8 D = inputLine1[x0];
				const uint8 E = inputLine1[x1];
				const uint8 F = inputLine1[x2];
				const uint8 G = inputLine2[x0];
				const uint8 H = inputLine2[x1];
				const uint8 I = inputLine2[x2];

				outputLine0[0] = (D==B && D!=H && B!=F) ? D : E;
				outputLine0[1] = (D==B && D!=H && B!=F && E!=C) || (B==F && B!=D && F!=H && E!=A) ? B : E;
				outputLine0[2] = (B==F && B!=D && F!=H) ? F : E;
				outputLine1[0] = (H==D && H!=F && D!=B && E!=A) || (D==B && D!=H && B!=F && E!=G) ? D : E;
				outputLine1[1] = E;
				outputLine1[2] = (B==F && B!=D && F!=H && E!=I) || (F==H && F!=B && H!=D && E!=C) ? F : E;
				outputLine2[0] = (H==D && H!=F && D!=B) ? D : E;
				outputLine2[1] = (F==H && F!=B && H!=D && E!=G) || (H==D && H!=F && D!=B && E!=I) ? H : E;
				outputLine2[2] = (F==H && F!=B && H!=D) ? F : E;

				outputLine0 += 3;
				outputLine1 += 3;
				outputLine2 += 3;
			}
		}
	}
}


void PaletteSprite::clear()
{
	mBitmap.clear(0);
}

void PaletteSprite::create(const Vec2i& size, const Vec2i& offset)
{
	mBitmap.create(size.x, size.y);
	mOffset = offset;
}

void PaletteSprite::createFromBitmap(const PaletteBitmap& bitmap, const Vec2i& offset)
{
	// Round size to next multiple of 4, which is needed when not using buffer textures
	const Vec2i size = roundUpToMultipleOf4(bitmap.getSize());
	if (size != bitmap.getSize())
	{
		mBitmap.create(size.x, size.y, 0);
		mBitmap.copyRect(bitmap, Recti(0, 0, size.x, size.y));
	}
	else
	{
		mBitmap.copy(bitmap);
	}
	mOffset = offset;
}

void PaletteSprite::createFromBitmap(PaletteBitmap&& bitmap, const Vec2i& offset)
{
	// Round size to next multiple of 4, which is needed when not using buffer textures
	const Vec2i size = roundUpToMultipleOf4(bitmap.getSize());
	if (size != bitmap.getSize())
	{
		mBitmap.create(size.x, size.y, 0);
		mBitmap.copyRect(bitmap, Recti(0, 0, size.x, size.y));
	}
	else
	{
		mBitmap.swap(bitmap);
	}
	mOffset = offset;
}

void PaletteSprite::createFromBitmap(const PaletteBitmap& bitmap, const Recti& sourceRect, const Vec2i& offset)
{
	// Round size to next multiple of 4, which is needed when not using buffer textures
	const Vec2i size = roundUpToMultipleOf4(sourceRect.getSize());
	if (size != sourceRect.getSize())
	{
		mBitmap.create(size.x, size.y, 0);
	}
	else
	{
		mBitmap.create(size.x, size.y);
	}
	mBitmap.copyRect(bitmap, sourceRect);
	mOffset = offset;
}

void PaletteSprite::createFromSpritePatterns(const std::vector<RenderUtils::SinglePattern>& patterns)
{
	if (patterns.empty())
		return;

	// Get bounding box of patterns
	int minX = 0xffff;
	int maxX = -0xffff;
	int minY = 0xffff;
	int maxY = -0xffff;
	for (const RenderUtils::SinglePattern& pattern : patterns)
	{
		minX = std::min(minX, pattern.mOffsetX);
		maxX = std::max(maxX, pattern.mOffsetX + 8);
		minY = std::min(minY, pattern.mOffsetY);
		maxY = std::max(maxY, pattern.mOffsetY + 8);
	}

	// Round size to next multiple of 4, which is needed when not using buffer textures
	maxX = minX + (maxX - minX + 3) / 4 * 4;
	maxY = minY + (maxY - minY + 3) / 4 * 4;

	// Create sprite
	create(Vec2i(maxX - minX, maxY - minY), Vec2i(minX, minY));
	clear();

	// Fill
	RenderUtils::blitSpritePatterns(mBitmap, -minX, -minY, patterns);
}

void PaletteSprite::blitInto(PaletteBitmap& output, const Vec2i& position) const
{
	int px = position.x + mOffset.x;
	int py = position.y + mOffset.y;

	const int minX = std::max<int>(0, -px);
	const int maxX = std::min<int>(mBitmap.mWidth, output.mWidth - px);
	const int minY = std::max<int>(0, -py);
	const int maxY = std::min<int>(mBitmap.mHeight, output.mHeight - py);

	for (int iy = minY; iy < maxY; ++iy)
	{
		const uint8* src = &mBitmap.mData[minX + iy * mBitmap.mWidth];
		uint8* dst = &output.mData[(px + minX) + (py + iy) * output.mWidth];

		for (int ix = minX; ix < maxX; ++ix)
		{
			// Check for transparency
			if (*src & 0x0f)
			{
				*dst = *src;
			}
			++src;
			++dst;
		}
	}
}

void PaletteSprite::blitInto(Bitmap& output, const Vec2i& position, const uint32* palette, const BlitOptions& blitOptions) const
{
	// We are converting the palette bitmap to RGBA, so we can use the same functionality as for component sprites
	static Bitmap tempBitmap;
	static int tempBitmapSize = 0;
	applyPalette(tempBitmap, tempBitmapSize, blitOptions.mUseUpscaledSprite ? getUpscaledBitmap() : mBitmap, palette);

	SpriteBase::blitInto(output, tempBitmap, position, blitOptions);
}

const PaletteBitmap& PaletteSprite::getUpscaledBitmap() const
{
	if (mUpscaledBitmap.empty())
	{
		// Apply Scale3x to get an upscaled version
		applyScale3x(mUpscaledBitmap, mBitmap);
	}
	return mUpscaledBitmap;
}
