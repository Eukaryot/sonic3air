/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/Application.h"
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
#include "oxygen/resources/FontCollection.h"
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
		const PaletteManager& paletteManager = VideoOut::instance().getRenderParts().getPaletteManager();
		const uint32* palette = paletteManager.getMainPalette(paletteIndex).getRawColors();

		PaletteBitmap bmp;
		bmp.create(16, 16);
		for (int colorIndex = 0; colorIndex < 0x100; ++colorIndex)
		{
			bmp.getData()[colorIndex] = (uint8)colorIndex;
		}

		std::vector<uint8> fileContent;
		if (!bmp.saveBMP(fileContent, palette))
			return false;
		return FTX::FileSystem->saveFile("palette_dump.bmp", fileContent);
	}

	// Function is not used any more
#if 0
	bool dumpPaletteAsPAL(int paletteIndex)
	{
		const PaletteManager& paletteManager = VideoOut::instance().getRenderParts().getPaletteManager();
		String str;
		str << "JASC-PAL\r\n";
		str << "0100\r\n";
		str << "256\r\n";
		for (int colorIndex = 0; colorIndex < 64; ++colorIndex)
		{
			const Color color = paletteManager.getPalette(paletteIndex).getColor(colorIndex);
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
	const Configuration& config = Configuration::instance();
	const Recti gameScreenRect = VideoOut::instance().getScreenRect();

	switch (config.mUpscaling)
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

			if (!config.mDevMode.mEnabled)	// If dev mode is enabled, integer scale is handled differently, see below
			{
				const int scale = mGameViewport.height / gameScreenRect.height;
				if (scale >= 1)
				{
					mGameViewport.width = roundToInt((float)gameScreenRect.width * scale);
					mGameViewport.height = roundToInt((float)gameScreenRect.height * scale);
					mGameViewport.x = mRect.x + (mRect.width - mGameViewport.width) / 2;
					mGameViewport.y = mRect.y + (mRect.height - mGameViewport.height) / 2;
				}
			}
			break;
		}

		case 2:
		{
			// Halfway stretch to fill
			const Recti letterBox = RenderUtils::getLetterBoxRect(mRect, gameScreenRect.getAspectRatio());
			mGameViewport.setPos((letterBox.getPos() + mRect.getPos()) / 2);		// Average between letter box and full stretch
			mGameViewport.setSize((letterBox.getSize() + mRect.getSize()) / 2);
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

	if (config.mDevMode.mEnabled)
	{
		// Consider integer scaling
		Vec2f scaledSize = Vec2f(mGameViewport.getSize()) * config.mDevMode.mGameViewScale;
		if (config.mUpscaling == 1)
		{
			const int scale = std::max((int)((float)mGameViewport.height * config.mDevMode.mGameViewScale) / gameScreenRect.height, 1);
			scaledSize = Vec2f(gameScreenRect.getSize() * scale);
		}

		const Vec2f maxPos = Vec2f(mRect.getEndPos()) - scaledSize;
		mGameViewport.x = roundToInt(maxPos.x * (1.0f + config.mDevMode.mGameViewAlignment.x) / 2.0f);
		mGameViewport.y = roundToInt(maxPos.y * (1.0f + config.mDevMode.mGameViewAlignment.y) / 2.0f);
		mGameViewport.setSize(Vec2i(scaledSize));
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
	removeAllChildren();
}

void GameView::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);

	if (ev.state && !FTX::System->wasEventConsumed())
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
							mDebugVisualizations.mDebugPaletteDisplay = (mDebugVisualizations.mDebugPaletteDisplay + 2) % 3 - 1;
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
							HighResolutionTimer timer;
							timer.start();
							RenderResources::instance().loadSprites();
							ResourcesCache::instance().loadAllResources();
							FontCollection::instance().reloadAll();
							setLogDisplay(String(0, "Reloaded resources in %0.2f sec", timer.getSecondsSinceStart()));
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
							toggle(mDebugVisualizations.mEnabled);
							break;
						}
						case 'c':
						{
							++mDebugVisualizations.mMode;
							break;
						}

						case 't':
						{
							mSimulation.getCodeExec().executeScriptFunction("OxygenCallback.debugAltT", false);
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

						case SDLK_F7:
						{
							mSimulation.reloadLastState();
							setLogDisplay("Reset emulator interface");
							break;
						}

						case SDLK_F11:
						case SDLK_BACKQUOTE:	// Alternative, especially for Macs, where F11 has other functions already
						{
							HighResolutionTimer timer;
							timer.start();
							if (mSimulation.triggerFullScriptsReload())
							{
								setLogDisplay(String(0, "Reloaded scripts in %0.2f sec", timer.getSecondsSinceStart()));
							}
							break;
						}

						case SDLK_TAB:
						{
							if (mDebugVisualizations.mDebugOutput >= 0)
							{
								VideoOut::instance().dumpDebugDraw(mDebugVisualizations.mDebugOutput);
								setLogDisplay("Dumped debug output to BMP file");
							}
							else if (mDebugVisualizations.mDebugPaletteDisplay >= 0)
							{
								dumpPaletteAsBMP(mDebugVisualizations.mDebugPaletteDisplay);
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
						// Supporting both Shift and Ctrl, as the Shift + Peroid combination does not seem to work for all keyboards
						const bool continueToDebugEvent = FTX::keyState(SDLK_LSHIFT) || FTX::keyState(SDLK_LCTRL);
						mSimulation.setNextSingleStep(true, continueToDebugEvent);
						if (!continueToDebugEvent)
							setLogDisplay(String(0, "Single step | Frame: %d", mSimulation.getFrameNumber() + 1));
						break;
					}
				}
			}
		}
	}
}

