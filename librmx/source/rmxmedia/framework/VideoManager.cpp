/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
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

	VideoManager::VideoManager()
	{
	}

	VideoManager::~VideoManager()
	{
		// TODO: Do this right here?
		SDL_DestroyWindow(mMainWindow);
	}

	bool VideoManager::setVideoMode(const VideoConfig& videoconfig)
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

	bool VideoManager::initialize(const VideoConfig& videoconfig)
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

	void VideoManager::setInitialized(const VideoConfig& videoconfig, SDL_Window* window)
	{
		mVideoConfig = videoconfig;
		mMainWindow = window;
		mInitialized = true;
	}

	void VideoManager::reshape(int width, int height)
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

	void VideoManager::beginRendering()
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

	void VideoManager::endRendering()
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

	void VideoManager::setPixelView()
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

	void VideoManager::setPerspective2D(double fov, double dnear, double dfar)
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

	void VideoManager::getScreenBitmap(Bitmap& bitmap)
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

	uint64 VideoManager::getNativeWindowHandle() const
	{
	#ifdef PLATFORM_WINDOWS
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWindowWMInfo(mMainWindow, &info))
			return 0;
		return (uint64)info.info.win.window;
	#else
		// TODO: Implement this
		return 0;
	#endif
	}

}
