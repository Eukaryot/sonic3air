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

	void InputContext::copy(const InputContext& source)
	{
		*this = source;
	}

	void InputContext::backupState()
	{
		mKeyOldState = mKeyState;
		mKeyRepeat.clearAllBits();
		mMouseOldPos = mMousePos;
		memcpy(mMouseButtonOldState, mMouseButtonState, sizeof(mMouseButtonState));
		mMouseWheel = 0;
	}

	void InputContext::refreshChangeFlags()
	{
		mKeyChange.makeXOR(mKeyState, mKeyOldState);
		mMouseRel = mMousePos - mMouseOldPos;
		for (int i = 0; i < 5; ++i)
			mMouseButtonChange[i] = (mMouseButtonState[i] != mMouseButtonOldState[i]);
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
		if (ev.repeat)
		{
			mKeyRepeat.setBit(bitIndex);
		}
	}

	void InputContext::applyEvent(const MouseEvent& ev)
	{
		mMouseButtonChange[(int)ev.button] = (ev.state != mMouseButtonState[(int)ev.button]);
		mMouseButtonState[(int)ev.button] = ev.state;
	}

	void InputContext::applyMousePos(int x, int y)
	{
		mMousePos.set(x, y);
	}

	void InputContext::applyMouseWheel(int diff)
	{
		mMouseWheel += diff;
	}

	bool InputContext::getMouseState(int button) const
	{
		if (button < 0 || button > 4)
			return false;
		return mMouseButtonState[button];
	}

	bool InputContext::getMouseChange(int button) const
	{
		if (button < 0 || button > 4)
			return false;
		return mMouseButtonChange[button];
	}
}
