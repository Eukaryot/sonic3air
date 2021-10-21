/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	This file is a derivative work for use in Oxygen Engine.
*	It's based on the Genesis Plus GX source code, original license see below.
*/


/***************************************************************************************
*  Genesis Plus
*  PSG sound chip (SN76489A compatible)
*
*  Support for discrete chip & integrated (ASIC) clones
*
*  Noise implementation based on http://www.smspower.org/Development/SN76489#NoiseChannel
*
*  Copyright (C) 2016-2017 Eke-Eke (Genesis Plus GX)
*
*  Redistribution and use of this code or any derivative works are permitted
*  provided that the following conditions are met:
*
*   - Redistributions may not be sold, nor may they be used in a commercial
*     product or activity.
*
*   - Redistributions that are modified from the original source must include the
*     complete source code, including the source code for all components used by a
*     binary built from the modified sources. However, as a special exception, the
*     source code distributed need not include anything that is normally distributed
*     (in either source or binary form) with the major components (compiler, kernel,
*     and so on) of the operating system on which the executable runs, unless that
*     component itself accompanies the executable.
*
*   - Redistributions must reproduce the above copyright notice, this list of
*     conditions and the following disclaimer in the documentation and/or other
*     materials provided with the distribution.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
****************************************************************************************/

#include "oxygen/pch.h"
#include "oxygen/simulation/sound/sn76489.h"
#include "oxygen/simulation/sound/blip_buf.h"


namespace soundemulation
{

	static constexpr int PSG_MCYCLES_RATIO = 16 * 15;

	/* Initial state of shift register */
	static constexpr int NoiseInitialState = 0x8000;

	/* Value below which PSG does not output  */
	static constexpr int PSG_CUTOFF = 1;

	static const uint16 PSGVolumeValues[16] =
	{
		/* These values are taken from a real SMS2's output */
		/*{892,892,892,760,623,497,404,323,257,198,159,123,96,75,60,0}, */
		/* I can't remember why 892... :P some scaling I did at some point */
		/* these values are true volumes for 2dB drops at each step (multiply previous by 10^-0.1) */
		1516, 1205, 957, 760, 603, 479, 381, 303, 240, 191, 152, 120, 96, 76, 60, 0
	};


	void SN76489::init(blip_t** blips_)
	{
		blips[0] = blips_[0];
		blips[1] = blips_[1];

		for (int i = 0; i < 4; i++)
		{
			mPreAmp[i][0] = 100;
			mPreAmp[i][1] = 100;
		}

		mNoiseFeedback = 9;
		mSRWidth = 16;
	}

	void SN76489::reset()
	{
		for (int i = 0; i <= 3; i++)
		{
			/* Initialise PSG state */
			mRegisters[2 * i] = 1; /* tone freq=1 */
			mRegisters[2 * i + 1] = 0xf; /* vol=off */

			/* Set counters to 0 */
			mToneFreqVals[i] = 0;

			/* Set flip-flops to 1 */
			mToneFreqPos[i] = 1;

			/* Clear stereo channels amplitude */
			mChannel[i][0] = 0;
			mChannel[i][1] = 0;

			/* Clear stereo channel outputs in delta buffer */
			mChanOut[i][0] = 0;
			mChanOut[i][1] = 0;
		}

		/* Initialise latched register index */
		mLatchedRegister = 0;

		/* Initialise noise generator */
		mNoiseShiftRegister = NoiseInitialState;
		mNoiseFreq = 0x10;

		/* Reset internal M-cycle counter */
		mClocks = 0;
	}

	/* Updates tone amplitude in delta buffer. Call whenever amplitude might have changed. */
	void SN76489::updateToneAmplitude(int i, int time)
	{
		/* left output */
		int delta = (mChannel[i][0] * mToneFreqPos[i]) - mChanOut[i][0];
		if (delta != 0)
		{
			mChanOut[i][0] += delta;
			blip_add_delta_fast(blips[0], time, delta);
		}

		/* right output */
		delta = (mChannel[i][1] * mToneFreqPos[i]) - mChanOut[i][1];
		if (delta != 0)
		{
			mChanOut[i][1] += delta;
			blip_add_delta_fast(blips[1], time, delta);
		}
	}

