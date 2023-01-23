/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class CustomAudioMixer final : public rmx::AudioMixer
{
public:
	CustomAudioMixer(int mixerId) : AudioMixer(mixerId) {}

	void setUnderwaterEffect(int effect, float volumeMultiplier);

protected:
	void performAudioMix(const MixerParameters& parameters) override;

private:
	static inline const constexpr size_t MAX_NUM_CHANNELS = 2;
	static inline const constexpr size_t OUTPUT_BUFFER_SIZE = 1024;
	static inline const constexpr size_t ACCUMULATION_BUFFER_SIZE = 128;

	int mUnderwaterEffect = 0;
	float mVolumeMultiplier = 1.0f;

	struct ChannelData
	{
		int32 mOutputBuffer[OUTPUT_BUFFER_SIZE] = { 0 };
		int32 mHistoryBuffer[ACCUMULATION_BUFFER_SIZE] = { 0 };	// Cyclic buffer
		size_t mIndexInHistory = 0;								// Index of next value in history buffer
		int64 mAccumulator = 0;
	};
	ChannelData mChannelData[MAX_NUM_CHANNELS];
};
