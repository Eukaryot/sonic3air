/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
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

void ControlsIn::startup()
{
}

void ControlsIn::shutdown()
{
}

void ControlsIn::update(bool readControllers)
{
	if (!readControllers)
		return;

	const bool switchLeftRight = Configuration::instance().mMirrorMode;

	// Update controllers
	for (int controllerIndex = 0; controllerIndex < NUM_GAMEPADS; ++controllerIndex)
	{
		const uint32 padIndex = (mGamepadsSwitched && controllerIndex < 2) ? (1 - controllerIndex) : controllerIndex;
		mGamepad[padIndex].mPreviousInput = mGamepad[padIndex].mCurrentInput;

		// Calculate new input flags
		uint16 inputFlags = 0;
		for (const auto& pair : inputFlagsLookup[controllerIndex])
		{
			if (pair.first->isPressed())
			{
				inputFlags |= pair.second;
			}
		}
		if (switchLeftRight)
		{
			inputFlags = (inputFlags & 0xfff3) | ((inputFlags & (uint16)Button::LEFT) << 1) | ((inputFlags & (uint16)Button::RIGHT) >> 1);
		}

		// Remove all inputs from our list of ignored input that are currently not pressed
		mGamepad[padIndex].mIgnoreInput &= inputFlags;

		// Remove all inputs from actual output that are still ignored
		inputFlags &= ~mGamepad[padIndex].mIgnoreInput;

		// Assign to emulator input flags
		mGamepad[padIndex].mCurrentInput = inputFlags;
	}
}

void ControlsIn::setIgnores(uint16 bitmask)
{
	for (int controllerIndex = 0; controllerIndex < NUM_GAMEPADS; ++controllerIndex)
	{
		mGamepad[controllerIndex].mIgnoreInput = bitmask;
	}
}

void ControlsIn::setAllIgnores()
{
	setIgnores(0x0fff);
}

const ControlsIn::Gamepad& ControlsIn::getGamepad(size_t index) const
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

void ControlsIn::injectInput(uint32 padIndex, uint16 inputFlags)
{
	RMX_CHECK(padIndex < (uint32)NUM_GAMEPADS, "Invalid controller index", return);

	mGamepad[padIndex].mPreviousInput = mGamepad[padIndex].mCurrentInput;
	mGamepad[padIndex].mCurrentInput = inputFlags;
}

bool ControlsIn::switchGamepads()
{
	mGamepadsSwitched = !mGamepadsSwitched;
	return mGamepadsSwitched;
}
