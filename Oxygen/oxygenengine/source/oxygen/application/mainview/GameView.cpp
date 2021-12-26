/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/HighResolutionTimer.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/helper/Profiling.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/rendering/RenderResources.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"


namespace
{
	bool translatePositionIntoRect(Vec2f& outPosition, const Rectf& rect, const Vec2f& inPosition)
	{
		outPosition.x = (inPosition.x - rect.x) / rect.width;
		outPosition.y = (inPosition.y - rect.y) / rect.height;
		return (outPosition.x >= 0.0f && outPosition.x < 1.0f && outPosition.y >= 0.0f && outPosition.y < 1.0f);
	}

	bool dumpPaletteAsBMP(int paletteIndex)
	{
		PaletteManager& paletteManager = VideoOut::instance().getRenderParts().getPaletteManager();
		std::vector<uint8> fileContent;
		Color palette[256] = { Color::TRANSPARENT };
		PaletteBitmap bmp;
		bmp.create(16, 16);
		for (int colorIndex = 0; colorIndex < 256; ++colorIndex)
		{
			palette[colorIndex] = paletteManager.getPaletteEntry(paletteIndex, colorIndex);
			bmp.getData()[colorIndex] = colorIndex;
		}
		if (!bmp.saveBMP(fileContent, palette))
			return false;
		return FTX::FileSystem->saveFile("palette_dump.bmp", fileContent);
	}

	// Function is not used any more
#if 0
	bool dumpPaletteAsPAL(int paletteIndex)
	{
		PaletteManager& paletteManager = VideoOut::instance().getRenderParts().getPaletteManager();
		String str;
		str << "JASC-PAL\r\n";
		str << "0100\r\n";
		str << "256\r\n";
		for (int colorIndex = 0; colorIndex < 64; ++colorIndex)
		{
			const Color color = paletteManager.getPaletteEntry(paletteIndex, colorIndex);
			str << roundToInt(color.r * 255.0f) << ' ' << roundToInt(color.g * 255.0f) << ' ' << roundToInt(color.b * 255.0f) << "\r\n";
		}
		for (int colorIndex = 64; colorIndex < 256; ++colorIndex)
		{
			str << "0 0 0\r\n";
		}
		return str.saveFile("palette_dump.pal");
	}
#endif
}



GameView::GameView(Simulation& simulation) :
	mSimulation(simulation)
{
}

GameView::~GameView()
{
}

void GameView::updateGameViewport()
{
	const Recti gameScreenRect = VideoOut::instance().getScreenRect();

	switch (Configuration::instance().mUpscaling)
	{
		default:
		case 0:
		{
			// Aspect fit
			mGameViewport = RenderUtils::getLetterBoxRect(mRect, gameScreenRect.getAspectRatio());
			break;
		}

		case 1:
		{
			// Integer upscaling
			mGameViewport = RenderUtils::getLetterBoxRect(mRect, gameScreenRect.getAspectRatio());
			const int scale = (int)mGameViewport.height / gameScreenRect.height;
			if (scale >= 1)
			{
				mGameViewport.width = (float)(gameScreenRect.width * scale);
				mGameViewport.height = (float)(gameScreenRect.height * scale);
				mGameViewport.x = mRect.x + (mRect.width - mGameViewport.width) / 2;
				mGameViewport.y = mRect.y + (mRect.height - mGameViewport.height) / 2;
			}
			break;
		}

		case 2:
		{
			// Halfway stretch to fill
			const Rectf letterBox = RenderUtils::getLetterBoxRect(mRect, gameScreenRect.getAspectRatio());
			mGameViewport.width = round((letterBox.width + mRect.width) * 0.5f);		// Average size of letter box and full stretch
			mGameViewport.height = round((letterBox.height + mRect.height) * 0.5f);
			mGameViewport.setPos((mRect.getSize() - mGameViewport.getSize()) * 0.5f);	// Center on screen
			break;
		}

		case 3:
		{
			// Stretch to fill
			mGameViewport = mRect;
			break;
		}

		case 4:
		{
			// Scale to fill
			mGameViewport = RenderUtils::getScaleToFillRect(mRect, gameScreenRect.getAspectRatio());
			break;
		}
	}
}

