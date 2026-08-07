/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"
#pragma warning(disable: 4005)	// Macro redefinition of APIENTRY


namespace rmx
{

	VideoConfig::VideoConfig()
	{
		mWindowRect.set(0, 0, 800, 500);
		mCaption = "FTX Window";
	}

	VideoConfig::VideoConfig(bool fullscreen, int width, int height, const String& caption)
	{
		mFullscreen = fullscreen;
		mResizeable = !fullscreen;
		mWindowRect.set(0, 0, width, height);
		mColorDepth = 32;
		mCaption = "FTX Window";
		mMultisampling = 0;
		mAutoClearScreen = true;
		mAutoSwapBuffers = true;
		if (caption.nonEmpty())
			mCaption = caption;
	}

}


namespace FTX
{
	int   screenWidth()			{ return FTX::Video->getScreenWidth(); }
	int   screenHeight()		{ return FTX::Video->getScreenHeight(); }
	Vec2i screenSize()			{ return FTX::Video->getScreenSize(); }
	const Recti& screenRect()	{ return FTX::Video->getScreenRect(); }
	bool  reshaped()			{ return FTX::Video->reshaped(); }

	float getTime()				{ return FTX::System->getTime(); }
	float getTimeDifference()	{ return FTX::System->getTimeDifference(); }
	float getFramerate()		{ return FTX::System->getFramerate(); }
	int   getFrameCounter()		{ return FTX::System->getFrameCounter(); }

	bool keyState(int key)				{ return FTX::System->getKeyState(key); }
	bool keyChange(int key)				{ return FTX::System->getKeyChange(key); }
	bool keyRepeat(int key)				{ return FTX::System->getKeyRepeat(key); }
	bool keyPressed(int key)			{ return keyState(key) && keyChange(key); }
	bool keyPressedOrRepeat(int key)	{ return keyPressed(key) || keyRepeat(key); }
	bool keyReleased(int key)			{ return !keyState(key) && keyChange(key); }

	const Vec2i& mousePos()						{ return FTX::System->getMousePos(); }
	const Vec2i& mouseRel()						{ return FTX::System->getMouseRel(); }
	int mouseWheel()							{ return FTX::System->getMouseWheel(); }
	bool mouseState(rmx::MouseButton button)	{ return FTX::System->getMouseState((int)button); }
	bool mouseChange(rmx::MouseButton button)	{ return FTX::System->getMouseChange((int)button); }
	bool mousePressed(rmx::MouseButton button)	{ return mouseState(button) && mouseChange(button); }
	bool mouseReleased(rmx::MouseButton button)	{ return !mouseState(button) && mouseChange(button); }
	bool mouseIn(const Recti& rect)				{ return rect.contains(FTX::System->getMousePos()); }
}
