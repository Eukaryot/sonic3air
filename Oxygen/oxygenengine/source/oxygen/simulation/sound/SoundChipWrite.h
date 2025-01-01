/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


struct SoundChipWrite
{
	enum class Target
	{
		NONE,
		YAMAHA_FMI,
		YAMAHA_FMII,
		SN76489
	};

	Target mTarget = Target::NONE;
	uint8 mAddress = 0;
	uint8 mData = 0;
	uint32 mCycles = 0;
	uint16 mLocation = 0;		// Only used in verification
	uint16 mFrameNumber = 0;	// Only used in verification

	inline bool operator==(const SoundChipWrite& other) const { return (mTarget == other.mTarget) && (mAddress == other.mAddress) && (mData == other.mData); }
//	inline bool operator==(const SoundChipWrite& other) const { return (mTarget == other.mTarget) && (mAddress == other.mAddress) && (mData == other.mData) && (mCycles == other.mCycles); }
};
