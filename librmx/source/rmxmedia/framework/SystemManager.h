/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	// FTX::System
	class API_EXPORT SystemManager
	{
	public:
		SystemManager();
		~SystemManager();

		// System
		bool initialize();
		void exit();

		void mainLoop();
		void run(GuiBase& app);

		template<typename T> void run()
		{
			T& t = mRoot.createChild<T>();
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
		bool  getKeyRepeat(int key) const		{ return mInputContext.getKeyRepeat(key); }
		const Vec2i& getMousePos() const		{ return mInputContext.getMousePos();  }
		const Vec2i& getMouseRel() const		{ return mInputContext.getMouseRel();  }
		int   getMouseWheel() const				{ return mInputContext.getMouseWheel(); }
		bool  getMouseState(int button) const	{ return mInputContext.getMouseState(button); }
		bool  getMouseChange(int button) const	{ return mInputContext.getMouseChange(button); }
		bool  mouseIn(const Recti& rect) const	{ return rect.contains(mInputContext.getMousePos()); }
		void  warpMouse(int x, int y);

		// Consumption of the current event (like a keyboard, mouse, update, render call)
		void  consumeCurrentEvent()		{ mCurrentEventConsumed = true; }
		bool  wasEventConsumed() const	{ return mCurrentEventConsumed; }

	private:
		void run();
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

		bool   mInitialized = false;
		bool   mRunning = false;
		uint32 mTicks = 0;
		float  mTotalTime = 0.0f;
		float  mTimeDifference = 0.0f;
		float  mFrameRate = 0.0f;
		int    mFrameCounter = 0;

		InputContext mInputContext;
		bool mCurrentEventConsumed = false;
	};

}