bool GameView::translatePositionIntoGameViewport(Vec2f& outPosition, const Vec2f& inPosition) const
{
	if (translatePositionIntoRect(outPosition, mGameViewport, inPosition))
	{
		outPosition.x *= (float)VideoOut::instance().getScreenWidth();
		outPosition.y *= (float)VideoOut::instance().getScreenHeight();
		return true;
	}
	return false;
}

void GameView::translateRectIntoGameViewport(Rectf& outRect, const Rectf& inRect) const
{
	const float scaleX = (float)VideoOut::instance().getScreenWidth() / mGameViewport.width;
	const float scaleY = (float)VideoOut::instance().getScreenHeight() / mGameViewport.height;
	outRect.x = (outRect.x - mGameViewport.x) * scaleX;
	outRect.y = (outRect.y - mGameViewport.y) * scaleY;
	outRect.width = outRect.width * scaleX;
	outRect.height = outRect.height * scaleY;
}

void GameView::translatePositionIntoScreenCoords(Vec2f& outPosition, const Vec2f& inPosition) const
{
	const float scaleX = mGameViewport.width / (float)VideoOut::instance().getScreenWidth();
	const float scaleY = mGameViewport.height / (float)VideoOut::instance().getScreenHeight();
	outPosition.x = mGameViewport.x + inPosition.x * scaleX;
	outPosition.y = mGameViewport.y + inPosition.y * scaleY;
}

void GameView::translateRectIntoScreenCoords(Rectf& outRect, const Rectf& inRect) const
{
	const float scaleX = mGameViewport.width / (float)VideoOut::instance().getScreenWidth();
	const float scaleY = mGameViewport.height / (float)VideoOut::instance().getScreenHeight();
	outRect.x = mGameViewport.x + inRect.x * scaleX;
	outRect.y = mGameViewport.y + inRect.y * scaleY;
	outRect.width = inRect.width * scaleX;
	outRect.height = inRect.height * scaleY;
}

void GameView::initialize()
{
	GuiBase::initialize();

	const Vec2i& resolution = Configuration::instance().mGameScreen;

	RMX_LOG_INFO("Creating game screen texture");
	EngineMain::instance().getDrawer().createTexture(mFinalGameTexture);
	mFinalGameTexture.setupAsRenderTarget(resolution.x, resolution.y);
}

void GameView::deinitialize()
{
	GuiBase::deinitialize();

	// Remove all children, as they must not get deleted automatically (which would be the case if they stay added as children)
	while (!mChildren.empty())
	{
		removeChild(*mChildren.begin());
	}
}

