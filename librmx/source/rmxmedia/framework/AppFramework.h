/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	AppFramework
*		Management of the application, incl. input and output.
*/

#pragma once


namespace rmx
{

	struct VideoConfig
	{
		enum class Renderer
		{
			SOFTWARE,
			OPENGL
		};

		Recti mWindowRect;				// Window rect on screen
		Vec2i mStartPos;				// Upper left corner
		int mColorDepth = 32;			// Fullscreen only: Color depth in bits per pixel
		bool mFullscreen = false;		// (Exclusive) fullscreen or window mode?
		bool mPositioning = false;		// Window mode only: Use "mStartPos"
		bool mResizeable = true;		// Window mode only: Resizable window
		bool mBorderless = false;		// Window mode only: Hide window frame / borderless window
		bool mVSync = true;				// Use vertical sync
		int mDisplayIndex = 0;			// Display index
		bool mHideCursor = false;		// Show or hide mouse cursor
		int mMultisampling = 0;			// Multisampling setting, usually 0 to disable
		bool mAutoClearScreen = true;	// Automatically clear screen before rendering
		bool mAutoSwapBuffers = true;	// Automatically swap buffers after rendering
		int mIconResource = 0;			// Resource number of icon
		Bitmap mIconBitmap;				// Bitmap of icon, as an alternative for "iconSource"
		String mIconSource;				// Source file for icon (in PNG format)
		String mCaption;				// Caption text for window
		Renderer mRenderer = Renderer::OPENGL;

		VideoConfig();
		VideoConfig(bool fullscreen, int width, int height, const String& caption = "");
	};

}


namespace FTX
{
	// Video
	int   screenWidth();
	int   screenHeight();
	Vec2i screenSize();
	const Recti& screenRect();
	bool  reshaped();

	// Timing
	float getTimeDifference();
	float getTime();
	float getFramerate();
	int   getFrameCounter();

	// Keyboard
	bool keyState(int key);
	bool keyChange(int key);
	bool keyRepeat(int key);
	bool keyPressed(int key);
	bool keyPressedOrRepeat(int key);
	bool keyReleased(int key);

	// Mouse
	const Vec2i& mousePos();
	const Vec2i& mouseRel();
	int  mouseWheel();
	bool mouseState(rmx::MouseButton button);
	bool mouseChange(rmx::MouseButton button);
	bool mousePressed(rmx::MouseButton button);
	bool mouseReleased(rmx::MouseButton button);
	bool mouseIn(const Recti& rect);
}
