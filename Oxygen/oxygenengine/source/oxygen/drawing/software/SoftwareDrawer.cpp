/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/software/SoftwareDrawer.h"
#include "oxygen/drawing/software/SoftwareDrawerTexture.h"
#include "oxygen/drawing/software/SoftwareRasterizer.h"
#include "oxygen/drawing/software/Blitter.h"
#include "oxygen/drawing/DrawCollection.h"
#include "oxygen/drawing/DrawCommand.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/resources/SpriteCollection.h"


namespace softwaredrawer
{
	const char* getSDLPixelFormatText(uint32 format)
	{
		switch (format)
		{
			case SDL_PIXELFORMAT_INDEX1LSB:		return "SDL_PIXELFORMAT_INDEX1LSB";
			case SDL_PIXELFORMAT_INDEX1MSB:		return "SDL_PIXELFORMAT_INDEX1MSB";
			case SDL_PIXELFORMAT_INDEX4LSB:		return "SDL_PIXELFORMAT_INDEX4LSB";
			case SDL_PIXELFORMAT_INDEX4MSB:		return "SDL_PIXELFORMAT_INDEX4MSB";
			case SDL_PIXELFORMAT_INDEX8:		return "SDL_PIXELFORMAT_INDEX8";
			case SDL_PIXELFORMAT_RGB332:		return "SDL_PIXELFORMAT_RGB332";
			case SDL_PIXELFORMAT_RGB444:		return "SDL_PIXELFORMAT_RGB444";
			case SDL_PIXELFORMAT_RGB555:		return "SDL_PIXELFORMAT_RGB555";
			case SDL_PIXELFORMAT_BGR555:		return "SDL_PIXELFORMAT_BGR555";
			case SDL_PIXELFORMAT_ARGB4444:		return "SDL_PIXELFORMAT_ARGB4444";
			case SDL_PIXELFORMAT_RGBA4444:		return "SDL_PIXELFORMAT_RGBA4444";
			case SDL_PIXELFORMAT_ABGR4444:		return "SDL_PIXELFORMAT_ABGR4444";
			case SDL_PIXELFORMAT_BGRA4444:		return "SDL_PIXELFORMAT_BGRA4444";
			case SDL_PIXELFORMAT_ARGB1555:		return "SDL_PIXELFORMAT_ARGB1555";
			case SDL_PIXELFORMAT_RGBA5551:		return "SDL_PIXELFORMAT_RGBA5551";
			case SDL_PIXELFORMAT_ABGR1555:		return "SDL_PIXELFORMAT_ABGR1555";
			case SDL_PIXELFORMAT_BGRA5551:		return "SDL_PIXELFORMAT_BGRA5551";
			case SDL_PIXELFORMAT_RGB565:		return "SDL_PIXELFORMAT_RGB565";
			case SDL_PIXELFORMAT_BGR565:		return "SDL_PIXELFORMAT_BGR565";
			case SDL_PIXELFORMAT_RGB24:			return "SDL_PIXELFORMAT_RGB24";
			case SDL_PIXELFORMAT_BGR24:			return "SDL_PIXELFORMAT_BGR24";
			case SDL_PIXELFORMAT_RGB888:		return "SDL_PIXELFORMAT_RGB888";
			case SDL_PIXELFORMAT_RGBX8888:		return "SDL_PIXELFORMAT_RGBX88881";
			case SDL_PIXELFORMAT_BGR888	:		return "SDL_PIXELFORMAT_BGR888";
			case SDL_PIXELFORMAT_BGRX8888:		return "SDL_PIXELFORMAT_BGRX8888";
			case SDL_PIXELFORMAT_ARGB8888:		return "SDL_PIXELFORMAT_ARGB8888";
			case SDL_PIXELFORMAT_RGBA8888:		return "SDL_PIXELFORMAT_RGBA8888";
			case SDL_PIXELFORMAT_ABGR8888:		return "SDL_PIXELFORMAT_ABGR8888";
			case SDL_PIXELFORMAT_BGRA8888:		return "SDL_PIXELFORMAT_BGRA8888";
			case SDL_PIXELFORMAT_ARGB2101010:	return "SDL_PIXELFORMAT_ARGB2101010";
			case SDL_PIXELFORMAT_YV12:			return "SDL_PIXELFORMAT_YV12";
			case SDL_PIXELFORMAT_IYUV:			return "SDL_PIXELFORMAT_IYUV";
			case SDL_PIXELFORMAT_YUY2:			return "SDL_PIXELFORMAT_YUY2";
			case SDL_PIXELFORMAT_UYVY:			return "SDL_PIXELFORMAT_UYVY";
			case SDL_PIXELFORMAT_YVYU:			return "SDL_PIXELFORMAT_YVYU";
			case SDL_PIXELFORMAT_NV12:			return "SDL_PIXELFORMAT_NV12";
			case SDL_PIXELFORMAT_NV21:			return "SDL_PIXELFORMAT_NV21";
		}
		return "<unresolved>";
	}

