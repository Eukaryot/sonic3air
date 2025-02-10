/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputManager.h"


class ControlsIn : public SingleInstance<ControlsIn>
{
public:
	enum class Button
	{
		UP    = 0x0001,
		DOWN  = 0x0002,
		LEFT  = 0x0004,
		RIGHT = 0x0008,
		B     = 0x0010,
		C     = 0x0020,
		A     = 0x0040,
		START = 0x0080,
		Z     = 0x0100,
		Y     = 0x0200,
		X     = 0x0400,
		MODE  = 0x0800
	};

	struct Gamepad
	{
		uint16 mCurrentInput = 0;
		uint16 mPreviousInput = 0;
		uint16 mIgnoreInput = 0;
		bool mInputWasInjected = false;

		inline bool isPressed(Button button) const		{ return (mCurrentInput & (uint16)button); }
		inline bool justPressed(Button button) const	{ return ((mCurrentInput ^ mPreviousInput) & (uint16)button); }
	};

	static const size_t NUM_GAMEPADS = InputManager::NUM_PLAYERS;

public:
	ControlsIn();

	void beginInputUpdate();
	void endInputUpdate();

	uint16 getInputFromController(uint32 padIndex) const;

	void injectInput(uint32 padIndex, uint16 inputFlags);
	void injectInputs(const uint16* inputFlags, size_t numInputs = InputManager::NUM_PLAYERS);
	void injectEmptyInputs(size_t numInputs = InputManager::NUM_PLAYERS);

	void setIgnores(uint16 bitmask);
	void setAllIgnores();

	Gamepad& getGamepad(size_t index);
	const Gamepad& getGamepad(size_t index) const;
	void writeCurrentState(uint16* outInputFlags, size_t numInputs = InputManager::NUM_PLAYERS) const;

	inline bool areGamepadsSwitched() const  { return mGamepadsSwitched; }
	bool switchGamepads();

private:
	Gamepad mGamepad[NUM_GAMEPADS];
	bool mGamepadsSwitched = false;
};