void GameView::mouse(const rmx::MouseEvent& ev)
{
	if (FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
	{
		const char* functionName = nullptr;
		if (ev.button == rmx::MouseButton::Left)
		{
			functionName = ev.state ? "OxygenCallback.debugAltLeftMouseDown" : "OxygenCallback.debugAltLeftMouseUp";
		}
		else if (ev.button == rmx::MouseButton::Right)
		{
			functionName = ev.state ? "OxygenCallback.debugAltRightMouseDown" : "OxygenCallback.debugAltRightMouseUp";
		}

		if (nullptr != functionName)
		{
			Vec2f relativePosition;
			if (translatePositionIntoGameViewport(relativePosition, Vec2f(ev.position)))
			{
				const uint8 flags = (FTX::keyState(SDLK_LSHIFT) || FTX::keyState(SDLK_RSHIFT)) ? 0x01 : 0x00;

				CodeExec::FunctionExecData execData;
				execData.addParam(lemon::PredefinedDataTypes::INT_16, roundToInt(relativePosition.x));
				execData.addParam(lemon::PredefinedDataTypes::INT_16, roundToInt(relativePosition.y));
				execData.addParam(lemon::PredefinedDataTypes::UINT_8, flags);
				mSimulation.getCodeExec().executeScriptFunction(functionName, false, &execData);
			}
		}
	}
}

void GameView::earlyUpdate(float timeElapsed)
{
	// Update children earlier than GameView's own siblings, especially S3AIR's MenuBackground before GameApp
	GuiBase::update(timeElapsed);
}

void GameView::update(float timeElapsed)
{
	if (mFadeChange != 0.0f)
	{
		mFadeValue = saturate(mFadeValue + timeElapsed * mFadeChange);
	}

	if (mStillImage.mMode == StillImageMode::BLURRING)
	{
		if (mStillImage.mBlurringTimeout > 0.0f)
		{
			mStillImage.mBlurringTimeout -= timeElapsed;
			mStillImage.mBlurringStepTimer += timeElapsed;
		}
		if (mStillImage.mBlurringTimeout <= 0.0f)
		{
			mStillImage.mBlurringTimeout = 0.0f;
			mStillImage.mMode = StillImageMode::STILL_IMAGE;
		}
	}

	if (EngineMain::getDelegate().useDeveloperFeatures() && FTX::keyState(SDLK_KP_9) && mSimulation.getFrameNumber() > 0)
	{
		int rewindSteps = 0;
		if (mRewindCounter == 0)
		{
			// Rewind in the first frame the key was just pressed
			rewindSteps = 1;
		}
		else
		{
			mRewindTimer += (SDL_GetModState() & KMOD_SHIFT) ? (timeElapsed * 3.0f) : timeElapsed;
			const float speed = (mRewindCounter == 1) ? 5 : (float)std::min(10 + mRewindCounter / 3, 120);
			const float delay = 1.0f / speed;
			if (mRewindTimer >= delay)
			{
				rewindSteps = (int)std::floor(mRewindTimer / delay);
				mRewindTimer -= delay * (float)rewindSteps;
			}
		}

		if (rewindSteps > 0)
		{
			setLogDisplay(String(0, "  Rewinding | Frame: %d", mSimulation.getFrameNumber() - rewindSteps));
			mSimulation.setRewind(rewindSteps);
			++mRewindCounter;
		}
	}
	else
	{
		mRewindTimer = 0.0f;
		mRewindCounter = 0;
	}

	// Debug output
	{
		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			if (!FTX::System->wasEventConsumed() && (FTX::keyChange(',') || FTX::keyChange('.') || FTX::keyChange('-')))
			{
				mDebugVisualizations.mDebugOutput = -1;
				if (FTX::keyState(','))
				{
					// Debug output for plane B
					mDebugVisualizations.mDebugOutput = 0;
				}
				else if (FTX::keyState('.'))
				{
					// Debug output for plane A or W
					mDebugVisualizations.mDebugOutput = (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT)) ? 2 : 1;
				}
				else if (FTX::keyState('-'))
				{
					// Debug output for patterns
					mDebugVisualizations.mDebugOutput = 3;
				}
			}

			DebugTracking& debugTracking = Application::instance().getSimulation().getCodeExec().getDebugTracking();
			debugTracking.clearScriptLogValue("~index");
			debugTracking.clearScriptLogValue("~addr");
			debugTracking.clearScriptLogValue("~ptrn");

			if (mDebugVisualizations.mDebugOutput >= 0)
			{
				// Get the mouse position inside the debug output
				PlaneManager& planeManager = VideoOut::instance().getRenderParts().getPlaneManager();
				Rectf rect;
				if (mDebugVisualizations.mDebugOutput <= PlaneManager::PLANE_A)
				{
					const Vec2i playfieldSize = planeManager.getPlayfieldSizeInPixels();
					rect = RenderUtils::getLetterBoxRect(mRect, (float)playfieldSize.x / (float)playfieldSize.y);
				}
				else
				{
					rect = RenderUtils::getLetterBoxRect(mRect, 2.0f);
				}

				Vec2f relativePosition;
				if (translatePositionIntoRect(relativePosition, rect, Vec2f(FTX::mousePos())))
				{
					const uint32 index = (int)(relativePosition.x * 64.0f) + (int)(relativePosition.y * 32.0f) * 64;
					debugTracking.updateScriptLogValue("~index", rmx::hexString(index, 4));
					if (mDebugVisualizations.mDebugOutput < 2)
					{
						const uint16 address = planeManager.getPatternVRAMAddress(mDebugVisualizations.mDebugOutput, (uint16)index);
						const uint16 pattern = planeManager.getPatternAtIndex(mDebugVisualizations.mDebugOutput, (uint16)index);
						debugTracking.updateScriptLogValue("~addr", rmx::hexString(address, 4));
						debugTracking.updateScriptLogValue("~ptrn", rmx::hexString(pattern, 4));
					}
					else
					{
						debugTracking.updateScriptLogValue("~addr", rmx::hexString((uint32)(index * 0x20), 4));
					}
				}
			}
		}
	}
}