	void applyTintToBitmap(Bitmap& destBitmap, const Color& tintColor)
	{
		const uint16 multiplicators[4] =
		{
			(uint16)roundToInt(tintColor.r * 256.0f),
			(uint16)roundToInt(tintColor.g * 256.0f),
			(uint16)roundToInt(tintColor.b * 256.0f),
			(uint16)roundToInt(tintColor.a * 256.0f)
		};

		const uint32 numPixels = destBitmap.getPixelCount();
		uint8* dst = (uint8*)destBitmap.getData();

		for (uint32 i = 0; i < numPixels; ++i)
		{
			dst[0] = (dst[0] * multiplicators[0]) >> 8;
			dst[1] = (dst[1] * multiplicators[1]) >> 8;
			dst[2] = (dst[2] * multiplicators[2]) >> 8;
			dst[3] = (dst[3] * multiplicators[3]) >> 8;
			dst += 4;
		}
	}

	void mirrorBitmapX(Bitmap& destBitmap, int& destReservedSize, const Bitmap& sourceBitmap)
	{
		destBitmap.createReusingMemory(sourceBitmap.getSize(), destReservedSize);
		for (int y = 0; y < sourceBitmap.getHeight(); ++y)
		{
			const uint32* src = sourceBitmap.getPixelPointer(sourceBitmap.getWidth() - 1, y);
			uint32* dst = destBitmap.getPixelPointer(0, y);

			for (int x = 0; x < sourceBitmap.getWidth(); ++x)
			{
				*dst = *src;
				--src;
				++dst;
			}
		}
	}


	struct Internal
	{
	public:
		Internal()
		{
		}

		~Internal()
		{
		}

		void setupScreenSurface(SDL_Window* window)
		{
			mOutputWindow = window;

			mScreenSurface = SDL_GetWindowSurface(window);
			RMX_CHECK(nullptr != mScreenSurface, "Could not get SDL screen surface", return);

			bool formatSupported = false;
			switch (mScreenSurface->format->format)
			{
				case SDL_PIXELFORMAT_RGB888:		// Used in my Windows 10
				case SDL_PIXELFORMAT_ARGB8888:		// Used in my Linux Mint
				{
					formatSupported = true;
					mSurfaceSwapRedBlue = true;
					break;
				}

				case SDL_PIXELFORMAT_BGR888:
				case SDL_PIXELFORMAT_ABGR8888:
				{
					formatSupported = true;
					mSurfaceSwapRedBlue = false;
					break;
				}

				// TODO: We could also support e.g. SDL_PIXELFORMAT_RGBA8888 by shifting everything by one byte in "getOutputWrapper"
			}

			if (formatSupported)
			{
				// Success, format is supported
				mScissorRect.set(0, 0, mScreenSurface->w, mScreenSurface->h);
			}
			else
			{
				// Fallback when not able to use the SDL surface directly
				if (!mDisplayedFormatWarning)
				{
					RMX_ERROR("Unsupported screen surface format " << getSDLPixelFormatText(mScreenSurface->format->format) << " (" << rmx::hexString(mScreenSurface->format->format, 8) << ")", );
					mDisplayedFormatWarning = true;
				}

				mScreenSurface = nullptr;
				mOutputWrapper = BitmapViewMutable<uint32>();
			}
		}

		SoftwareDrawerTexture* createTexture(DrawerTexture& outTexture)
		{
			SoftwareDrawerTexture* texture = new SoftwareDrawerTexture(outTexture);
			return texture;
		}

