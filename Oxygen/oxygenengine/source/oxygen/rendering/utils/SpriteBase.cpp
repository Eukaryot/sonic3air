/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/utils/SpriteBase.h"


namespace spritebaseinternal
{
	uint32 FORCE_INLINE blendColors(uint32 src, uint32 dst)
	{
		const uint8* srcPtr = (uint8*)&src;
		uint8* dstPtr = (uint8*)&dst;
		const int alpha = srcPtr[3];
		const int oneMinusAlpha = 0x100 - alpha;

		dstPtr[0] = ((int)srcPtr[0] * alpha + (int)dstPtr[0] * oneMinusAlpha) >> 8;
		dstPtr[1] = ((int)srcPtr[1] * alpha + (int)dstPtr[1] * oneMinusAlpha) >> 8;
		dstPtr[2] = ((int)srcPtr[2] * alpha + (int)dstPtr[2] * oneMinusAlpha) >> 8;
		return dst | 0xff000000;
	}


	template<bool USE_ALPHA, bool DEPTH_TEST>
	void blitLine_withTintAdd(uint32* dst, const uint32* src, int numPixels, const uint8* depthBuffer, uint8 depthValue, const Color& addedColor, const Color& tintColor)
	{
		for (int i = 0; i < numPixels; ++i)
		{
			uint32 pixel = *src;
			++src;

			// Check for transparency
			if (!USE_ALPHA || (pixel & 0xff000000) != 0)
			{
				// Depth test
				if (!DEPTH_TEST || depthValue >= depthBuffer[i])
				{
					// Tint color and alpha transparency
					Color color = Color::fromABGR32(pixel);
					color.r = saturate(addedColor.r + color.r * tintColor.r);
					color.g = saturate(addedColor.g + color.g * tintColor.g);
					color.b = saturate(addedColor.b + color.b * tintColor.b);
					if (USE_ALPHA)
						color.a = saturate(color.a * tintColor.a);
					else
						color.a = tintColor.a;

					if (color.a < 1.0f)
					{
						// TODO: Use "blendColors" function from above, and integer arithmetic in general here
						color = color.blendOver(Color::fromABGR32(*dst | 0xff000000));
						color.a = 1.0f;
					}
					pixel = color.getABGR32();

					*dst = pixel;
				}
			}
			++dst;
		}
	}

	template<bool USE_ALPHA, bool DEPTH_TEST>
	void blitLine_noTintAdd(uint32* dst, const uint32* src, int numPixels, const uint8* depthBuffer, uint8 depthValue)
	{
		for (int i = 0; i < numPixels; ++i)
		{
			uint32 pixel = *src;
			++src;

			// Check for transparency
			if (!USE_ALPHA || (pixel & 0xff000000) != 0)
			{
				// Depth test
				if (!DEPTH_TEST || depthValue >= depthBuffer[i])
				{
					// Consider alpha transparency
					if (USE_ALPHA && (pixel & 0xff000000) != 0xff000000)
					{
						pixel = blendColors(pixel , *dst);
					}

					*dst = pixel;
				}
			}
			++dst;
		}
	}

	void blitLine_simple(uint32* dst, const uint32* src, int numPixels)
	{
		memcpy(dst, src, numPixels * sizeof(uint32));
	}

