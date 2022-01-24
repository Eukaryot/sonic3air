/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
	std::map<const InputManager::Control*, uint16> inputFlagsLookup[2];
}


ControlsIn::ControlsIn()
{
	mInputPad[0] = mInputPad[1] = 0;
	mPrevInputPad[0] = mPrevInputPad[1] = 0;

	for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex)
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

		mIgnoreInput[controllerIndex] = 0;
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
	for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex)
	{
		const uint32 padIndex = mGamepadsSwitched ? (1 - controllerIndex) : controllerIndex;
		mPrevInputPad[padIndex] = mInputPad[padIndex];

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
		mIgnoreInput[padIndex] &= inputFlags;

		// Remove all inputs from actual output that are still ignored
		inputFlags &= ~mIgnoreInput[padIndex];

		// Assign to emulator input flags
		mInputPad[padIndex] = inputFlags;
	}
}

void ControlsIn::setIgnores(uint16 bitmask)
{
	for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex)
	{
		mIgnoreInput[controllerIndex] = bitmask;
	}
}

void ControlsIn::setAllIgnores()
{
	setIgnores(0x0fff);
}

void ControlsIn::injectInput(uint32 padIndex, uint16 inputFlags)
{
	RMX_CHECK(padIndex == 0 || padIndex == 1, "Invalid controller index", return);

	mPrevInputPad[padIndex] = mInputPad[padIndex];
	mInputPad[padIndex] = inputFlags;
}

bool ControlsIn::switchGamepads()
{
	mGamepadsSwitched = !mGamepadsSwitched;
	return mGamepadsSwitched;
}