void GameView::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);
	if (ev.state)
	{
		const bool altPressed = (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT));

		// Hotkeys that are always available
		if (altPressed)
		{
			// No key repeat for these
			if (!ev.repeat)
			{
				switch (ev.key)
				{
					case 'b':
					{
						int& effect = Configuration::instance().mBackgroundBlur;
						effect = (effect + 1) % 5;
						setLogDisplay("Background Blur: " + std::to_string(effect * 25) + "%");
						break;
					}

					case 'f':
					case 'g':
					{
						int& effect = Configuration::instance().mFiltering;
						if (ev.key == 'f')
							effect = (effect + 6) % 7;
						else
							effect = (effect + 1) % 7;

						static const std::string FILTER_METHOD_NAME[] = { "Sharp", "Soft 1", "Soft 2", "xBRZ", "HQ2x", "HQ3x", "HQ4x" };
						setLogDisplay("Filtering method: " + FILTER_METHOD_NAME[effect]);
						break;
					}

					case 'h':
					{
						Configuration::FrameSyncType& frameSync = Configuration::instance().mFrameSync;
						frameSync = Configuration::FrameSyncType(((int)frameSync + 1) % (int)Configuration::FrameSyncType::_NUM);
						EngineMain::instance().setVSyncMode(frameSync);

						static const std::string FRAME_SYNC_NAME[(int)Configuration::FrameSyncType::_NUM] = { "V-Sync Off", "VSync On", "V-Sync + FPS Cap", "Frame Interpolation" };
						setLogDisplay("Frame Sync: " + FRAME_SYNC_NAME[(int)frameSync]);
						break;
					}
				}
			}
		}

		// Hotkeys requiring dev mode
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			// No key repeat for these
			if (!ev.repeat)
			{
				if (altPressed)
				{
					switch (ev.key)
					{
						case 'm':
						{
							mDebugPaletteDisplay = (mDebugPaletteDisplay + 2) % 3 - 1;
							break;
						}
					}
				}
				else
				{
					switch (ev.key)
					{
						case SDLK_KP_1:
							setGameSpeed(1.0f);
							break;

						case SDLK_KP_2:
							setGameSpeed(FTX::keyState(SDLK_LCTRL) ? 2.0f : 3.0f);
							break;

						case SDLK_KP_3:
							setGameSpeed(5.0f);
							break;

						case SDLK_KP_4:
							setGameSpeed(10.0f);
							break;

						case SDLK_KP_7:
							setGameSpeed(1000.0f);
							break;

						case SDLK_F10:
						{
							RenderResources::instance().loadSpriteCache();
							ResourcesCache::instance().loadAllResources();
							setLogDisplay("Reloaded resources");
							break;
						}
					}
				}
			}
		}

		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			// Hotkeys that are only available when developer features are active
			if (altPressed)
			{
				// No key repeat for these
				if (!ev.repeat)
				{
					switch (ev.key)
					{
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						{
							VideoOut::instance().toggleLayerRendering(ev.key - '1');
							setLogDisplay("Rendering layers: " + VideoOut::instance().getLayerRenderingDebugString());
							break;
						}

						case 'v':
						{
							mDebugVisualizationsEnabled = !mDebugVisualizationsEnabled;
							break;
						}
						case 'c':
						{
							++mDebugVisualizationsMode;
							break;
						}

						case 't':
						{
							RenderParts& renderer = RenderParts::instance();
							renderer.setFullEmulation(!renderer.getFullEmulation());

							if (renderer.getFullEmulation())
								setLogDisplay("Set renderer mode: Full emulation");
							else
								setLogDisplay("Set renderer mode: Abstraction (allows for Alt-click / Alt-rightclick)");
							break;
						}
					}
				}
			}
			else
			{
				// No key repeat for these
				if (!ev.repeat)
				{
					switch (ev.key)
					{
						case SDLK_KP_0:
							setGameSpeed(0.0f);
							break;

						case SDLK_KP_5:
						case SDLK_CLEAR:
							setGameSpeed(FTX::keyState(SDLK_LCTRL) ? 0.5f : 0.2f);
							break;

						case SDLK_KP_6:
							setGameSpeed(FTX::keyState(SDLK_LCTRL) ? 0.01f : 0.05f);
							break;

						case SDLK_KP_PERIOD:
							// Supporting both Shift and Ctrl, as the Shift + Peroid combination does not seem to work for all keyboards
							mSimulation.setNextSingleStep(true, FTX::keyState(SDLK_LSHIFT) || FTX::keyState(SDLK_LCTRL));
							break;

						case SDLK_F7:
						{
							mSimulation.reloadLastState();
							setLogDisplay("Reset emulator interface");
							break;
						}

						case SDLK_F11:
						{
							HighResolutionTimer timer;
							timer.start();
							if (mSimulation.reloadScripts(true))
							{
								setLogDisplay(String(0, "Reloaded scripts in %0.2f sec", timer.getSecondsSinceStart()));
							}
							break;
						}

						case SDLK_TAB:
						{
							if (mDebugOutput >= 0)
							{
								VideoOut::instance().dumpDebugDraw(mDebugOutput);
								setLogDisplay("Dumped debug output to BMP file");
							}
							else if (mDebugPaletteDisplay >= 0)
							{
								dumpPaletteAsBMP(mDebugPaletteDisplay);
								setLogDisplay("Dumped palette to 'palette_dump.bmp'");
							}
							break;
						}
					}
				}

				// Key repeat is fine for these
				switch (ev.key)
				{
					case SDLK_KP_PERIOD:
					{
						// Supporting both Shift and Ctrl, as the SHift + Peroid combination does not seem to work for all keyboards
						mSimulation.setNextSingleStep(true, FTX::keyState(SDLK_LSHIFT) || FTX::keyState(SDLK_LCTRL));
						break;
					}
				}
			}
		}
	}
}

