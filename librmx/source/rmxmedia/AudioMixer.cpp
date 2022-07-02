/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


namespace rmx
{

	namespace
	{
		void mixInSamples(int32* output, const short* input, int numSamples, int sourceIndexStart, int sourceIndexAdvance, int volume, int volumeChange)
		{
			int j = sourceIndexStart;
			if (volumeChange == 0)
			{
				volume >>= 8;
				for (int i = 0; i < numSamples; ++i)
				{
					output[i] += input[j >> 16] * volume;
					j += sourceIndexAdvance;
				}
			}
			else
			{
				if (volume + volumeChange * numSamples < 0)
				{
					numSamples = -volume / volumeChange;
				}
				else if (volume + volumeChange * numSamples > 0x10000)
				{
					numSamples = (0x10000 - volume) / volumeChange;
				}

				for (int i = 0; i < numSamples; ++i)
				{
					output[i] += (input[j >> 16] * volume) >> 8;
					j += sourceIndexAdvance;
					volume += volumeChange;
				}
			}
		}

		void mixInSampleAverages(int32* output, const short* input0, const short* input1, int numSamples, int sourceIndexStart, int sourceIndexAdvance, int volume, int volumeChange)
		{
			int j = sourceIndexStart;
			volume /= 2;
			volumeChange /= 2;
			if (volumeChange == 0)
			{
				volume >>= 8;
				for (int i = 0; i < numSamples; ++i)
				{
					const int k = j >> 16;
					output[i] += (input0[k] + input1[k]) * volume;
					j += sourceIndexAdvance;
				}
			}
			else
			{
				if (volume + volumeChange * numSamples < 0)
				{
					numSamples = -volume / volumeChange;
				}
				else if (volume + volumeChange * numSamples > 0x10000)
				{
					numSamples = (0x10000 - volume) / volumeChange;
				}

				for (int i = 0; i < numSamples; ++i)
				{
					const int k = j >> 16;
					output[i] += ((input0[k] + input1[k]) * volume) >> 8;
					j += sourceIndexAdvance;
					volume += volumeChange;
				}
			}
		}
	}



	AudioMixer::~AudioMixer()
	{
		// Remove from hierarchy: Insert all children into own parent
		//  -> Except if this is the root mixer; but when that one is destroyed, all its child will get destroyed afterwards as well
		if (nullptr != mParent)
		{
			for (AudioMixer* child : mChildren)
			{
				child->mParent = mParent;
				mParent->mChildren.push_back(child);
			}
		}

		// Stop all playing audio instances
		for (const auto& [key, audioInstance] : mAudioInstances)
		{
			audioInstance->mPlaybackDone = true;
			audioInstance->mAudioMixer = nullptr;
		}
	}

	void AudioMixer::addChild(AudioMixer& child)
	{
		if (child.mParent == this)
			return;

		if (nullptr != child.mParent)
		{
			child.mParent->removeChildInternal(child);
		}

		child.mParent = this;
		mChildren.push_back(&child);
	}

	void AudioMixer::clearAudioInstances()
	{
		mAudioInstances.clear();
	}

	void AudioMixer::addAudioInstance(AudioManager::AudioInstance& audioInstance)
	{
		mAudioInstances[audioInstance.mID] = &audioInstance;
	}

	void AudioMixer::removeAudioInstance(AudioManager::AudioInstance& audioInstance)
	{
		mAudioInstances.erase(audioInstance.mID);
	}

	void AudioMixer::performAudioMix(const MixerParameters& parameters)
	{
		updateOutputVolume(parameters);
		mixInAllChildren(parameters);
		mixInAllAudioInstances(parameters);
	}

	void AudioMixer::updateOutputVolume(const MixerParameters& parameters)
	{
		mOutputVolume = parameters.mAccumulatedVolume * mRelativeVolume;
	}

	void AudioMixer::mixInAllChildren(const MixerParameters& parameters)
	{
		if (mChildren.empty())
			return;

		MixerParameters newParameters = parameters;
		newParameters.mAccumulatedVolume = mOutputVolume;
		for (rmx::AudioMixer* audioMixer : mChildren)
		{
			audioMixer->performAudioMix(newParameters);
		}
	}