		void setCurrentRenderTarget(DrawerTexture* renderTarget)
		{
			RMX_ASSERT(mScissorStack.empty(), "Changed render targets with non-empty scissor stack");
			if (nullptr == mCurrentRenderTarget)
			{
				unlockScreenSurface();
			}

			mCurrentRenderTarget = renderTarget;

			if (nullptr == mCurrentRenderTarget)
			{
				// Next call to "getOutputWrapper" will update the output wrapper accordingly
				mScissorRect.set(0, 0, mScreenSurface->w, mScreenSurface->h);
			}
			else
			{
				mOutputWrapper = BitmapViewMutable<uint32>(mCurrentRenderTarget->accessBitmap());
				mScissorRect.set(Vec2i(), mCurrentRenderTarget->getSize());
			}
		}

		DrawerTexture* getCurrentRenderTarget() const
		{
			return mCurrentRenderTarget;
		}

		BitmapViewMutable<uint32>& getOutputWrapper()
		{
			if (nullptr == mCurrentRenderTarget)
			{
				if (nullptr != mScreenSurface)
				{
					if (!mIsScreenSurfaceLocked)
					{
						SDL_LockSurface(mScreenSurface);
						mIsScreenSurfaceLocked = true;
						mOutputWrapper = BitmapViewMutable<uint32>((uint32*)mScreenSurface->pixels, Vec2i(mScreenSurface->w, mScreenSurface->h));
					}
				}
				else
				{
					// Handle invalid screen surface
					mOutputWrapper = BitmapViewMutable<uint32>();
				}
			}
			return mOutputWrapper;
		}

		bool needSwapRedBlueChannels()
		{
			return (nullptr == mCurrentRenderTarget && mSurfaceSwapRedBlue);
		}

		void swapRedBlueChannels(uint32* dst, const uint32* src, int numPixels)
		{
			// Copy over data and swap red and blue channels
			//  -> Note that input and output may be identical, or must not overlap otherwise
			int k = 0;
			if constexpr (sizeof(void*) == 8)
			{
			#if !defined(PLATFORM_VITA)
				// On 64-bit architectures: Process 2 pixels at once
				for (; k < numPixels; k += 2)
				{
					const uint64 colors = *(uint64*)&src[k];
					*(uint64*)&dst[k] = ((colors & 0x00ff000000ff0000ull) >> 16) | (colors & 0xff00ff00ff00ff00ull) | ((colors & 0x000000ff000000ffull) << 16);
				}
			#endif
			}
			// Process single pixels
			for (; k < numPixels; ++k)
			{
				const uint32 color = src[k];
				dst[k] = ((color & 0x00ff0000) >> 16) | (color & 0xff00ff00) | ((color & 0x000000ff) << 16);
			}
		}

		void setupRedBlueSwappedBitmapWrapper(BitmapViewMutable<uint32>& bitmapWrapper)
		{
			// Setup temp buffer with the right size
			mTempBuffer.createReusingMemory(bitmapWrapper.getSize().x, bitmapWrapper.getSize().y, mTempReservedSize);

			// Copy over data and swap red and blue channels
			swapRedBlueChannels(mTempBuffer.getData(), bitmapWrapper.getData(), mTempBuffer.getPixelCount());

			// Use the temp buffer as replacement for the bitmap wrapper
			bitmapWrapper = mTempBuffer;
		}

		void unlockScreenSurface()
		{
			if (nullptr != mScreenSurface && mIsScreenSurfaceLocked)
			{
				SDL_UnlockSurface(mScreenSurface);
				mIsScreenSurfaceLocked = false;
			}
		}

		const Recti& getScissorRect() const
		{
			return mScissorRect;
		}

		bool useAlphaBlending() const
		{
			return (mCurrentBlendMode == BlendMode::ALPHA);
		}

