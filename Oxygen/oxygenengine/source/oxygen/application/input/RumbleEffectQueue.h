/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
	bool addEffect(float lowFrequencyRumble, float highFrequencyRumble, uint32 endTicks);
	bool removeExpiredEffects(uint32 currentTicks);

	inline float RumbleEffectQueue::getCurrentLowFreqIntensity() const	{ return getCurrentIntensity(mLowFreqEffects); }
	inline float RumbleEffectQueue::getCurrentHighFreqIntensity() const	{ return getCurrentIntensity(mHighFreqEffects); }

private:
	float getCurrentIntensity(const std::map<uint32, float>& effects) const;

private:
	std::map<uint32, float> mLowFreqEffects;	// Key is a timestamp in milliseconds (see "SDL_GetTicks()") when this effect ends, value is the rumble intensity
	std::map<uint32, float> mHighFreqEffects;	// Same here
};