	void blitSpriteNoTransform(Bitmap& destBitmap, const Vec2i& destPosition, const Bitmap& sourceBitmap, const Vec2i& offset, const SpriteBase::BlitOptions& blitOptions)
	{
		const bool useTintAdd = (nullptr != blitOptions.mTintColor || nullptr != blitOptions.mAddedColor);
		const Color tintColor = (nullptr == blitOptions.mTintColor) ? Color::WHITE : *blitOptions.mTintColor;
		const Color addedColor = (nullptr == blitOptions.mAddedColor) ? Color::TRANSPARENT : *blitOptions.mAddedColor;

		const int px = destPosition.x + offset.x;
		const int py = destPosition.y + offset.y;

		Recti bbox(px, py, sourceBitmap.mWidth, sourceBitmap.mHeight);
		bbox.intersect(Recti(0, 0, destBitmap.mWidth, destBitmap.mHeight));
		if (nullptr != blitOptions.mTargetRect)
		{
			bbox.intersect(*blitOptions.mTargetRect);
			if (bbox.empty())
				return;
		}

		const int minX = bbox.x;
		const int minY = bbox.y;
		const int maxY = bbox.y + bbox.height;

		for (int iy = minY; iy < maxY; ++iy)
		{
			uint32* dst = &destBitmap.mData[minX + iy * destBitmap.mWidth];
			const uint32* src = &sourceBitmap.mData[(minX - px) + (iy - py) * sourceBitmap.mWidth];

			if (!useTintAdd)
			{
				if (blitOptions.mIgnoreAlpha)
				{
					if (nullptr == blitOptions.mDepthBuffer)
					{
						blitLine_simple(dst, src, bbox.width);
					}
					else
					{
						blitLine_noTintAdd<false, true>(dst, src, bbox.width, &blitOptions.mDepthBuffer[minX + iy * 0x200], blitOptions.mDepthValue);
					}
				}
				else
				{
					if (nullptr == blitOptions.mDepthBuffer)
					{
						blitLine_noTintAdd<true, false>(dst, src, bbox.width, nullptr, 0);
					}
					else
					{
						blitLine_noTintAdd<true, true>(dst, src, bbox.width, &blitOptions.mDepthBuffer[minX + iy * 0x200], blitOptions.mDepthValue);
					}
				}
			}
			else
			{
				if (blitOptions.mIgnoreAlpha)
				{
					if (nullptr == blitOptions.mDepthBuffer)
					{
						blitLine_withTintAdd<false, false>(dst, src, bbox.width, nullptr, 0, addedColor, tintColor);
					}
					else
					{
						blitLine_withTintAdd<false, true>(dst, src, bbox.width, &blitOptions.mDepthBuffer[minX + iy * 0x200], blitOptions.mDepthValue, addedColor, tintColor);
					}
				}
				else
				{
					if (nullptr == blitOptions.mDepthBuffer)
					{
						blitLine_withTintAdd<true, false>(dst, src, bbox.width, nullptr, 0, addedColor, tintColor);
					}
					else
					{
						blitLine_withTintAdd<true, true>(dst, src, bbox.width, &blitOptions.mDepthBuffer[minX + iy * 0x200], blitOptions.mDepthValue, addedColor, tintColor);
					}
				}
			}
		}
	}