		void drawRect(Recti targetRect, Bitmap* inputBitmap, Color color, Vec2f uv0 = Vec2f(0.0f, 0.0f), Vec2f uv1 = Vec2f(1.0f, 1.0f))
		{
			// Target rect to fill in the output
			bool mirrorX = false;
			if (targetRect.width < 0)
			{
				// Mirror horizontally
				mirrorX = true;
				targetRect.x += targetRect.width;
				targetRect.width = -targetRect.width;
			}
			const Recti uncroppedRect = targetRect;
			targetRect.intersect(targetRect, getScissorRect());

			if (targetRect.empty())
				return;

			if (nullptr != inputBitmap)
			{
				// Consider mirroring
				if (mirrorX)
				{
					softwaredrawer::mirrorBitmapX(mTempBuffer, mTempReservedSize, *inputBitmap);
					inputBitmap = &mTempBuffer;
				}

				BitmapViewMutable<uint32> inputWrapper(*inputBitmap);

				const Vec2f originalUVStart = uv0;
				const Vec2f originalUVRange = uv1 - uv0;
				Recti inputRect;
				bool useUVs = false;
				{
					// Calculate actual UVs that also take cropping into account
					if (targetRect != uncroppedRect)
					{
						const Vec2f relativeStart = Vec2f(targetRect.getPos() - uncroppedRect.getPos()) / Vec2f(uncroppedRect.getSize());
						const Vec2f relativeEnd   = Vec2f(targetRect.getPos() - uncroppedRect.getPos() + targetRect.getSize()) / Vec2f(uncroppedRect.getSize());
						uv0 = originalUVStart + relativeStart * originalUVRange;
						uv1 = originalUVStart + relativeEnd * originalUVRange;
					}

					useUVs = (uv0.x < 0.0f || uv0.x > uv1.x || uv1.x > 1.0f || uv0.y < 0.0f || uv0.y > uv1.y || uv1.y > 1.0f);

					// Get the part from the input that will get drawn
					const Vec2f inputStart = uv0 * Vec2f(inputWrapper.getSize());
					const Vec2f inputEnd = uv1 * Vec2f(inputWrapper.getSize());
					inputRect.x = roundToInt(inputStart.x);
					inputRect.y = roundToInt(inputStart.y);
					inputRect.width = roundToInt(inputEnd.x) - inputRect.x;
					inputRect.height = roundToInt(inputEnd.y) - inputRect.y;
				}

				if (useUVs || targetRect.getSize() != inputRect.getSize())
				{
					if (needSwapRedBlueChannels())
					{
						setupRedBlueSwappedBitmapWrapper(inputWrapper);
					}

					Blitter::Options options;
					options.mBlendMode = useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE;
					options.mTintColor = (color == Color::WHITE) ? nullptr : &color;

					if (useUVs)
					{
						mBlitter.blitRectWithUVs(getOutputWrapper(), targetRect, inputWrapper, inputRect, options);
					}
					else
					{
						mBlitter.blitRectWithScaling(getOutputWrapper(), targetRect, inputWrapper, inputRect, options);
					}
				}
				else
				{
					// Get the part from the input that will get drawn
					//  -> Calculated here again, as we need to use the original UVs in this case
					const Vec2f inputStart = originalUVStart * Vec2f(inputWrapper.getSize());
					const Vec2f inputEnd = (originalUVStart + originalUVRange) * Vec2f(inputWrapper.getSize());
					inputRect.x = roundToInt(inputStart.x);
					inputRect.y = roundToInt(inputStart.y);
					inputRect.width = roundToInt(inputEnd.x) - inputRect.x;
					inputRect.height = roundToInt(inputEnd.y) - inputRect.y;

					Blitter::Options blitterOptions;
					blitterOptions.mBlendMode = useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE;
					blitterOptions.mTintColor = (color != Color::WHITE) ? &color : nullptr;
					blitterOptions.mSwapRedBlueChannels = needSwapRedBlueChannels();

					mBlitter.blitSprite(Blitter::OutputWrapper(getOutputWrapper(), getScissorRect()), Blitter::SpriteWrapper(inputWrapper.getData(), inputWrapper.getSize(), Vec2i(), inputRect), uncroppedRect.getPos() - inputRect.getPos(), blitterOptions);
				}
			}
			else
			{
				if (needSwapRedBlueChannels())
					color.swapRedBlue();

				const BitmapViewMutable<uint32> outputView(getOutputWrapper(), targetRect);
				mBlitter.blitColor(outputView, color, useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE);
			}
		}

		void drawIndexed(Vec2i position, PaletteBitmap& inputBitmap, const PaletteBase& palette, Color color)
		{
			const Blitter::OutputWrapper outputWrapper(getOutputWrapper(), getScissorRect());
			const Blitter::IndexedSpriteWrapper inputWrapper(inputBitmap.getData(), inputBitmap.getSize(), Vec2i());
			const Blitter::PaletteWrapper paletteWrapper(palette.getRawColors(), palette.getSize());

			Blitter::Options blitterOptions;
			blitterOptions.mBlendMode = useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE;
			blitterOptions.mTintColor = (color != Color::WHITE) ? &color : nullptr;
			blitterOptions.mSwapRedBlueChannels = needSwapRedBlueChannels();

			mBlitter.blitIndexed(outputWrapper, inputWrapper, paletteWrapper, position, blitterOptions);
		}

