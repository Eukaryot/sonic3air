/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	// FTX::Video
	class API_EXPORT VideoManager
	{
	friend class SystemManager;

	public:
		VideoManager();
		~VideoManager();

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