	void AudioMixer::mixInAllAudioInstances(const MixerParameters& parameters)
	{
		for (const auto& [key, audioInstance] : mAudioInstances)
		{
			mixInAudioInstance(*audioInstance, parameters.mOutputBuffers, parameters.mOutputSamples, *parameters.mOutputFormat);
		}
	}

	void AudioMixer::mixInAudioInstance(AudioManager::AudioInstance& audioInstance, int32*const* outputBuffer, size_t numOutputSamplesNeeded, const SDL_AudioSpec& outputFormat)
	{
		// Mix in audio data into the output stream
		if (audioInstance.mPaused)
			return;

		AudioBuffer& audioBuffer = *audioInstance.mAudioBuffer;

		// Properties
		float playSpeed = audioInstance.mSpeed * (float)audioBuffer.getFrequency() / (float)outputFormat.freq;
		playSpeed = clamp(playSpeed, 0.1f, 10.0f);
		const int sourceIndexAdvance = roundToInt(playSpeed * 0x10000);

		// Offsets where the data starts
		int32* output[2] = { outputBuffer[0], outputBuffer[1] };

		// While still waiting for playback start, don't mix in anything yet
		if (audioInstance.mPosition < 0)
		{
			int instProgress = (int)(numOutputSamplesNeeded * playSpeed);
			if (audioInstance.mPosition + instProgress <= 0)
			{
				audioInstance.mPosition += instProgress;
				return;
			}

			int offset = (int)(-audioInstance.mPosition / playSpeed);
			output[0] += offset;
			output[1] += offset;
			numOutputSamplesNeeded -= offset;
			audioInstance.mPosition = 0;
		}

		// Perform the actual audio mixing
		audioBuffer.lock();
		const bool result = mixAudioBufferInner(audioInstance, output, numOutputSamplesNeeded, outputFormat, sourceIndexAdvance);
		audioBuffer.unlock();

		if (!result)
		{
			audioInstance.mPlaybackDone = true;
		}
	}

