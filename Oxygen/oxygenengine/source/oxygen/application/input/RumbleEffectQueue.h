/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class RumbleEffectQueue
{
public:
	void reset();
	bool addEffect(float lowFrequencyRumble, float highFrequencyRumble, uint64 endTicks);
	bool removeExpiredEffects(uint64 currentTicks);

	inline float getCurrentLowFreqIntensity() const		{ return getCurrentIntensity(mLowFreqEffects); }
	inline float getCurrentHighFreqIntensity() const	{ return getCurrentIntensity(mHighFreqEffects); }

private:
	float getCurrentIntensity(const std::map<uint64, float>& effects) const;

private:
	std::map<uint64, float> mLowFreqEffects;	// Key is a timestamp in milliseconds (see "SDL_GetTicks()") when this effect ends, value is the rumble intensity
	std::map<uint64, float> mHighFreqEffects;	// Same here
};
