/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/sound/SoundChipWrite.h"


class Internal;

class SoundDriver
{
public:
	// M-Cycles per frame: 262 lines with 3420 cycles each (NTSC console)
	static const constexpr uint32 MCYCLES_PER_FRAME = 3420 * 262;

	enum class UpdateResult
	{
		CONTINUE,	// Not finished yet
		FINISHED,	// Finished, but sound chips may still produce output
		STOP		// Enforce stop of sound incl. sound chips (used for 1-up jingle)
	};

public:
	SoundDriver();
	~SoundDriver();

	void setFixedContent(const uint8* data, uint32 size, uint32 offset);
	void setSourceAddress(uint32 sourceAddress);

	void reset();
	void playSound(uint8 sfxId);
	void setTempoSpeedup(uint8 tempoSpeedup);

	UpdateResult update();
	const std::vector<SoundChipWrite>& getSoundChipWrites() const;

private:
	Internal& mInternal;
};
