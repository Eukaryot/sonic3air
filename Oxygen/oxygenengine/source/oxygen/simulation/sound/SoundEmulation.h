/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/sound/SoundChipWrite.h"


struct blip_t;

class SoundEmulation
{
public:
	SoundEmulation();
	~SoundEmulation();

	bool init(int samplerate, double framerate);
	void reset();
	void shutdown();
	int update(int16* outBuffer, const std::vector<SoundChipWrite>& inputData);

private:
	int internalUpdate(uint32 cycles, const std::vector<SoundChipWrite>& inputData);
	void fmUpdate(uint32 cycles);

private:
	struct Internal;
	Internal& mInternal;

	blip_t* blips[2];  /* Blip Buffer resampling */

	/* FM output buffer (large enough to hold a whole frame at original chips rate) */
	int fm_buffer[1080 * 2];
	int fm_last[2];
	int* fm_ptr = nullptr;

	/* Cycle-accurate FM samples */
	uint32 fm_cycles_ratio = 0;
	uint32 fm_cycles_start = 0;
	uint32 fm_cycles_count = 0;
};