void GameView::render()
{
	mRect = FTX::screenRect();

	const Configuration& config = Configuration::instance();
	Drawer& drawer = EngineMain::instance().getDrawer();
	VideoOut& videoOut = VideoOut::instance();
	const Recti gameScreenRect = VideoOut::instance().getScreenRect();
	mFinalGameTexture.setupAsRenderTarget(gameScreenRect.width, gameScreenRect.height);

	// Refresh simulation output image
	if (mStillImage.mMode != StillImageMode::NONE)
	{
		const constexpr float REDUCTION = 0.0333f;	// One blur step every 1/30 second
		if (mStillImage.mBlurringStepTimer >= REDUCTION)
		{
			mStillImage.mBlurringStepTimer -= REDUCTION;
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

	if (mDebugVisualizations.mDebugOutput >= 0)
	{
		// Draw a dark background over the full screen
		drawer.setBlendMode(BlendMode::OPAQUE);
		drawer.drawRect(FTX::screenRect(), Color(0.15f, 0.15f, 0.15f));
		drawer.performRendering();

		videoOut.renderDebugDraw(mDebugVisualizations.mDebugOutput, mRect);

		// Enable alpha again for the UI
		drawer.setBlendMode(BlendMode::ALPHA);
		drawer.performRendering();

		// No "GuiBase::render()" call here, as this would e.g. draw menus on top (and in wrong resolutions)
		return;
	}

	// Here goes the real rendering
	drawer.setRenderTarget(mFinalGameTexture, gameScreenRect);
	drawer.setBlendMode(BlendMode::OPAQUE);

	// Simple mirror mode implementation: Just mirror the whole screen
	if (config.mMirrorMode)
	{
		drawer.drawRect(gameScreenRect, videoOut.getGameScreenTexture(), Vec2f(1.0f, 0.0f), Vec2f(0.0f, 1.0f), Color::WHITE);
	}
	else
	{
		drawer.drawRect(gameScreenRect, videoOut.getGameScreenTexture());
	}

	// Enable alpha for the UI
	drawer.setBlendMode(BlendMode::ALPHA);

	// Highlight rects (from rendered geometry dev mode window)
	for (const std::pair<Recti, Color>& pair : mScreenHighlightRects)
	{
		drawer.drawRect(pair.first, pair.second);
	}
	mScreenHighlightRects.clear();

	// Debug visualizations
	if (mDebugVisualizations.mEnabled)
	{
		if (!mDebugVisualizationsOverlay.isValid())
		{
			drawer.createTexture(mDebugVisualizationsOverlay);
		}
		Bitmap& bitmap = mDebugVisualizationsOverlay.accessBitmap();
		bitmap.resize(gameScreenRect.width, gameScreenRect.height);
		bitmap.clear(0);
		EngineMain::getDelegate().fillDebugVisualization(bitmap, mDebugVisualizations.mMode);
		mDebugVisualizationsOverlay.bitmapUpdated();

		drawer.drawRect(gameScreenRect, mDebugVisualizationsOverlay);
	}

	// Palette debug output
	if (mDebugVisualizations.mDebugPaletteDisplay >= 0)
	{
		const PaletteManager& paletteManager = videoOut.getRenderParts().getPaletteManager();
		for (int paletteIndex = 0; paletteIndex <= mDebugVisualizations.mDebugPaletteDisplay; ++paletteIndex)
		{
			const int baseX = 2 + paletteIndex * 84;
			const int baseY = 2;
			drawer.drawRect(Recti(baseX, baseY, 81, 59), Color::BLACK);

			int py = baseY + 1;
			for (int k = 0; k < 0x100; ++k)
			{
				const int px = baseX + 1 + (k & 0x0f) * 5;
				const int height = (k < 0x40) ? 4 : 2;
				Color color = paletteManager.getMainPalette(paletteIndex).getColor(k);
				color.a = 1.0f;
				drawer.drawRect(Recti(px, py, 4, height), color);

				if ((k & 0x0f) == 0x0f)
				{
					py += height;
					py += (k == 0x3f) ? 3 : 1;
				}
			}
		}
		if (mDebugVisualizations.mDebugPaletteDisplay >= 1)
		{
			const int baseX = 2 + 2 * 84;
			const int baseY = 2;
			drawer.drawRect(Recti(baseX, baseY, 5, 50), Color::BLACK);

			for (int k = 0; k < 16; ++k)
			{
				const float fraction = (float)k / 15.0f;
				const Color color1 = Color(fraction, fraction, fraction);
				const Color color2 = paletteManager.getGlobalComponentAddedColor() + color1 * paletteManager.getGlobalComponentTintColor();
				drawer.drawRect(Recti(baseX + 1, baseY + 1 + k * 3, 3, 3), color2);
				drawer.drawRect(Recti(baseX + 3, baseY + 1 + k * 3, 1, 1), color1);
			}
		}

	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (drawer.getType() == Drawer::Type::OPENGL)
		{
			FTX::Painter->setColor(Color::WHITE);	// Prevent possible broken display in UI (e.g. in S3AIR's menus)
		}
	#endif
	}
	drawer.performRendering();

	// Render the (pixelated) game UI
	mRect = gameScreenRect;
	GuiBase::render();

	// White overlay (used in Time Attack restart)
	if (mWhiteOverlayAlpha > 0.0f)
	{
		drawer.drawRect(gameScreenRect, Color(1.0f, 1.0f, 1.0f, mWhiteOverlayAlpha));
	}

	// Fade from / to black
	if (mFadeValue < 1.0f)
	{
		drawer.drawRect(gameScreenRect, Color(0.0f, 0.0f, 0.0f, 1.0f - mFadeValue));
	}

	if (config.mPerformanceDisplay == 1)
	{
		// Show frame rate, using pixelated display
		const double averageTime = Profiling::getRootRegion().mAverageTime;
		if (averageTime > 0.0)
		{
			drawer.printText(EngineMain::getDelegate().getDebugFont(3), Vec2i(gameScreenRect.width - 3, 2), String(0, "%d FPS", roundToInt((float)(1.0 / averageTime))), 3);
		}
	}

	// Draw the combined image
	drawer.setWindowRenderTarget(FTX::screenRect());
	drawer.setBlendMode(BlendMode::OPAQUE);
	drawer.drawUpscaledRect(mGameViewport, mFinalGameTexture);

	if (!FTX::Video->getVideoConfig().mAutoClearScreen)
	{
		// Draw black bars so no screen clearing is needed
		const int x1 = mGameViewport.x;
		const int x2 = mGameViewport.x + mGameViewport.width;
		const int x3 = FTX::Video->getScreenWidth();
		const int y1 = mGameViewport.y;
		const int y2 = mGameViewport.y + mGameViewport.height;
		const int y3 = FTX::Video->getScreenHeight();

		drawer.drawRect(Recti(0, 0, x3, y1), Color::BLACK);
		drawer.drawRect(Recti(0, y2, x3, y3 - y2), Color::BLACK);
		drawer.drawRect(Recti(0, y1, x1, y2 - y1), Color::BLACK);
		drawer.drawRect(Recti(x2, y1, x3 - x2, y2 - y1), Color::BLACK);
	}

	// Enable alpha again
	drawer.setBlendMode(BlendMode::ALPHA);

	drawer.performRendering();
}

void GameView::setFadedIn()
{
	mFadeValue = 1.0f;
	mFadeChange = 0.0f;
	setStillImageMode(StillImageMode::NONE);
}

void GameView::startFadingIn(float fadeTime)
{
	mFadeValue = 0.0f;
	mFadeChange = 1.0f / fadeTime;
	setStillImageMode(StillImageMode::NONE);
}

void GameView::startFadingOut(float fadeTime)
{
	mFadeChange = -1.0f / fadeTime;
	setStillImageMode(StillImageMode::NONE);
}

void GameView::getScreenshot(Bitmap& outBitmap)
{
	mFinalGameTexture.writeContentToBitmap(outBitmap);
}

void GameView::setStillImageMode(StillImageMode mode, float timeout)
{
	mStillImage.mMode = mode;
	mStillImage.mBlurringTimeout = (mode != StillImageMode::NONE) ? (timeout == 0.0f ? 3.0f : timeout) : 0.0f;
	mStillImage.mBlurringStepTimer = 0.0f;
}

void GameView::addScreenHighlightRect(const Recti& rect, const Color& color)
{
	mScreenHighlightRects.emplace_back(rect, color);
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
