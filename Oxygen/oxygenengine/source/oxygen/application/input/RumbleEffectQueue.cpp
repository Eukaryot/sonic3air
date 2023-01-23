/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/RumbleEffectQueue.h"


void RumbleEffectQueue::reset()
{
	mLowFreqEffects.clear();
	mHighFreqEffects.clear();
}

bool RumbleEffectQueue::addEffect(float lowFrequencyRumble, float highFrequencyRumble, uint32 endTicks)
{
	bool anyCurrentIntensityChanged = false;
	for (int pass = 0; pass < 2; ++pass)
	{
		// Add to rumble effects queue for that player, at least if it's going to make any difference at all
		const float intensity = (pass == 0) ? lowFrequencyRumble : highFrequencyRumble;
		if (intensity <= 0.0f)
			continue;

		std::map<uint32, float>& effects = (pass == 0) ? mLowFreqEffects : mHighFreqEffects;
		bool shouldBeAdded = true;
		if (effects.empty())
		{
			anyCurrentIntensityChanged = true;
		}
		else
		{
			if (intensity > effects.begin()->second)
			{
				anyCurrentIntensityChanged = true;
			}
			else
			{
				// The entries in the map are sorted by their end timestamp, and are also decreasing in intensities
				for (auto pair : effects)
				{
					// Find the first existing effect that lasts at least as long than our new one
					if (pair.first >= endTicks)
					{
						// The new one would be unnecessary if it does not have a higher intensity than that existing effect
						//  -> All effects in the map afterwards will all have lower intensities, so there's no need to check them as well
						shouldBeAdded = (intensity > pair.second);
						break;
					}
				}
			}
		}

		if (shouldBeAdded)
		{
			// Remove effects that are going to be superseded by the new effect
			for (auto it = effects.begin(); it != effects.end(); )
			{
				if (it->first >= endTicks)
				{
					// No need to check the effects that will last longer anyways
					break;
				}

				if (it->second <= intensity)
				{
					it = effects.erase(it);
				}
				else
				{
					++it;
				}
			}

			// Add our new effect to the map
			effects[endTicks] = intensity;
		}
	}

	return anyCurrentIntensityChanged;
}

bool RumbleEffectQueue::removeExpiredEffects(uint32 currentTicks)
{
	bool anyChange = false;
	for (int pass = 0; pass < 2; ++pass)
	{
		std::map<uint32, float>& effects = (pass == 0) ? mLowFreqEffects : mHighFreqEffects;
		while (!effects.empty() && effects.begin()->first <= currentTicks)
		{
			effects.erase(effects.begin());
			anyChange = true;
		}
	}
	return anyChange;
}

float RumbleEffectQueue::getCurrentIntensity(const std::map<uint32, float>& effects) const
{
	return effects.empty() ? 0.0f : effects.begin()->second;
}
