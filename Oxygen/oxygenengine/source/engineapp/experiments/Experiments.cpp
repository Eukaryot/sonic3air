#include "engineapp/pch.h"
#include "engineapp/experiments/Experiments.h"

#ifdef USE_EXPERIMENTS

#include <lemon/program/FunctionWrapper.h>
#include <lemon/program/Module.h>


void Experiments::registerScriptBindings(lemon::Module& module)
{
	module.addUserDefinedFunction("SoundDriver.writeYM", lemon::wrap(*this, &Experiments::writeYM));
	module.addUserDefinedFunction("SoundDriver.writePSG", lemon::wrap(*this, &Experiments::writePSG));
}

void Experiments::onPreFrameUpdate()
{
	if (nullptr == mCurrentFrame || !mCurrentFrame->mAudioData.empty())
	{
		mAudioFrames.emplace_back();
		mCurrentFrame = &mAudioFrames.back();
	}
}

void Experiments::onPostFrameUpdate()
{
	if (!mInitialized)
	{
		mAudioBuffer.clear(44100, 2);
		mAudioBuffer.setPersistent(false);

		mEmulationSound.audio_init(44100, 60.0);

		mInitialized = true;
	}

	if (!mCurrentFrame->mAudioData.empty())
	{
		mDebugOutput << "\r\n";
		mDebugOutput << "#" << (mAudioFrames.size() - 1) << "\r\n";

		for (AudioData& audioData : mCurrentFrame->mAudioData)
		{
			if (!audioData.mIsPSG)
			{
				mDebugOutput << "YM: " << rmx::hexString(audioData.mPort * 0x100 + audioData.mReg, 4) << ", " << rmx::hexString(audioData.mData, 2) << "\r\n";
			}
		}

		for (AudioData& audioData : mCurrentFrame->mAudioData)
		{
			if (audioData.mIsPSG)
			{
				mDebugOutput << "PSG: " << rmx::hexString(audioData.mData, 2) << "\r\n";
			}
		}
	}


	static int16 soundBuffer[0x10000];

	std::vector<uint32> writes;
	for (AudioData& audioData : mCurrentFrame->mAudioData)
	{
		if (audioData.mIsPSG)
		{
			writes.push_back(((audioData.mCycles / 75) << 17) + 0x80000000 + audioData.mData);
		}
		else
		{
			writes.push_back(((audioData.mCycles / 75) << 17) + ((uint32)audioData.mPort << 16) + ((uint32)audioData.mReg << 8) + audioData.mData);
		}
	}

	const uint32 length = mEmulationSound.audio_update(soundBuffer, writes);	// Returns length in samples

	// Check if sound chips still produce output
	bool isPlaying = false;
	for (uint32 i = 0; i < length * 2; ++i)
	{
		if (soundBuffer[i] < -2 || soundBuffer[i] > 0)	// Sometimes we get -2 indefinitely (e.g. sound ID "CC" does this)
		{
			isPlaying = true;
			break;
		}
	}

	//if (isPlaying)
	{
		static int16 pcm[2][0x10000];
		int16* pcmPtr[2] = { pcm[0], pcm[1] };

		for (uint32 i = 0; i < length; ++i)
		{
			pcm[0][i] = soundBuffer[i * 2];
			pcm[1][i] = soundBuffer[i * 2 + 1];
		}
		mAudioBuffer.addData(pcmPtr, length);
	}

	if (!mAudioRef.valid())
	{
		FTX::Audio->addSound(&mAudioBuffer, mAudioRef, true);
	}
}

void Experiments::writeYM(uint32 cycles, uint8 port, uint8 reg, uint8 data)
{
	if (nullptr != mCurrentFrame)
	{
		AudioData& audioData = vectorAdd(mCurrentFrame->mAudioData);
		audioData.mIsPSG = false;
		audioData.mCycles = cycles;
		audioData.mPort = port;
		audioData.mReg = reg;
		audioData.mData = data;
	}
}

void Experiments::writePSG(uint32 cycles, uint8 data)
{
	if (nullptr != mCurrentFrame)
	{
		AudioData& audioData = vectorAdd(mCurrentFrame->mAudioData);
		audioData.mIsPSG = true;
		audioData.mCycles = cycles;
		audioData.mData = data;
	}
}

#endif
