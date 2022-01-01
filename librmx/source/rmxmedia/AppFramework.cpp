/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"
#pragma warning(disable: 4005)	// Macro redefinition of APIENTRY


namespace rmx
{

	VideoConfig::VideoConfig()
	{
		memset(this, 0, sizeof(VideoConfig) - sizeof(String) * 2);
		rect.set(0, 0, 800, 500);
		caption = "FTX Window";
	}

	VideoConfig::VideoConfig(bool fullscr, int wid, int hgt, const String& caption_)
	{
		memset(this, 0, sizeof(VideoConfig) - sizeof(String) * 2);
		fullscreen = fullscr;
		resizeable = !fullscr;
		rect.setSize(wid, hgt);
		bpp = 32;
		caption = "FTX Window";
		multisampling = 0;
		autoclearscreen = true;
		autoswapbuffers = true;
		if (caption_.nonEmpty())
			caption = caption_;
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
		static_assert(SDL_NUM_SCANCODES == 0x0200);
		const int entryIndex = (ev.key & 0x01ff) / 32;
		const int keyBitmask = 1 << (ev.key % 32);

		const bool oldState = (mKeyState[entryIndex] & keyBitmask) != 0;
		const bool change = (oldState != ev.state);
		if (change)
		{
			mKeyChange[entryIndex] |= keyBitmask;
			if (ev.state)
				mKeyState[entryIndex] |= keyBitmask;
			else
				mKeyState[entryIndex] &= ~keyBitmask;
		}
		else
		{
			mKeyChange[entryIndex] &= ~keyBitmask;
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
