/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/Configuration.h"


namespace
{
	std::map<const InputManager::Control*, uint16> inputFlagsLookup[ControlsIn::NUM_GAMEPADS];
}


ControlsIn::ControlsIn()
{
	for (int controllerIndex = 0; controllerIndex < NUM_GAMEPADS; ++controllerIndex)
	{
		auto& map = inputFlagsLookup[controllerIndex];
		const InputManager::ControllerScheme& controller = InputManager::instance().getController(controllerIndex);

		map.emplace(&controller.Start,	(uint16)Button::START);
		map.emplace(&controller.Back,	(uint16)Button::MODE);
		map.emplace(&controller.Up,		(uint16)Button::UP);
		map.emplace(&controller.Down,	(uint16)Button::DOWN);
		map.emplace(&controller.Left,	(uint16)Button::LEFT);
		map.emplace(&controller.Right,	(uint16)Button::RIGHT);
		map.emplace(&controller.A,		(uint16)Button::A);
		map.emplace(&controller.B,	 	(uint16)Button::B);
		map.emplace(&controller.X,	 	(uint16)Button::C);
		map.emplace(&controller.Y,	 	(uint16)Button::Y);
		map.emplace(&controller.L,	 	(uint16)Button::X);
		map.emplace(&controller.R,	 	(uint16)Button::Z);
	}
}

void ControlsIn::beginInputUpdate()
{
	// Backup input
	for (int padIndex = 0; padIndex < NUM_GAMEPADS; ++padIndex)
	{
		mGamepad[padIndex].mPreviousInput = mGamepad[padIndex].mCurrentInput;
		mGamepad[padIndex].mInputWasInjected = false;
	}
}

void ControlsIn::endInputUpdate()
{
	for (int padIndex = 0; padIndex < NUM_GAMEPADS; ++padIndex)
	{
		if (!mGamepad[padIndex].mInputWasInjected)
		{
			// Update from actual local controllers
			mGamepad[padIndex].mCurrentInput = getInputFromController(padIndex);
		}

		// Remove all flags from our list of ignored inputs that are currently not pressed
		mGamepad[padIndex].mIgnoreInput &= mGamepad[padIndex].mCurrentInput;

		// Remove all flags from actual inputs that are still ignored
		mGamepad[padIndex].mCurrentInput &= ~mGamepad[padIndex].mIgnoreInput;
	}
}

uint16 ControlsIn::getInputFromController(uint32 padIndex) const
{
	uint16 inputFlags = 0;

	const uint32 controllerIndex = mGamepadsSwitched ? (padIndex ^ 1) : padIndex;	// Swap gamepads 0 and 1, as well as 2 and 3
	for (const auto& pair : inputFlagsLookup[controllerIndex])
	{
		if (pair.first->isPressed())
		{
			inputFlags |= pair.second;
		}
	}

	// In mirror mode, exchange left and right
	if (Configuration::instance().mMirrorMode)
	{
		inputFlags = (inputFlags & 0xfff3) | ((inputFlags & (uint16)Button::LEFT) << 1) | ((inputFlags & (uint16)Button::RIGHT) >> 1);
	}

	return inputFlags;
}

void ControlsIn::injectInput(uint32 padIndex, uint16 inputFlags)
{
	RMX_ASSERT(padIndex < (uint32)NUM_GAMEPADS, "Invalid pad index " << padIndex);

	mGamepad[padIndex].mPreviousInput = mGamepad[padIndex].mCurrentInput;
	mGamepad[padIndex].mCurrentInput = inputFlags;
	mGamepad[padIndex].mInputWasInjected = true;
}

void ControlsIn::setIgnores(uint16 bitmask)
{
	for (int padIndex = 0; padIndex < NUM_GAMEPADS; ++padIndex)
	{
		mGamepad[padIndex].mIgnoreInput = bitmask;
	}
}

void ControlsIn::setAllIgnores()
{
	setIgnores(0x0fff);
}

ControlsIn::Gamepad& ControlsIn::getGamepad(size_t index)
{
	if (index < NUM_GAMEPADS)
	{
		return mGamepad[index];
	}
	else
	{
		static Gamepad defaultGamepad;
		return defaultGamepad;
	}
}

const ControlsIn::Gamepad& ControlsIn::getGamepad(size_t index) const
{
	return const_cast<ControlsIn*>(this)->getGamepad(index);
}

bool ControlsIn::switchGamepads()
{
	mGamepadsSwitched = !mGamepadsSwitched;
	return mGamepadsSwitched;
}
