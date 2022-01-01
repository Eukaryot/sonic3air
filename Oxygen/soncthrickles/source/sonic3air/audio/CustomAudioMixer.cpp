/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/audio/CustomAudioMixer.h"


void CustomAudioMixer::setUnderwaterEffect(int effect, float volumeMultiplier)
{
	mVolumeMultiplier = volumeMultiplier;

	const int newEffect = clamp(effect, 0, ACCUMULATION_BUFFER_SIZE);
	if (newEffect == mUnderwaterEffect)
		return;

	if (newEffect <= 0)
	{
		// Effect gets switched off, nothing to do
	}
	else if (mUnderwaterEffect <= 0)
	{
		// Effect gets switched on, start with empty history and accumulator
		for (size_t k = 0; k < MAX_NUM_CHANNELS; ++k)
		{
			mChannelData[k].mAccumulator = 0;
			mChannelData[k].mIndexInHistory = 0;
			memset(mChannelData[k].mHistoryBuffer, 0, sizeof(mChannelData[k].mHistoryBuffer));
		}
	}
	else
	{
		// Effects gets reduced or extended, keep the history and recalculate the accumulators
		for (size_t k = 0; k < MAX_NUM_CHANNELS; ++k)
		{
			ChannelData& data = mChannelData[k];
			data.mAccumulator = 0;
			for (size_t i = 1; i <= (size_t)effect; ++i)
			{
				const size_t lookupIndex = (data.mIndexInHistory - i + ACCUMULATION_BUFFER_SIZE) % ACCUMULATION_BUFFER_SIZE;
				data.mAccumulator += data.mHistoryBuffer[lookupIndex];
			}
		}
	}

	mUnderwaterEffect = newEffect;
}

void CustomAudioMixer::performAudioMix(const MixerParameters& parameters)
{
	// Cache these values here already, as they might be changed by the main thread while this function is executed on the audio thread
	const int effect = mUnderwaterEffect;
	const float volume = mVolumeMultiplier;

	if (effect <= 0)
	{
		rmx::AudioMixer::performAudioMix(parameters);
		return;
	}

	// Clear output buffers
	for (size_t k = 0; k < MAX_NUM_CHANNELS; ++k)
	{
		memset(mChannelData[k].mOutputBuffer, 0, parameters.mOutputSamples * sizeof(int32));
	}

	// Mix everything into output buffers
	MixerParameters newParameters = parameters;
	newParameters.mOutputBuffers[0] = mChannelData[0].mOutputBuffer;
	newParameters.mOutputBuffers[1] = mChannelData[1].mOutputBuffer;
	rmx::AudioMixer::performAudioMix(newParameters);

	const int divisor = roundToInt((float)effect / volume);

	// Now do the post-processing
	for (size_t k = 0; k < MAX_NUM_CHANNELS; ++k)
	{
		ChannelData& data = mChannelData[k];
		int32* output = parameters.mOutputBuffers[k];

		for (size_t i = 0; i < parameters.mOutputSamples; ++i)
		{
			const int32 inputValue = data.mOutputBuffer[i];

			// Get an old input value, namely n values back in the past (where n = effect)
			const size_t lookupIndex = (data.mIndexInHistory - effect + ACCUMULATION_BUFFER_SIZE) % ACCUMULATION_BUFFER_SIZE;
			const int32 historyValue = data.mHistoryBuffer[lookupIndex];

			// Update accumulator
			//  -> It is the sum of the last n input values (where n = effect)
			data.mAccumulator += (int64)(inputValue - historyValue);

			// Get the average and use it as output
			output[i] = (int32)(data.mAccumulator / divisor);

			// Write input value into history and advance to next
			data.mHistoryBuffer[data.mIndexInHistory] = inputValue;
			data.mIndexInHistory = (data.mIndexInHistory + 1) % ACCUMULATION_BUFFER_SIZE;
		}
	}
}