	bool AudioMixer::mixAudioBufferInner(AudioManager::AudioInstance& audioInstance, int32** output, size_t numOutputSamplesNeeded, const SDL_AudioSpec& outputFormat, int sourceIndexAdvance)
	{
		AudioBuffer& audioBuffer = *audioInstance.mAudioBuffer;

		// Check if the audio buffer got cleared (that can happen in Oxygen Engine)
		//  -> In that case, just stop the sound immediately
		if (audioInstance.mPosition > audioBuffer.getLength())
			return false;

		const int instanceChannels = audioBuffer.getChannels();
		const float volumeMultiplier = mOutputVolume * 0x10000;

		// Loop to mix in samples in multiple blocks (if necessary)
		int sourceSamplePositionFraction = 0;
		while (numOutputSamplesNeeded > 0)
		{
			RMX_ASSERT(audioInstance.mPosition <= audioBuffer.getLength(), "Audio instance position " << audioInstance.mPosition << " exceeding audio buffer length " << audioBuffer.getLength());
			short* instanceData[2];
			int numAvailableInputSamples = audioBuffer.getData(instanceData, audioInstance.mPosition);
			if (numAvailableInputSamples <= 0)
			{
				// Reached the end of available data

				// Check if streaming is active and not yet complete
				if (audioInstance.mStreaming && !audioBuffer.isCompleted())
				{
					// We don't have any more data yet, sorry
					return true;
				}
				else if (audioInstance.mLoop)
				{
					// Restart looped sound
					audioInstance.mPosition = audioInstance.mLoopStart;
					numAvailableInputSamples = audioBuffer.getData(instanceData, audioInstance.mPosition);
					if (numAvailableInputSamples <= 0)
					{
						return audioInstance.mStreaming;
					}
				}
				else
				{
					// Stop playback
					return false;
				}
			}

			// Number of samples to be mixed in next as a single block
			int numBlockSamples;
			{
				// Consider timeout
				if (audioInstance.mTimeout > 0)
				{
					numAvailableInputSamples = std::min(numAvailableInputSamples, audioInstance.mTimeout);
				}

				// Limit by samples available in instance
				numBlockSamples = (sourceSamplePositionFraction + (numAvailableInputSamples << 16) - 1) / sourceIndexAdvance + 1;
				numBlockSamples = std::min(numBlockSamples, (int)numOutputSamplesNeeded);
			}

			// Determine starting volumes and volume change per sample
			int volume[2];
			int volumeChange[2];
			{
				const float baseVolume = audioInstance.mVolume * volumeMultiplier;
				const float baseVolumeChange = audioInstance.mVolumeChange * volumeMultiplier / (float)outputFormat.freq;

				if (audioInstance.mPanning && (outputFormat.channels == 2))
				{
					volume[0] = roundToInt(baseVolume * (1.0f - audioInstance.mPanning));
					volume[1] = roundToInt(baseVolume * (1.0f + audioInstance.mPanning));
					volumeChange[0] = roundToInt(baseVolumeChange * (1.0f - audioInstance.mPanning));
					volumeChange[1] = roundToInt(baseVolumeChange * (1.0f + audioInstance.mPanning));
				}
				else
				{
					volume[0] = roundToInt(baseVolume);
					volume[1] = roundToInt(baseVolume);
					volumeChange[0] = roundToInt(baseVolumeChange);
					volumeChange[1] = roundToInt(baseVolumeChange);
				}
			}

			// Mix in audio samples
			if (outputFormat.channels == 1)
			{
				// Output as Mono
				if (instanceChannels == 1)
				{
					mixInSamples(output[0], instanceData[0], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[0], volumeChange[0]);
				}
				else
				{
					mixInSampleAverages(output[0], instanceData[0], instanceData[1], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[0], volumeChange[0]);
				}
			}
			else
			{
				// Output as Stereo
				if (instanceChannels == 1)
				{
					mixInSamples(output[0], instanceData[0], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[0], volumeChange[0]);
					mixInSamples(output[1], instanceData[0], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[1], volumeChange[1]);
				}
				else if (audioInstance.mPanning)
				{
					mixInSampleAverages(output[0], instanceData[0], instanceData[1], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[0], volumeChange[0]);
					mixInSampleAverages(output[1], instanceData[0], instanceData[1], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[1], volumeChange[1]);
				}
				else
				{
					mixInSamples(output[0], instanceData[0], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[0], volumeChange[0]);
					mixInSamples(output[1], instanceData[1], numBlockSamples, sourceSamplePositionFraction, sourceIndexAdvance, volume[1], volumeChange[1]);
				}
			}

			output[0] += numBlockSamples;
			output[1] += numBlockSamples;

			// Advance in input
			sourceSamplePositionFraction += sourceIndexAdvance * numBlockSamples;
			audioInstance.mPosition += sourceSamplePositionFraction >> 16;
			sourceSamplePositionFraction &= 0xffff;

			if (audioInstance.mTimeout > 0)
			{
				audioInstance.mTimeout -= numAvailableInputSamples;
				if (audioInstance.mTimeout <= 0)
					return false;
			}

			// Update instance volume
			if (audioInstance.mVolumeChange != 0.0f)
			{
				const float deltaTime = (float)numBlockSamples / (float)outputFormat.freq;	// In seconds
				audioInstance.mVolume += audioInstance.mVolumeChange * deltaTime;
				if (audioInstance.mVolume < 0.0f)
				{
					audioInstance.mVolume = 0.0f;
					audioInstance.mVolumeChange = 0.0f;
					return false;
				}
				if (audioInstance.mVolume >= 1.0f)
				{
					audioInstance.mVolume = 1.0f;
					audioInstance.mVolumeChange = 0.0f;
				}
			}

			numOutputSamplesNeeded -= numBlockSamples;
		}

		// Done
		return true;
	}

	void AudioMixer::removeChildInternal(AudioMixer& child)
	{
		for (size_t i = 0; i < mChildren.size(); ++i)
		{
			if (mChildren[i] == &child)
			{
				mChildren.erase(mChildren.begin() + i);
				child.mParent = nullptr;
				return;
			}
		}
	}

}
