/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/EngineMain.h"
#include "engineapp/version.inc"

#ifdef USE_EXPERIMENTS

#include "oxygen/simulation/sound/sound.h"


class Experiments
{
public:
	void registerScriptBindings(lemon::Module& module);
	void onPreFrameUpdate();
	void onPostFrameUpdate();

private:
	void writeYM(uint32 cycles, uint8 port, uint8 reg, uint8 data);
	void writePSG(uint32 cycles, uint8 data);

private:
	struct AudioData
	{
		bool   mIsPSG = false;
		uint32 mCycles = 0;
		uint8  mPort = 0;
		uint8  mReg = 0;
		uint8  mData = 0;
	};
	struct AudioFrame
	{
		std::vector<AudioData> mAudioData;
	};
	std::vector<AudioFrame> mAudioFrames;
	AudioFrame* mCurrentFrame = nullptr;

	bool mInitialized = false;
	AudioBuffer mAudioBuffer;
	AudioReference mAudioRef;
	soundemulation::Sound mEmulationSound;

	String mDebugOutput;
};

#endif