	void blitSpriteWithTransform(Bitmap& destBitmap, const Vec2i& destPosition, const Bitmap& sourceBitmap, const Vec2i& offset, const SpriteBase::BlitOptions& blitOptions)
	{
		const bool useTintAdd = (nullptr != blitOptions.mTintColor || nullptr != blitOptions.mAddedColor);
		const Color tintColor = (nullptr == blitOptions.mTintColor) ? Color::WHITE : *blitOptions.mTintColor;
		const Color addedColor = (nullptr == blitOptions.mAddedColor) ? Color::TRANSPARENT : *blitOptions.mAddedColor;

		Recti bbox;
		{
			Vec2f min(1e10f, 1e10f);
			Vec2f max(-1e10f, -1e10f);
			const Vec2i corners[4] = { Vec2i(0, 0), Vec2i(sourceBitmap.mWidth - 1, 0), Vec2i(0, sourceBitmap.mHeight - 1), Vec2i(sourceBitmap.mWidth - 1, sourceBitmap.mHeight - 1) };
			for (int i = 0; i < 4; ++i)
			{
				const Vec2f localCorner((float)(corners[i].x + offset.x + 0.5f), (float)(corners[i].y + offset.y + 0.5f));
				const float screenCornerX = destPosition.x + localCorner.x * blitOptions.mTransform[0] + localCorner.y * blitOptions.mTransform[1];
				const float screenCornerY = destPosition.y + localCorner.x * blitOptions.mTransform[2] + localCorner.y * blitOptions.mTransform[3];
				min.x = std::min(screenCornerX, min.x);
				min.y = std::min(screenCornerY, min.y);
				max.x = std::max(screenCornerX, max.x);
				max.y = std::max(screenCornerY, max.y);
			}

			bbox.x = (int)min.x;
			bbox.y = (int)min.y;
			bbox.width = (int)max.x + 1 - bbox.x;
			bbox.height = (int)max.y + 1 - bbox.y;
		}
		bbox.intersect(Recti(0, 0, destBitmap.mWidth, destBitmap.mHeight));
		if (nullptr != blitOptions.mTargetRect)
		{
			bbox.intersect(*blitOptions.mTargetRect);
			if (bbox.empty())
				return;
		}

		const int minX = bbox.x;
		const int maxX = bbox.x + bbox.width;
		const int minY = bbox.y;
		const int maxY = bbox.y + bbox.height;

		for (int iy = minY; iy < maxY; ++iy)
		{
			uint32* dst = &destBitmap.mData[minX + iy * destBitmap.mWidth];

			for (int ix = minX; ix < maxX; ++ix)
			{
				uint32 pixel = 0;
				{
					// Transform into palette sprite coordinates
					const float dx = (float)(ix - destPosition.x) + 0.5f;
					const float dy = (float)(iy - destPosition.y) + 0.5f;
					const int localX = roundToInt(dx * blitOptions.mInvTransform[0] + dy * blitOptions.mInvTransform[1] - 0.5f) - offset.x;
					const int localY = roundToInt(dx * blitOptions.mInvTransform[2] + dy * blitOptions.mInvTransform[3] - 0.5f) - offset.y;

					if (localX >= 0 && localX < (int)sourceBitmap.mWidth &&
						localY >= 0 && localY < (int)sourceBitmap.mHeight)
					{
						pixel = sourceBitmap.mData[localX + localY * sourceBitmap.mWidth];
					}
				}

				// Check for transparency
				if ((pixel & 0xff000000) != 0 && !blitOptions.mIgnoreAlpha)
				{
					// Depth test
					if (nullptr == blitOptions.mDepthBuffer || blitOptions.mDepthValue >= blitOptions.mDepthBuffer[ix + iy * 0x200])
					{
						if (!useTintAdd)
						{
							// Consider alpha transparency
							if ((pixel & 0xff000000) != 0xff000000)
							{
								pixel = blendColors(pixel , *dst);
							}
						}
						else
						{
							// Tint color and alpha transparency
							Color color = Color::fromABGR32(pixel);
							color.r = saturate(addedColor.r + color.r * tintColor.r);
							color.g = saturate(addedColor.g + color.g * tintColor.g);
							color.b = saturate(addedColor.b + color.b * tintColor.b);
							color.a = saturate(color.a * tintColor.a);

							if (color.a < 1.0f)
							{
								// TODO: Use "blendColors" function from above, and integer arithmetic in general here
								color = color.blendOver(Color::fromABGR32(*dst | 0xff000000));
								color.a = 1.0f;
							}
							pixel = color.getABGR32();
						}

						*dst = pixel;
					}
				}
				++dst;
			}
		}
	}
}


void SpriteBase::blitInto(Bitmap& output, const Bitmap& input, const Vec2i& position, const BlitOptions& blitOptions) const
{
	if (nullptr == blitOptions.mTransform)
	{
		spritebaseinternal::blitSpriteNoTransform(output, position, input, mOffset, blitOptions);
	}
	else
	{
		spritebaseinternal::blitSpriteWithTransform(output, position, input, mOffset, blitOptions);
	}
}
