/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


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

public:
	ControlsIn();

	void startup();
	void shutdown();
	void update(bool readControllers);

	void setIgnores(uint16 bitmask);
	void setAllIgnores();

	inline uint16 getInputPad(size_t index) const	   { return mInputPad[index]; }
	inline uint16 getPrevInputPad(size_t index) const  { return mPrevInputPad[index]; }

	void injectInput(uint32 padIndex, uint16 inputFlags);

	inline bool areGamepadsSwitched() const  { return mGamepadsSwitched; }
	bool switchGamepads();

private:
	uint16 mInputPad[2];
	uint16 mPrevInputPad[2];
	uint16 mIgnoreInput[2];
	bool mGamepadsSwitched = false;
};