		void printText(Font& font, const StringReader& text, const Recti& rect, const DrawerPrintOptions& printOptions)
		{
			Bitmap& bufferBitmap = mTempBuffer;

			Vec2i drawPosition;
			font.printBitmap(bufferBitmap, drawPosition, rect, text, printOptions.mAlignment, printOptions.mSpacing, &mTempReservedSize);

			Blitter::Options blitterOptions;
			blitterOptions.mBlendMode = BlendMode::ALPHA;
			blitterOptions.mTintColor = (printOptions.mTintColor != Color::WHITE) ? &printOptions.mTintColor : nullptr;
			blitterOptions.mSwapRedBlueChannels = needSwapRedBlueChannels();
			mBlitter.blitSprite(Blitter::OutputWrapper(getOutputWrapper(), getScissorRect()), Blitter::SpriteWrapper(bufferBitmap, Vec2i()), drawPosition, blitterOptions);
		}

	public:
		SDL_Window* mOutputWindow = nullptr;
		SDL_Surface* mScreenSurface = nullptr;
		bool mSurfaceSwapRedBlue = false;

		BlendMode mCurrentBlendMode = BlendMode::OPAQUE;
		SamplingMode mCurrentSamplingMode = SamplingMode::POINT;
		TextureWrapMode mCurrentWrapMode = TextureWrapMode::CLAMP;

		Recti mScissorRect;
		std::vector<Recti> mScissorStack;

		Blitter mBlitter;
		Bitmap mTempBuffer;
		int mTempReservedSize = 0;

	private:
		DrawerTexture* mCurrentRenderTarget = nullptr;
		BitmapViewMutable<uint32> mOutputWrapper;
		bool mIsScreenSurfaceLocked = false;
		bool mDisplayedFormatWarning = false;
	};
}


SoftwareDrawer::SoftwareDrawer() :
	mInternal(*new softwaredrawer::Internal())
{
}

SoftwareDrawer::~SoftwareDrawer()
{
	delete &mInternal;
}

void SoftwareDrawer::createTexture(DrawerTexture& outTexture)
{
	outTexture.setImplementation(mInternal.createTexture(outTexture));
}

void SoftwareDrawer::refreshTexture(DrawerTexture& texture)
{
	createTexture(texture);
}

void SoftwareDrawer::setupRenderWindow(SDL_Window* window)
{
	mInternal.setupScreenSurface(window);
}