	/* Updates noise amplitude in delta buffer. Call whenever amplitude might have changed. */
	void SN76489::updateNoiseAmplitude(int time)
	{
		/* left output */
		int delta = (mChannel[3][0] * (mNoiseShiftRegister & 0x1)) - mChanOut[3][0];
		if (delta != 0)
		{
			mChanOut[3][0] += delta;
			blip_add_delta_fast(blips[0], time, delta);
		}

		/* right output */
		delta = (mChannel[3][1] * (mNoiseShiftRegister & 0x1)) - mChanOut[3][1];
		if (delta != 0)
		{
			mChanOut[3][1] += delta;
			blip_add_delta_fast(blips[1], time, delta);
		}
	}

	/* Runs tone channel for clock_length clocks */
	void SN76489::runTone(int i, int clocks)
	{
		/* Update in case a register changed etc. */
		updateToneAmplitude(i, mClocks);

		/* Time of next transition */
		int time = mToneFreqVals[i];

		/* Process any transitions that occur within clocks we're running */
		while (time < clocks)
		{
			if (mRegisters[i * 2] > PSG_CUTOFF)
			{
				/* Flip the flip-flop */
				mToneFreqPos[i] = -mToneFreqPos[i];
			}
			else
			{
				/* stuck value */
				mToneFreqPos[i] = 1;
			}
			updateToneAmplitude(i, time);

			/* Advance to time of next transition */
			time += mRegisters[i * 2] * PSG_MCYCLES_RATIO;
		}

		/* Update channel tone counter */
		mToneFreqVals[i] = time;
	}

	/* Runs noise channel for clock_length clocks */
	void SN76489::runNoise(int clocks)
	{
		/* Noise channel: match to tone2 if in slave mode */
		int NoiseFreq = mNoiseFreq;
		if (NoiseFreq == 0x80)
		{
			NoiseFreq = mRegisters[2 * 2];
			mToneFreqVals[3] = mToneFreqVals[2];
		}

		/* Update in case a register changed etc. */
		updateNoiseAmplitude(mClocks);

		/* Time of next transition */
		int time = mToneFreqVals[3];

		/* Process any transitions that occur within clocks we're running */
		while (time < clocks)
		{
			/* Flip the flip-flop */
			mToneFreqPos[3] = -mToneFreqPos[3];
			if (mToneFreqPos[3] == 1)
			{
				/* On the positive edge of the square wave (only once per cycle) */
				int Feedback = mNoiseShiftRegister;
				if (mRegisters[6] & 0x4)
				{
					/* White noise */
					/* Calculate parity of fed-back bits for feedback */
					/* Do some optimised calculations for common (known) feedback values */
					/* If two bits fed back, I can do Feedback=(nsr & fb) && (nsr & fb ^ fb) */
					/* since that's (one or more bits set) && (not all bits set) */
					Feedback = ((Feedback & mNoiseFeedback) && ((Feedback & mNoiseFeedback) ^ mNoiseFeedback));
				}
				else    /* Periodic noise */
					Feedback = Feedback & 1;

				mNoiseShiftRegister = (mNoiseShiftRegister >> 1) | (Feedback << (mSRWidth - 1));
				updateNoiseAmplitude(time);
			}

			/* Advance to time of next transition */
			time += NoiseFreq * PSG_MCYCLES_RATIO;
		}

		/* Update channel tone counter */
		mToneFreqVals[3] = time;
	}

	void SN76489::runUntil(unsigned int clocks)
	{
		/* Run noise first, since it might use current value of third tone frequency counter */
		runNoise(clocks);

		/* Run tone channels */
		for (int i = 0; i < 3; ++i)
		{
			runTone(i, clocks);
		}
	}