void GameView::mouse(const rmx::MouseEvent& ev)
{
	// TODO: This functionality (as well as abstraction plane A rendering) is entirely S3AIR specific and should be moved out of the core engine
	if (EngineMain::getDelegate().useDeveloperFeatures() && !RenderParts::instance().getFullEmulation())
	{
		if (ev.state && FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
		{
			// Translate mouse position into game screen position
			Vec2f relativePosition;
			if (translatePositionIntoGameViewport(relativePosition, ev.position))
			{
				EmulatorInterface& emulatorInterface = mSimulation.getCodeExec().getEmulatorInterface();

				const uint32 cameraX = emulatorInterface.readMemory16(0xffffee78);
				const uint32 cameraY = emulatorInterface.readMemory16(0xffffee7c);

				const uint32 globalColumn = cameraX + roundToInt(relativePosition.x);
				const uint32 globalRow = cameraY + roundToInt(relativePosition.y);

				const uint32 chunkColumn = globalColumn / 128;
				const uint32 chunkRow = (globalRow / 128) & 0x1f;		// Looks like there are only 32 chunks in y-direction allowed in a level; that makes a maximum level height of 4096 pixels

				const uint32 chunkAddress = 0xffff0000 + emulatorInterface.readMemory16(0xffff8008 + chunkRow * 4) + chunkColumn;
				uint8 chunkType = emulatorInterface.readMemory8(chunkAddress);

				int8 change = (ev.button == rmx::MouseButton::Left) ? 1 : (ev.button == rmx::MouseButton::Right) ? -1 : 0;
				if (FTX::keyState(SDLK_LSHIFT))
					change *= 0x10;
				chunkType += change;
				emulatorInterface.writeMemory8(chunkAddress, chunkType);

				LogDisplay::instance().updateScriptLogValue("chunk", rmx::hexString(chunkAddress, 8) + " -> " + rmx::hexString(chunkType, 2));
			}
		}
	}
}

void GameView::update(float timeElapsed)
{
	if (mFadeChange != 0.0f)
	{
		mFadeValue = saturate(mFadeValue + timeElapsed * mFadeChange);
	}

	if (mBlurringStillImage && mBlurringTimeout > 0.0f)
	{
		mBlurringTimeout -= timeElapsed;
		mBlurringStepTimer += timeElapsed;
	}

	// Debug output
	{
		mDebugOutput = -1;
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			if (FTX::keyState(','))
			{
				// Debug output for plane B
				mDebugOutput = 0;
			}
			else if (FTX::keyState('.'))
			{
				// Debug output for plane A or W
				mDebugOutput = (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT)) ? 2 : 1;
			}
			else if (FTX::keyState('-'))
			{
				// Debug output for patterns
				mDebugOutput = 3;
			}

			LogDisplay::instance().clearScriptLogValue("~index");
			LogDisplay::instance().clearScriptLogValue("~addr");
			LogDisplay::instance().clearScriptLogValue("~ptrn");

			if (mDebugOutput >= 0)
			{
				// Get the mouse position inside the debug output
				Rectf rect;
				if (mDebugOutput <= PlaneManager::PLANE_A)
				{
					const Vec2i playfieldSize = VideoOut::instance().getRenderParts().getPlaneManager().getPlayfieldSizeInPixels();
					rect = RenderUtils::getLetterBoxRect(mRect, (float)playfieldSize.x / (float)playfieldSize.y);
				}
				else
				{
					rect = RenderUtils::getLetterBoxRect(mRect, 2.0f);
				}

				Vec2f relativePosition;
				if (translatePositionIntoRect(relativePosition, rect, FTX::mousePos()))
				{
					const uint32 index = (int)(relativePosition.x * 64.0f) + (int)(relativePosition.y * 32.0f) * 64;
					LogDisplay::instance().updateScriptLogValue("~index", rmx::hexString(index, 4));
					if (mDebugOutput < 2)
					{
						const uint16 address = VideoOut::instance().getRenderParts().getPlaneManager().getPatternVRAMAddress(mDebugOutput, index);
						const uint16 pattern = VideoOut::instance().getRenderParts().getPlaneManager().getPatternAtIndex(mDebugOutput, index);
						LogDisplay::instance().updateScriptLogValue("~addr", rmx::hexString(address, 4));
						LogDisplay::instance().updateScriptLogValue("~ptrn", rmx::hexString(pattern, 4));
					}
					else
					{
						LogDisplay::instance().updateScriptLogValue("~addr", rmx::hexString((uint32)(index * 0x20), 4));
					}
				}
			}
		}
	}

	GuiBase::update(timeElapsed);
}

