/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	FTX_System
*		FTX system manager class.
*/

#pragma once


namespace rmx
{

	// FTX::System
	class API_EXPORT FTX_SystemManager
	{
	public:
		FTX_SystemManager();
		~FTX_SystemManager();

		// System
		bool initialize();
		void exit();

		void mainLoop();
		void run(GuiBase& app);

		template<typename T> void run()
		{
			T* t = mRoot.createChild<T>();
			run();
			mRoot.deleteChild(t);
		}

		void quit();

		// Time measurement
		float getTime() const			{ return mTotalTime; }
		float getTimeDifference() const	{ return mTimeDifference; }
		float getFramerate() const		{ return mFrameRate; }
		int   getFrameCounter() const	{ return mFrameCounter; }

		// Input
		bool  getKeyState(int key) const		{ return mInputContext.getKeyState(key); }
		bool  getKeyChange(int key) const		{ return mInputContext.getKeyChange(key); }
		const Vec2i& getMousePos() const		{ return mInputContext.mMousePos;  }
		const Vec2i& getMouseRel() const		{ return mInputContext.mMouseRel;  }
		int   getMouseWheel() const				{ return mInputContext.mMouseWheel; }
		bool  getMouseState(int button) const	{ return mInputContext.getMouseState(button); }
		bool  getMouseChange(int button) const	{ return mInputContext.getMouseChange(button); }
		bool  mouseIn(const Recti& rect) const	{ return rect.contains(mInputContext.mMousePos); }
		void  warpMouse(int x, int y);

	private:
		void run();
		void startTick();
		void checkSDLEvents();
		void reshape(int width, int height);
		void keyboard(const SDL_KeyboardEvent& ev);
		void textinput(const SDL_TextInputEvent& ev);
		void mouse(const SDL_MouseButtonEvent& ev);
		void mousewheel(const SDL_MouseWheelEvent& ev);
		void update();
		void render();

	private:
		// Root element
		GuiBase mRoot;

		InputContext mInputContext;
		bool   mInitialized = false;
		bool   mRunning = false;
		uint32 mTicks = 0;
		float  mTotalTime = 0.0f;
		float  mTimeDifference = 0.0f;
		float  mFrameRate = 0.0f;
		int    mFrameCounter = 0;
	};


	// FTX::Video
	class API_EXPORT FTX_VideoManager
	{
	friend class FTX_SystemManager;

	public:
		FTX_VideoManager();
		~FTX_VideoManager();

		bool isActive() const  { return mInitialized; }

		// System
		bool initialize(const VideoConfig& videoconfig);
		void setInitialized(const VideoConfig& videoconfig, SDL_Window* window);

		void reshape(int width, int height);

		void beginRendering();
		void endRendering();

		// Video
		const VideoConfig& getVideoConfig() const { return mVideoConfig; }
		void setAutoClearScreen(bool enable)	{ mVideoConfig.mAutoClearScreen = enable; }
		void setAutoSwapBuffers(bool enable)	{ mVideoConfig.mAutoSwapBuffers = enable; }

		int getScreenWidth() const				{ return mVideoConfig.mWindowRect.width; }
		int getScreenHeight() const				{ return mVideoConfig.mWindowRect.height; }
		Vec2i getScreenSize() const				{ return mVideoConfig.mWindowRect.getSize(); }
		const Recti& getScreenRect() const		{ return mVideoConfig.mWindowRect; }

		bool reshaped() const					{ return mReshaped; }

		void setPixelView();
		void setPerspective2D(double fov, double dnear, double dfar);
		void getScreenBitmap(Bitmap& bitmap);

		uint64 getNativeWindowHandle() const;
		SDL_Window* getMainWindow() const { return mMainWindow; }

	private:
		bool setVideoMode(const VideoConfig& videoconfig);

	private:
		bool mInitialized = false;
		VideoConfig mVideoConfig;
		bool mReshaped = false;
		SDL_Window* mMainWindow = nullptr;
	};

}