void SoftwareDrawer::performRendering(const DrawCollection& drawCollection)
{
	for (DrawCommand* drawCommand : drawCollection.getDrawCommands())
	{
		switch (drawCommand->getType())
		{
			case DrawCommand::Type::UNDEFINED:
			{
				RMX_ERROR("Got invalid draw command", );
				continue;
			}

			case DrawCommand::Type::SET_WINDOW_RENDER_TARGET:
			{
				//SetWindowRenderTargetDrawCommand& dc = drawCommand->as<SetWindowRenderTargetDrawCommand>();
				if (nullptr != mInternal.getCurrentRenderTarget())
				{
					mInternal.getCurrentRenderTarget()->bitmapUpdated();
					mInternal.setCurrentRenderTarget(nullptr);
				}
				break;
			}

			case DrawCommand::Type::SET_RENDER_TARGET:
			{
				SetRenderTargetDrawCommand& dc = drawCommand->as<SetRenderTargetDrawCommand>();
				if (nullptr != mInternal.getCurrentRenderTarget())
				{
					mInternal.getCurrentRenderTarget()->bitmapUpdated();
				}
				mInternal.setCurrentRenderTarget(dc.mTexture);
				break;
			}

			case DrawCommand::Type::RECT:
			{
				RectDrawCommand& dc = drawCommand->as<RectDrawCommand>();
				Bitmap* inputBitmap = (nullptr == dc.mTexture) ? nullptr : &dc.mTexture->accessBitmap();

				mInternal.drawRect(dc.mRect, inputBitmap, dc.mColor, dc.mUV0, dc.mUV1);
				break;
			}

			case DrawCommand::Type::UPSCALED_RECT:
			{
				UpscaledRectDrawCommand& dc = drawCommand->as<UpscaledRectDrawCommand>();
				if (nullptr != dc.mTexture)
				{
					BitmapViewMutable<uint32>& outputWrapper = mInternal.getOutputWrapper();
					BitmapViewMutable<uint32> inputWrapper(dc.mTexture->accessBitmap());

					if (mInternal.needSwapRedBlueChannels())
					{
						mInternal.setupRedBlueSwappedBitmapWrapper(inputWrapper);
					}

					mInternal.mBlitter.blitRectWithScaling(outputWrapper, dc.mRect, inputWrapper, Recti(0, 0, inputWrapper.getSize().x, inputWrapper.getSize().y), Blitter::Options());
				}
				break;
			}

			case DrawCommand::Type::SPRITE:
			{
				SpriteDrawCommand& sc = drawCommand->as<SpriteDrawCommand>();
				const SpriteCollection::Item* item = SpriteCollection::instance().getSprite(sc.mSpriteKey);
				if (nullptr == item)
					break;

				const PaletteBase* palette = nullptr;
				if (!item->mUsesComponentSprite)
				{
					palette = PaletteCollection::instance().getPalette(sc.mPaletteKey, 0);
					if (nullptr == palette)
						break;
				}

				SpriteBase& sprite = *item->mSprite;
				Vec2i offset = sprite.mOffset;

				// TODO: No support for bilinear sampling here...

				if (item->mUsesComponentSprite)
				{
					Vec2i size = sprite.getSize();
					if (sc.mScale.x != 1.0f || sc.mScale.y != 1.0f)
					{
						offset.x = roundToInt((float)offset.x * sc.mScale.x);
						offset.y = roundToInt((float)offset.y * sc.mScale.y);
						size.x = roundToInt((float)size.x * sc.mScale.x);
						size.y = roundToInt((float)size.y * sc.mScale.y);
					}
					const Recti targetRect(sc.mPosition + offset, size);

					ComponentSprite& componentSprite = static_cast<ComponentSprite&>(sprite);
					mInternal.drawRect(targetRect, &componentSprite.accessBitmap(), sc.mTintColor);
				}
				else
				{
					// TODO: Support scaling here as well

					PaletteSprite& paletteSprite = static_cast<PaletteSprite&>(sprite);
					mInternal.drawIndexed(sc.mPosition + offset, paletteSprite.accessBitmap(), *palette, sc.mTintColor);
				}
				break;
			}

			case DrawCommand::Type::SPRITE_RECT:
			{
				SpriteRectDrawCommand& sc = drawCommand->as<SpriteRectDrawCommand>();
				const SpriteCollection::Item* item = SpriteCollection::instance().getSprite(sc.mSpriteKey);
				if (nullptr == item)
					break;
				if (!item->mUsesComponentSprite)
					break;

				ComponentSprite& sprite = *static_cast<ComponentSprite*>(item->mSprite);

				// TODO: No support for bilinear sampling here...

				mInternal.drawRect(sc.mRect, &sprite.accessBitmap(), sc.mTintColor);
				break;
			}

			case DrawCommand::Type::MESH:
			{
				MeshDrawCommand& dc = drawCommand->as<MeshDrawCommand>();
				if (nullptr != dc.mTexture)
				{
					BitmapViewMutable<uint32> outputView(mInternal.getOutputWrapper().getData(), mInternal.getOutputWrapper().getSize());
					Bitmap& inputBitmap = dc.mTexture->accessBitmap();

					Blitter::Options options;
					options.mBlendMode = mInternal.useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE;
					options.mSamplingMode = (mInternal.mCurrentSamplingMode == SamplingMode::BILINEAR) ? SamplingMode::BILINEAR : SamplingMode::POINT;
					// Note that this does not support red-blue channel swap

					SoftwareRasterizer rasterizer(outputView, options);
					SoftwareRasterizer::Vertex triangle[3];

					const int numTriangles = (int)dc.mTriangles.size() / 3;
					for (int i = 0; i < numTriangles; ++i)
					{
						DrawerMeshVertex* input = &dc.mTriangles[i * 3];
						for (int k = 0; k < 3; ++k)
						{
							triangle[k].mPosition = input[k].mPosition;
							triangle[k].mUV = input[k].mTexcoords;
						}
						rasterizer.drawTriangle(triangle, inputBitmap, false);
					}
				}
				break;
			}

			case DrawCommand::Type::MESH_VERTEX_COLOR:
			{
				MeshVertexColorDrawCommand& dc = drawCommand->as<MeshVertexColorDrawCommand>();
				BitmapViewMutable<uint32> outputView(mInternal.getOutputWrapper().getData(), mInternal.getOutputWrapper().getSize());

				Blitter::Options options;
				options.mBlendMode = mInternal.useAlphaBlending() ? BlendMode::ALPHA : BlendMode::OPAQUE;

				SoftwareRasterizer rasterizer(outputView, options);
				SoftwareRasterizer::Vertex triangle[3];
				const bool swapRedBlue = mInternal.needSwapRedBlueChannels();

				const int numTriangles = (int)dc.mTriangles.size() / 3;
				for (int i = 0; i < numTriangles; ++i)
				{
					DrawerMeshVertex_P2_C4* input = &dc.mTriangles[i * 3];
					for (int k = 0; k < 3; ++k)
					{
						triangle[k].mPosition = input[k].mPosition;
						triangle[k].mColor = input[k].mColor;
					}
					if (swapRedBlue)
					{
						for (int k = 0; k < 3; ++k)
							triangle[k].mColor.swapRedBlue();
					}
					rasterizer.drawTriangle(triangle);
				}
				break;
			}

			case DrawCommand::Type::SET_BLEND_MODE:
			{
				SetBlendModeDrawCommand& dc = drawCommand->as<SetBlendModeDrawCommand>();
				mInternal.mCurrentBlendMode = dc.mBlendMode;
				break;
			}

			case DrawCommand::Type::SET_SAMPLING_MODE:
			{
				SetSamplingModeDrawCommand& dc = drawCommand->as<SetSamplingModeDrawCommand>();
				mInternal.mCurrentSamplingMode = dc.mSamplingMode;
				break;
			}

			case DrawCommand::Type::SET_WRAP_MODE:
			{
				SetWrapModeDrawCommand& dc = drawCommand->as<SetWrapModeDrawCommand>();
				mInternal.mCurrentWrapMode = dc.mWrapMode;
				break;
			}

			case DrawCommand::Type::PRINT_TEXT:
			{
				PrintTextDrawCommand& dc = drawCommand->as<PrintTextDrawCommand>();
				mInternal.printText(*dc.mFont, dc.mText, dc.mRect, dc.mPrintOptions);
				break;
			}

			case DrawCommand::Type::PRINT_TEXT_W:
			{
				PrintTextWDrawCommand& dc = drawCommand->as<PrintTextWDrawCommand>();
				mInternal.printText(*dc.mFont, dc.mText, dc.mRect, dc.mPrintOptions);
				break;
			}

			case DrawCommand::Type::PUSH_SCISSOR:
			{
				PushScissorDrawCommand& dc = drawCommand->as<PushScissorDrawCommand>();

				mInternal.mScissorRect.intersect(dc.mRect);
				mInternal.mScissorStack.emplace_back(mInternal.mScissorRect);
				break;
			}

			case DrawCommand::Type::POP_SCISSOR:
			{
				mInternal.mScissorStack.pop_back();
				if (mInternal.mScissorStack.empty())
				{
					if (nullptr != mInternal.mScreenSurface)
					{
						mInternal.mScissorRect.set(0, 0, mInternal.mScreenSurface->w, mInternal.mScreenSurface->h);
					}
				}
				else
				{
					mInternal.mScissorRect = mInternal.mScissorStack.back();
				}
				break;
			}
		}
	}
}

void SoftwareDrawer::presentScreen()
{
	if (nullptr == mInternal.mScreenSurface)
		return;

	mInternal.unlockScreenSurface();
	SDL_UpdateWindowSurface(mInternal.mOutputWindow);
}

const BitmapViewMutable<uint32>& SoftwareDrawer::getRenderTarget() const
{
	return mInternal.getOutputWrapper();
}

bool SoftwareDrawer::needSwapRedBlueChannels() const
{
	return mInternal.needSwapRedBlueChannels();
}
