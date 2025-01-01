/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
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



	InputContext::InputContext()
	{
		memset(this, 0, sizeof(InputContext));
	}

	InputContext::~InputContext()
	{
	}

	void InputContext::copy(const InputContext& source)
	{
		*this = source;
	}

	void InputContext::applyEvent(const KeyboardEvent& ev)
	{
		static_assert(SDL_NUM_SCANCODES == 0x0200);		// That's actually only partially relevant, as we're using keycodes, not scancodes
		const size_t bitIndex = getBitIndex(ev.key);

		const bool oldState = mKeyState.isBitSet(bitIndex);
		const bool change = (oldState != ev.state);
		if (change)
		{
			mKeyState.setBit(bitIndex, ev.state);
			mKeyChange.setBit(bitIndex);
		}
		else
		{
			mKeyChange.clearBit(bitIndex);
		}
	}

	void InputContext::applyEvent(const MouseEvent& ev)
	{
		mMouseChange[(int)ev.button] = (ev.state != mMouseState[(int)ev.button]);
		mMouseState[(int)ev.button] = ev.state;
	}

	bool InputContext::getMouseState(int button) const
	{
		if (button < 0 || button > 4)
			return false;
		return mMouseState[button];
	}

	bool InputContext::getMouseChange(int button) const
	{
		if (button < 0 || button > 4)
			return false;
		return mMouseChange[button];
	}
}


namespace FTX
{
	int   screenWidth()			{ return FTX::Video->getScreenWidth(); }
	int   screenHeight()		{ return FTX::Video->getScreenHeight(); }
    Vec2i screenSize()		    { return FTX::Video->getScreenSize(); }
	const Recti& screenRect()	{ return FTX::Video->getScreenRect(); }
	bool  reshaped()			{ return FTX::Video->reshaped(); }

	float getTime()				{ return FTX::System->getTime(); }
	float getTimeDifference()	{ return FTX::System->getTimeDifference(); }
	float getFramerate()		{ return FTX::System->getFramerate(); }
	int   getFrameCounter()		{ return FTX::System->getFrameCounter(); }

	bool keyState(int key)		{ return FTX::System->getKeyState(key); }
	bool keyChange(int key)		{ return FTX::System->getKeyChange(key); }

	const Vec2i& mousePos()			{ return FTX::System->getMousePos(); }
	const Vec2i& mouseRel()			{ return FTX::System->getMouseRel(); }
	int mouseWheel()				{ return FTX::System->getMouseWheel(); }
	bool mouseState(rmx::MouseButton button)	{ return FTX::System->getMouseState((int)button); }
	bool mouseChange(rmx::MouseButton button)	{ return FTX::System->getMouseChange((int)button); }
	bool mouseIn(const Recti& rect)	{ return rect.contains(FTX::System->getMousePos()); }
}
