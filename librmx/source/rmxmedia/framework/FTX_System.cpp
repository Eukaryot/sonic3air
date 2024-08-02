/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#if defined(PLATFORM_WINDOWS)
	#pragma warning(disable: 4005)	// Macro redefinition of APIENTRY

	#if defined(__GNUC__)
		#include <SDL2/SDL_syswm.h>
	#else
		#include <SDL/SDL_syswm.h>
	#endif

	#define WIN32_LEAN_AND_MEAN
	#include "CleanWindowsInclude.h"

#elif defined(PLATFORM_WEB)
	#include <emscripten.h>
	#include <emscripten/html5.h>

#endif


namespace rmx
{

	FTX_SystemManager::FTX_SystemManager()
	{
	}

	FTX_SystemManager::~FTX_SystemManager()
	{
	}

	bool FTX_SystemManager::initialize()
	{
		if (mInitialized)
			return true;

		// Initialize SDL video
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cout << "SDL_Init(SDL_INIT_VIDEO) failed with error: " << SDL_GetError() << "\n";
			return false;
		}

		mInitialized = true;
		return true;
	}

	void FTX_SystemManager::exit()
	{
		// Quit SDL
		SDL_Quit();
	}

	void FTX_SystemManager::startTick()
	{
		// TODO...
	}

	void FTX_SystemManager::checkSDLEvents()
	{
		// Handle events
		InputContext& ctx = mInputContext;
		const Vec2i oldMousePos = ctx.mMousePos;
		bool oldMouseState[5];
		memcpy(oldMouseState, ctx.mMouseState, sizeof(ctx.mMouseState));
		ctx.mMouseWheel = 0;

		// Process SDL event queue
		SDL_Event evnt;
		while (SDL_PollEvent(&evnt))
		{
			switch (evnt.type)
			{
				case SDL_QUIT:
					quit();
					break;

				case SDL_WINDOWEVENT:
					if (evnt.window.event == SDL_WINDOWEVENT_RESIZED || evnt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
						reshape(evnt.window.data1, evnt.window.data2);
					break;

				case SDL_KEYDOWN:
				case SDL_KEYUP:
					keyboard(evnt.key);
					break;

				case SDL_TEXTINPUT:
					textinput(evnt.text);
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					mouse(evnt.button);
					break;

				case SDL_MOUSEWHEEL:
					mousewheel(evnt.wheel);
					break;

				case SDL_MOUSEMOTION:
					ctx.mMousePos.set(evnt.motion.x, evnt.motion.y);
					break;
			}

			mRoot.sdlEvent(evnt);
		}

		// Track changes since previous update
		ctx.mMouseRel = ctx.mMousePos - oldMousePos;
		for (int i = 0; i < 5; ++i)
			ctx.mMouseChange[i] = (ctx.mMouseState[i] != oldMouseState[i]);
	}

	void FTX_SystemManager::reshape(int width, int height)
	{
		if (FTX::Video.valid())
			FTX::Video->reshape(width, height);
	}

	void FTX_SystemManager::keyboard(const SDL_KeyboardEvent& evnt)
	{
		KeyboardEvent ev;
		ev.key = evnt.keysym.sym;
		ev.scancode = evnt.keysym.scancode;
		ev.modifiers = evnt.keysym.mod;
		ev.state = (evnt.type == SDL_KEYDOWN);
		ev.repeat = (evnt.repeat != 0);
		mInputContext.applyEvent(ev);

		mCurrentEventConsumed = false;
		mRoot.keyboard(ev);
	}

	void FTX_SystemManager::textinput(const SDL_TextInputEvent& evnt)
	{
		TextInputEvent ev;
		ev.text.readUnicode((const uint8*)evnt.text, (uint32)strlen(evnt.text), UnicodeEncoding::UTF8);

		mCurrentEventConsumed = false;
		mRoot.textinput(ev);
	}

	void FTX_SystemManager::mouse(const SDL_MouseButtonEvent& evnt)
	{
		// Mouse click
		if (evnt.button < 1 || evnt.button > 7)
			return;

		static const MouseButton buttonMap[3] = { MouseButton::Left, MouseButton::Middle, MouseButton::Right };
		MouseEvent ev;
		ev.button = (evnt.button <= 3) ? buttonMap[evnt.button-1] : (MouseButton)(evnt.button-3);
		ev.state = (evnt.type == SDL_MOUSEBUTTONDOWN);
		ev.position.set(evnt.x, evnt.y);
		mInputContext.applyEvent(ev);

		mCurrentEventConsumed = false;
		mRoot.mouse(ev);
	}

	void FTX_SystemManager::mousewheel(const SDL_MouseWheelEvent& evnt)
	{
		// Mouse wheel
		mInputContext.mMouseWheel += evnt.y;
	}

	void FTX_SystemManager::update()
	{
		// Update timing
		unsigned int oldTicks = mTicks;
		mTicks = SDL_GetTicks();
		mTimeDifference = (float)(mTicks - oldTicks) * 0.001f;
		mTotalTime += mTimeDifference;

		const float dt = clamp(mTimeDifference, 0.001f, 1.0f);
		const float adaption = expf(-dt * 10.0f);
		mFrameRate = (1.0f / dt) * (1.0f - adaption) + mFrameRate * adaption;

		// Update root GuiBase instance
		mCurrentEventConsumed = false;
		mRoot.update(dt);
		++mFrameCounter;
	}

	void FTX_SystemManager::render()
	{
		// Perform rendering
		if (!FTX::Video.valid())
			return;
		if (!FTX::Video->isActive())
			return;

		mCurrentEventConsumed = false;
		FTX::Video->beginRendering();
		mRoot.render();
		FTX::Video->endRendering();
	}

	void FTX_SystemManager::run(GuiBase& app)
	{
		mRoot.addChild(app);
		run();
		mRoot.removeChild(app);
	}

	void FTX_SystemManager::mainLoop()
	{
		startTick();
		checkSDLEvents();
		update();
		render();

#ifdef PLATFORM_WEB
		if (!mRunning)
		{
			emscripten_cancel_main_loop();
			EM_ASM(
				if(Module["onExit"])Module["onExit"]();
			);
		}
#endif
	}

	void loop_func(void* arg)
	{
		FTX_SystemManager* host = (FTX_SystemManager*)arg;
		host->mainLoop();
	}

	void FTX_SystemManager::run()
	{
		// Start
		mTicks = SDL_GetTicks();
		mRunning = true;

#ifdef PLATFORM_WEB
		emscripten_set_main_loop_arg(loop_func, (void*)this, 0, 1);
#else
		// Main loop
		while (mRunning)
		{
			mainLoop();
		}
#endif
	}

	void FTX_SystemManager::quit()
	{
		mRunning = false;
	}

	void FTX_SystemManager::warpMouse(int x, int y)
	{
		SDL_WarpMouseInWindow(FTX::Video->mMainWindow, x, y);
		mInputContext.mMousePos.set(x, y);
	}



	FTX_VideoManager::FTX_VideoManager()
	{
	}

	FTX_VideoManager::~FTX_VideoManager()
	{
		// TODO: Do this right here?
		SDL_DestroyWindow(mMainWindow);
	}

	bool FTX_VideoManager::setVideoMode(const VideoConfig& videoconfig)
	{
		// Change video mode
		uint32 flags = 0;
	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (videoconfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
			flags |= SDL_WINDOW_OPENGL;
		}
	#endif
		if (videoconfig.mFullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}
		else
		{
			if (videoconfig.mBorderless)
				flags |= SDL_WINDOW_BORDERLESS;
			if (videoconfig.mResizeable)
				flags |= SDL_WINDOW_RESIZABLE;
		}

		int startX = SDL_WINDOWPOS_CENTERED_DISPLAY(videoconfig.mDisplayIndex);
		int startY = SDL_WINDOWPOS_CENTERED_DISPLAY(videoconfig.mDisplayIndex);
		if (videoconfig.mPositioning)
		{
			startX = videoconfig.mStartPos.x;
			startY = videoconfig.mStartPos.y;
		}

		mMainWindow = SDL_CreateWindow(*videoconfig.mCaption, startX, startY, videoconfig.mWindowRect.width, videoconfig.mWindowRect.height, flags);

		// Success so far?
		if (nullptr == mMainWindow)
			return false;

	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (videoconfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
			SDL_GL_CreateContext(mMainWindow);
			SDL_GL_SetSwapInterval(videoconfig.mVSync ? 1 : 0);
		}
	#endif

		// Copy video config
		mVideoConfig = videoconfig;

		SDL_GetWindowSize(mMainWindow, &mVideoConfig.mWindowRect.width, &mVideoConfig.mWindowRect.height);
		SDL_ShowCursor(!videoconfig.mHideCursor);

	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (videoconfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
			// Defaults for OpenGL
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		#ifdef ALLOW_LEGACY_OPENGL
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.001f);
		#endif
		}
	#endif
		return true;
	}

	bool FTX_VideoManager::initialize(const VideoConfig& videoconfig)
	{
		// Initialize video mode
		if (!FTX::System->initialize())
			return false;

		// Only change the mode?
		if (mInitialized)
			return setVideoMode(videoconfig);

	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (videoconfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
			// Setup video mode for OpenGL
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, videoconfig.mMultisampling);
		}
	#endif

		if (!setVideoMode(videoconfig))
			return false;

	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (videoconfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef RMX_USE_GLEW
			const GLenum result = glewInit();
			if (result != GLEW_OK)
			{
				RMX_ERROR("Error in OpenGL initialization (glewInit):\n" << glewGetErrorString(result), );
				return false;
			}
		#endif
		#ifdef RMX_USE_GLAD
			gladLoadGL();
		#endif
		}
	#endif

		mInitialized = true;

	#ifdef PLATFORM_WINDOWS
		// Set icon (Windows)
		if (mVideoConfig.mIconResource != 0)
		{
			HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(mVideoConfig.mIconResource));
			SendMessage((HWND)FTX::Video->getNativeWindowHandle(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}
	#endif

		// Set icon (general)
		if (nullptr != mVideoConfig.mIconBitmap.getData() || mVideoConfig.mIconSource.nonEmpty())
		{
			Bitmap tmp;
			Bitmap* bitmap = &mVideoConfig.mIconBitmap;
			if (bitmap->empty())
			{
				bitmap = nullptr;
				if (tmp.load(mVideoConfig.mIconSource.toWString()))
				{
					bitmap = &tmp;
				}
			}

			if (nullptr != bitmap)
			{
				bitmap->rescale(32, 32);
				SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(bitmap->getData(), 32, 32, 32, bitmap->getWidth() * sizeof(uint32), 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
				SDL_SetWindowIcon(mMainWindow, icon);
				SDL_FreeSurface(icon);
			}
		}

		return true;
	}

	void FTX_VideoManager::setInitialized(const VideoConfig& videoconfig, SDL_Window* window)
	{
		mVideoConfig = videoconfig;
		mMainWindow = window;
		mInitialized = true;
	}

	void FTX_VideoManager::reshape(int width, int height)
	{
		// Called e.g. when window size changed
		if (mVideoConfig.mWindowRect.width == width && mVideoConfig.mWindowRect.height == height)
			return;

		mVideoConfig.mWindowRect.width = width;
		mVideoConfig.mWindowRect.height = height;
		mReshaped = true;
	/*
		// Reset video mode
		setVideoMode(mVideoConfig);
	*/
	}

	void FTX_VideoManager::beginRendering()
	{
		if (mVideoConfig.mAutoClearScreen)
		{
		#ifdef RMX_WITH_OPENGL_SUPPORT
			if (mVideoConfig.mRenderer == VideoConfig::Renderer::OPENGL)
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
		#endif
		}
	}

	void FTX_VideoManager::endRendering()
	{
		if (mVideoConfig.mAutoSwapBuffers)
		{
		#ifdef RMX_WITH_OPENGL_SUPPORT
			if (mVideoConfig.mRenderer == VideoConfig::Renderer::OPENGL)
			{
				SDL_GL_SwapWindow(mMainWindow);
			}
		#endif
		}
		mReshaped = false;
	}

	void FTX_VideoManager::setPixelView()
	{
	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (mVideoConfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			// Set 2D view
			glViewport(0, 0, mVideoConfig.mWindowRect.width, mVideoConfig.mWindowRect.height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0.0, mVideoConfig.mWindowRect.width, mVideoConfig.mWindowRect.height, 0.0, +1.0, -1.0);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		#else
			RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
		#endif
		}
	#endif
	}

	void FTX_VideoManager::setPerspective2D(double fov, double dnear, double dfar)
	{
	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (mVideoConfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			// Set perspective 2D view
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			double dx = mVideoConfig.mWindowRect.width * dnear * 0.5;
			double dy = mVideoConfig.mWindowRect.height * dnear * 0.5;
			glFrustum(-dx, dx, dy, -dy, dnear, dfar);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(-mVideoConfig.mWindowRect.width * 0.5f, -mVideoConfig.mWindowRect.height * 0.5f, -1.0f);
		#else
			RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
		#endif
		}
	#endif
	}

	void FTX_VideoManager::getScreenBitmap(Bitmap& bitmap)
	{
	#ifdef RMX_WITH_OPENGL_SUPPORT
		if (mVideoConfig.mRenderer == VideoConfig::Renderer::OPENGL)
		{
			const Recti& screen = getScreenRect();
			bitmap.create(screen.width, screen.height);
			glReadPixels(screen.left, screen.top, screen.width, screen.height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.getData());
			bitmap.mirrorVertical();
		}
	#endif
	}

	uint64 FTX_VideoManager::getNativeWindowHandle() const
	{
	#ifdef PLATFORM_WINDOWS
		SDL_SysWMinfo info;
		SDL_GetWindowWMInfo(mMainWindow, &info);
		return (uint64)info.info.win.window;
	#else
		// TODO: Implement this
		return 0;
	#endif
	}

}