	void SN76489::config(unsigned int clocks, int preAmp, int boostNoise, int stereo)
	{
		/* cycle-accurate Game Gear stereo */
		if (clocks > mClocks)
		{
			/* Run chip until current timestamp */
			runUntil(clocks);

			/* Update internal M-cycle counter */
			mClocks += ((clocks - mClocks + PSG_MCYCLES_RATIO - 1) / PSG_MCYCLES_RATIO) * PSG_MCYCLES_RATIO;
		}

		for (int i = 0; i < 4; i++)
		{
			/* stereo channel pre-amplification */
			mPreAmp[i][0] = preAmp * ((stereo >> (i + 4)) & 1);
			mPreAmp[i][1] = preAmp * ((stereo >> (i + 0)) & 1);

			/* noise channel boost */
			if (i == 3)
			{
				mPreAmp[3][0] = mPreAmp[3][0] << boostNoise;
				mPreAmp[3][1] = mPreAmp[3][1] << boostNoise;
			}

			/* update stereo channel amplitude */
			mChannel[i][0] = (PSGVolumeValues[mRegisters[i * 2 + 1]] * mPreAmp[i][0]) / 100;
			mChannel[i][1] = (PSGVolumeValues[mRegisters[i * 2 + 1]] * mPreAmp[i][1]) / 100;
		}
	}

	void SN76489::update(unsigned int clocks)
	{
		if (clocks > mClocks)
		{
			/* Run chip until current timestamp */
			runUntil(clocks);

			/* Update internal M-cycle counter */
			mClocks += ((clocks - mClocks + PSG_MCYCLES_RATIO - 1) / PSG_MCYCLES_RATIO) * PSG_MCYCLES_RATIO;
		}

		/* Adjust internal M-cycle counter for next frame */
		mClocks -= clocks;

		/* Adjust channel time counters for new frame */
		for (int i = 0; i<4; ++i)
		{
			mToneFreqVals[i] -= clocks;
		}
	}

	void SN76489::write(unsigned int clocks, unsigned int data)
	{
		if (clocks > mClocks)
		{
			/* run chip until current timestamp */
			runUntil(clocks);

			/* update internal M-cycle counter */
			mClocks += ((clocks - mClocks + PSG_MCYCLES_RATIO - 1) / PSG_MCYCLES_RATIO) * PSG_MCYCLES_RATIO;
		}

		unsigned int index;
		if (data & 0x80)
		{
			/* latch byte  %1 cc t dddd */
			mLatchedRegister = index = (data >> 4) & 0x07;
		}
		else
		{
			/* restore latched register index */
			index = mLatchedRegister;
		}

		switch (index)
		{
			case 0:
			case 2:
			case 4: /* Tone Channels frequency */
			{
				if (data & 0x80)
				{
					/* Data byte  %1 cc t dddd */
					mRegisters[index] = (mRegisters[index] & 0x3f0) | (data & 0xf);
				}
				else
				{
					/* Data byte  %0 - dddddd */
					mRegisters[index] = (mRegisters[index] & 0x00f) | ((data & 0x3f) << 4);
				}

				/* zero frequency behaves the same as a value of 1 */
				if (mRegisters[index] == 0)
				{
					mRegisters[index] = 1;
				}
				break;
			}

			case 1:
			case 3:
			case 5: /* Tone Channels attenuation */
			{
				data &= 0x0f;
				mRegisters[index] = data;
				data = PSGVolumeValues[data];
				index >>= 1;
				mChannel[index][0] = (data * mPreAmp[index][0]) / 100;
				mChannel[index][1] = (data * mPreAmp[index][1]) / 100;
				break;
			}

			case 6: /* Noise control */
			{
				mRegisters[6] = data & 0x0f;

				/* reset shift register */
				mNoiseShiftRegister = NoiseInitialState;

				/* set noise signal generator frequency */
				mNoiseFreq = 0x10 << (data & 0x3);
				break;
			}

			case 7: /* Noise attenuation */
			{
				data &= 0x0f;
				mRegisters[7] = data;
				data = PSGVolumeValues[data];
				mChannel[3][0] = (data * mPreAmp[3][0]) / 100;
				mChannel[3][1] = (data * mPreAmp[3][1]) / 100;
				break;
			}
		}
	}

}
