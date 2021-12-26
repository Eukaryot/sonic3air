/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
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

	void mirrorBitmapX(Bitmap& destBitmap, const Bitmap& sourceBitmap)
	{
		destBitmap.create(sourceBitmap.getWidth(), sourceBitmap.getHeight());
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
				mOutputWrapper.Reset();
			}
		}

		SoftwareDrawerTexture* createTexture()
		{
			SoftwareDrawerTexture* texture = new SoftwareDrawerTexture();
			return texture;
		}

		void setCurrentRenderTarget(DrawerTexture* renderTarget)
		{
			if (nullptr == mCurrentRenderTarget)
			{
				unlockScreenSurface();
			}

			mCurrentRenderTarget = renderTarget;

			if (nullptr == mCurrentRenderTarget)
			{
				// Next call to "getOutputWrapper" will update everything accordingly
			}
			else
			{
				mOutputWrapper.Set(mCurrentRenderTarget->accessBitmap());
			}
		}

		DrawerTexture* getCurrentRenderTarget() const
		{
			return mCurrentRenderTarget;
		}

		BitmapWrapper& getOutputWrapper()
		{
			if (nullptr == mCurrentRenderTarget)
			{
				if (nullptr != mScreenSurface)
				{
					if (!mIsScreenSurfaceLocked)
					{
						SDL_LockSurface(mScreenSurface);
						mIsScreenSurfaceLocked = true;
						mOutputWrapper.Set((uint32*)mScreenSurface->pixels, Vec2i(mScreenSurface->w, mScreenSurface->h));
					}
				}
				else
				{
					// Handle invalid screen surface
					mOutputWrapper.Reset();
				}
			}
			return mOutputWrapper;
		}

		bool needSwapRedBlueChannels()
		{
			return (nullptr == mCurrentRenderTarget && mSurfaceSwapRedBlue);
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
			return (mCurrentBlendMode == DrawerBlendMode::ALPHA);
		}

		void printText(Font& font, const StringReader& text, const Recti& rect, const rmx::Painter::PrintOptions& printOptions)
		{
			BitmapWrapper& outputWrapper = getOutputWrapper();
			Bitmap& bufferBitmap = mTempBuffer;

			Vec2i drawPosition;
			font.printBitmap(bufferBitmap, drawPosition, rect, text, printOptions.mAlignment, printOptions.mSpacing);
			if (printOptions.mTintColor != Color::WHITE)
			{
				softwaredrawer::applyTintToBitmap(bufferBitmap, printOptions.mTintColor);
			}

			// Consider cropping
			const Recti uncroppedRect(drawPosition.x, drawPosition.y, bufferBitmap.mWidth, bufferBitmap.mHeight);
			Recti croppedRect = uncroppedRect;
			croppedRect.intersect(uncroppedRect, mScissorRect);
			if (croppedRect.width > 0 && croppedRect.height > 0)
			{
				Recti inputRect(0, 0, bufferBitmap.mWidth, bufferBitmap.mHeight);
				inputRect.x += (croppedRect.x - uncroppedRect.x);
				inputRect.y += (croppedRect.y - uncroppedRect.y);
				inputRect.width  -= (uncroppedRect.width - croppedRect.width);
				inputRect.height -= (uncroppedRect.height - croppedRect.height);

				Blitter::Options options;
				options.mSwapRedBlue = needSwapRedBlueChannels();
				options.mUseAlphaBlending = true;

				BitmapWrapper bufferWrapper(bufferBitmap);
				Blitter::blitBitmap(outputWrapper, drawPosition + inputRect.getPos(), bufferWrapper, inputRect, options);
			}
		}

	public:
		SDL_Window* mOutputWindow = nullptr;
		SDL_Surface* mScreenSurface = nullptr;
		bool mSurfaceSwapRedBlue = false;

		DrawerBlendMode mCurrentBlendMode = DrawerBlendMode::NONE;
		DrawerSamplingMode mCurrentSamplingMode = DrawerSamplingMode::POINT;
		DrawerWrapMode mCurrentWrapMode = DrawerWrapMode::CLAMP;

		Recti mScissorRect;
		std::vector<Recti> mScissorStack;

		Bitmap mTempBuffer;

	private:
		DrawerTexture* mCurrentRenderTarget = nullptr;
		BitmapWrapper mOutputWrapper;
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
	outTexture.setImplementation(mInternal.createTexture());
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
				BitmapWrapper& outputWrapper = mInternal.getOutputWrapper();

				bool mirrorX = false;
				Recti rect = dc.mRect;
				if (rect.width < 0)
				{
					// Mirror horizontally
					mirrorX = true;
					rect.x += rect.width;
					rect.width = -rect.width;
				}
				const Recti uncroppedRect = rect;
				rect.intersect(rect, mInternal.getScissorRect());

				if (!rect.empty())
				{
					Blitter::Options options;
					options.mSwapRedBlue = mInternal.needSwapRedBlueChannels();
					options.mUseAlphaBlending = mInternal.useAlphaBlending();

					if (nullptr != dc.mTexture)
					{
						Bitmap* inputBitmap = &dc.mTexture->accessBitmap();

						// Consider mirroring
						if (mirrorX)
						{
							softwaredrawer::mirrorBitmapX(mInternal.mTempBuffer, *inputBitmap);
							inputBitmap = &mInternal.mTempBuffer;
						}

						BitmapWrapper inputWrapper(*inputBitmap);

						// Scaling?
						const bool useScaling = (rect.getSize() != inputBitmap->getSize());

						// Consider cropping
						Recti inputRect(rect.getPos() - uncroppedRect.getPos(), rect.getSize());
						if (useScaling)
						{
							const float scaleX = (float)inputBitmap->getWidth() / (float)uncroppedRect.width;
							const float scaleY = (float)inputBitmap->getHeight() / (float)uncroppedRect.height;
							inputRect.x = roundToInt((float)inputRect.x * scaleX);
							inputRect.y = roundToInt((float)inputRect.y * scaleY);
							inputRect.width = roundToInt((float)inputRect.width * scaleX);
							inputRect.height = roundToInt((float)inputRect.height * scaleY);
						}

						// TODO: Support tint color everywhere
						options.mTintColor = dc.mColor;

						if (useScaling)
						{
							Blitter::blitBitmapWithScaling(outputWrapper, rect, inputWrapper, inputRect, options);
						}
						else
						{
							Blitter::blitBitmap(outputWrapper, rect.getPos(), inputWrapper, inputRect, options);
						}
					}
					else
					{
						Blitter::blitColor(outputWrapper, rect, dc.mColor, options);
					}
				}
				break;
			}

			case DrawCommand::Type::UPSCALED_RECT:
			{
				UpscaledRectDrawCommand& dc = drawCommand->as<UpscaledRectDrawCommand>();
				if (nullptr != dc.mTexture)
				{
					BitmapWrapper& outputWrapper = mInternal.getOutputWrapper();
					BitmapWrapper inputWrapper(dc.mTexture->accessBitmap());

					Blitter::Options options;
					options.mSwapRedBlue = mInternal.needSwapRedBlueChannels();

					Blitter::blitBitmapWithScaling(outputWrapper, dc.mRect, inputWrapper, Recti(0, 0, inputWrapper.mSize.x, inputWrapper.mSize.y), options);
				}
				break;
			}

			case DrawCommand::Type::MESH:
			{
				MeshDrawCommand& dc = drawCommand->as<MeshDrawCommand>();
				if (nullptr != dc.mTexture)
				{
					BitmapWrapper& outputWrapper = mInternal.getOutputWrapper();
					Bitmap& inputBitmap = dc.mTexture->accessBitmap();

					Blitter::Options options;
					options.mSwapRedBlue = mInternal.needSwapRedBlueChannels();
					options.mUseAlphaBlending = mInternal.useAlphaBlending();
					options.mUseBilinearSampling = (mInternal.mCurrentSamplingMode == DrawerSamplingMode::BILINEAR);

					SoftwareRasterizer rasterizer(outputWrapper, options);
					SoftwareRasterizer::Vertex_P2_T2 triangle[3];

					const int numTriangles = (int)dc.mTriangles.size() / 3;
					for (int i = 0; i < numTriangles; ++i)
					{
						DrawerMeshVertex* input = &dc.mTriangles[i * 3];
						for (int k = 0; k < 3; ++k)
						{
							triangle[k].mPosition = input[k].mPosition;
							triangle[k].mUV = input[k].mTexcoords;
						}
						rasterizer.drawTriangle(triangle, inputBitmap);
					}
				}
				break;
			}

			case DrawCommand::Type::MESH_VERTEX_COLOR:
			{
				MeshVertexColorDrawCommand& dc = drawCommand->as<MeshVertexColorDrawCommand>();
				BitmapWrapper& outputWrapper = mInternal.getOutputWrapper();

				Blitter::Options options;
				options.mSwapRedBlue = mInternal.needSwapRedBlueChannels();
				options.mUseAlphaBlending = mInternal.useAlphaBlending();

				SoftwareRasterizer rasterizer(outputWrapper, options);
				SoftwareRasterizer::Vertex_P2_C4 triangle[3];

				const int numTriangles = (int)dc.mTriangles.size() / 3;
				for (int i = 0; i < numTriangles; ++i)
				{
					DrawerMeshVertex_P2_C4* input = &dc.mTriangles[i * 3];
					for (int k = 0; k < 3; ++k)
					{
						triangle[k].mPosition = input[k].mPosition;
						triangle[k].mColor = input[k].mColor;
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