void GameView::render()
{
	mRect = FTX::screenRect();

	Drawer& drawer = EngineMain::instance().getDrawer();
	VideoOut& videoOut = VideoOut::instance();
	const Recti gameScreenRect = VideoOut::instance().getScreenRect();
	mFinalGameTexture.setupAsRenderTarget(gameScreenRect.width, gameScreenRect.height);

	// Refresh simulation output image
	if (mBlurringStillImage)
	{
		const constexpr float REDUCTION = 0.0333f;	// Can't remember any more why this is 1/30...
		if (mBlurringStepTimer >= REDUCTION)
		{
			mBlurringStepTimer -= REDUCTION;
			videoOut.blurGameScreen();
		}
	}
	else
	{
		// Render a new game screen image if needed
		//  -> This does some checks internally to determine if a new image is actually needed
		//  -> E.g. only if a simulation frame was completed since last call or if frame interpolation is active
		videoOut.updateGameScreen();
	}

	updateGameViewport();

	for (GuiBase* child : getChildren())
	{
		child->setRect(gameScreenRect);
	}

	if (mDebugOutput >= 0)
	{
		// Draw a dark background over the full screen
		drawer.setBlendMode(DrawerBlendMode::NONE);
		drawer.drawRect(FTX::screenRect(), Color(0.15f, 0.15f, 0.15f));
		drawer.performRendering();

		videoOut.renderDebugDraw(mDebugOutput, mRect);

		// Enable alpha again for the UI
		drawer.setBlendMode(DrawerBlendMode::ALPHA);
		drawer.performRendering();

		GuiBase::render();
		return;
	}

	// Here goes the real rendering
	drawer.setRenderTarget(mFinalGameTexture, gameScreenRect);
	drawer.setBlendMode(DrawerBlendMode::NONE);
#if 0
	// Test: Lazy version of a simple mirror mode
	//  -> TODO: ControlsIn needs to support mirror mode as well
	if (drawer.getType() != Drawer::Type::SOFTWARE)
	{
		const Recti drawRect(gameScreenRect.x + gameScreenRect.width, gameScreenRect.y, -gameScreenRect.width, gameScreenRect.height);
		drawer.drawRect(drawRect, videoOut.getGameScreenTexture());
	}
	else
#endif
	{
		drawer.drawRect(gameScreenRect, videoOut.getGameScreenTexture());
	}

	// Enable alpha for the UI
	drawer.setBlendMode(DrawerBlendMode::ALPHA);

	// Debug visualizations
	if (mDebugVisualizationsEnabled)
	{
		if (!mDebugVisualizationsOverlay.isValid())
		{
			drawer.createTexture(mDebugVisualizationsOverlay);
		}
		Bitmap& bitmap = mDebugVisualizationsOverlay.accessBitmap();
		bitmap.resize(gameScreenRect.width, gameScreenRect.height);
		bitmap.clear(0);
		EngineMain::getDelegate().fillDebugVisualization(bitmap, mDebugVisualizationsMode);
		mDebugVisualizationsOverlay.bitmapUpdated();

		drawer.drawRect(gameScreenRect, mDebugVisualizationsOverlay);
	}

	// Palette debug output
	if (mDebugPaletteDisplay >= 0)
	{
		const PaletteManager& paletteManager = videoOut.getRenderParts().getPaletteManager();
		for (int paletteIndex = 0; paletteIndex <= mDebugPaletteDisplay; ++paletteIndex)
		{
			const int baseX = 2 + paletteIndex * 84;
			const int baseY = 2;
			drawer.drawRect(Recti(baseX, baseY, 81, 59), Color::BLACK);

			int py = baseY + 1;
			for (int k = 0; k < 0x100; ++k)
			{
				const int px = baseX + 1 + (k % 0x10) * 5;
				const int height = (k < 0x40) ? 4 : 2;
				Color color = paletteManager.getPaletteEntry(paletteIndex, k);
				color.a = 1.0f;
				drawer.drawRect(Recti(px, py, 4, height), color);

				if ((k & 0x0f) == 0x0f)
				{
					py += height;
					py += (k == 0x3f) ? 3 : 1;
				}
			}
		}
		if (mDebugPaletteDisplay >= 1)
		{
			const int baseX = 2 + 2 * 84;
			const int baseY = 2;
			drawer.drawRect(Recti(baseX, baseY, 5, 50), Color::BLACK);

			for (int k = 0; k < 16; ++k)
			{
				const float fraction = (float)k / 15.0f;
				const Color color1 = Color(fraction, fraction, fraction);
				const Color color2 = Color(paletteManager.getGlobalComponentAddedColor().r + paletteManager.getGlobalComponentTintColor().r * fraction,
										   paletteManager.getGlobalComponentAddedColor().g + paletteManager.getGlobalComponentTintColor().g * fraction,
										   paletteManager.getGlobalComponentAddedColor().b + paletteManager.getGlobalComponentTintColor().b * fraction);
				drawer.drawRect(Recti(baseX + 1, baseY + 1 + k * 3, 3, 3), color2);
				drawer.drawRect(Recti(baseX + 3, baseY + 1 + k * 3, 1, 1), color1);
			}
		}

		if (drawer.getType() == Drawer::Type::OPENGL)
		{
			FTX::Painter->setColor(Color::WHITE);	// Prevent possible broken display in UI (e.g. in S3AIR's menus)
		}
	}
	drawer.performRendering();

	// Render the (pixelated) game UI
	mRect = gameScreenRect;
	GuiBase::render();

	// White overlay (used in Time Attack restart)
	if (mWhiteOverlayAlpha > 0.0f)
	{
		drawer.drawRect(FTX::screenRect(), Color(1.0f, 1.0f, 1.0f, mWhiteOverlayAlpha));
	}

	if (Configuration::instance().mPerformanceDisplay == 1)
	{
		// Show frame rate, using pixelated display
		const double averageTime = Profiling::getRootRegion().mAverageTime;
		if (averageTime > 0.0)
		{
			drawer.printText(EngineMain::getDelegate().getDebugFont(3), Recti(gameScreenRect.width - 3, 2, 0, 0), String(0, "%d FPS", roundToInt((float)(1.0 / averageTime))), 3);
		}
	}

	// Draw the combined image
	drawer.setWindowRenderTarget(FTX::screenRect());
	drawer.setBlendMode(DrawerBlendMode::NONE);
	drawer.drawUpscaledRect(mGameViewport, mFinalGameTexture);

	if (!FTX::Video->getVideoConfig().autoclearscreen)
	{
		// Draw black bars so no screen clearing is needed
		const float x1 = mGameViewport.x;
		const float x2 = mGameViewport.x + mGameViewport.width;
		const float x3 = (float)FTX::Video->getScreenWidth();
		const float y1 = mGameViewport.y;
		const float y2 = mGameViewport.y + mGameViewport.height;
		const float y3 = (float)FTX::Video->getScreenHeight();

		drawer.drawRect(Rectf(0, 0, x3, y1), Color::BLACK);
		drawer.drawRect(Rectf(0, y2, x3, y3 - y2), Color::BLACK);
		drawer.drawRect(Rectf(0, y1, x1, y2 - y1), Color::BLACK);
		drawer.drawRect(Rectf(x2, y1, x3 - x2, y2 - y1), Color::BLACK);
	}

	// Enable alpha again
	// TODO: Better do the fading inside the game viewport (instead of the full window) for performance reasons -- or apply to "drawUpscaledRect"
	drawer.setBlendMode(DrawerBlendMode::ALPHA);
	if (mFadeValue < 1.0f)
	{
		drawer.drawRect(FTX::screenRect(), Color(0.0f, 0.0f, 0.0f, 1.0f - mFadeValue));
	}

	drawer.performRendering();
}

void GameView::setFadedIn()
{
	mFadeValue = 1.0f;
	mFadeChange = 0.0f;
	setBlurringStillImage(false);
}

void GameView::startFadingIn(float fadeTime)
{
	mFadeValue = 0.0f;
	mFadeChange = 1.0f / fadeTime;
	setBlurringStillImage(false);
}

void GameView::startFadingOut(float fadeTime)
{
	mFadeChange = -1.0f / fadeTime;
	setBlurringStillImage(false);
}

void GameView::setBlurringStillImage(bool enable, float timeout)
{
	mBlurringStillImage = enable;
	mBlurringTimeout = enable ? (timeout == 0.0f ? 3.0f : timeout) : 0.0f;
	mBlurringStepTimer = 0.0f;
}

void GameView::setLogDisplay(const String& string, float time)
{
	LogDisplay::instance().setLogDisplay(string, time);
}

void GameView::setGameSpeed(float speed)
{
	mSimulation.setSpeed(speed);
	setLogDisplay(String(0, "Emulator speed: %.02f", speed));
}
