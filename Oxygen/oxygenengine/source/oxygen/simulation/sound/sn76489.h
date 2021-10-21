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

#pragma once

struct blip_t;

namespace soundemulation
{
	class SN76489
	{
	public:
		void init(blip_t** blips);
		void reset();
		void config(unsigned int clocks, int preAmp, int boostNoise, int stereo);
		void write(unsigned int clocks, unsigned int data);
		void update(unsigned int cycles);

	private:
		void updateToneAmplitude(int i, int time);
		void updateNoiseAmplitude(int time);
		void runTone(int i, int clocks);
		void runNoise(int clocks);
		void runUntil(unsigned int clocks);

	private:
		blip_t* blips[2];

		/* Configuration */
		int mPreAmp[4][2];       /* stereo channels pre-amplification ratio (%) */
		int mNoiseFeedback;
		int mSRWidth;

		/* PSG registers: */
		int mRegisters[8];       /* Tone, vol x4 */
		int mLatchedRegister;
		int mNoiseShiftRegister;
		int mNoiseFreq;          /* Noise channel signal generator frequency */

		/* Output calculation variables */
		int mToneFreqVals[4];    /* Frequency register values (counters) */
		int mToneFreqPos[4];     /* Frequency channel flip-flops */
		int mChannel[4][2];      /* current amplitude of each (stereo) channel */
		int mChanOut[4][2];      /* current output value of each (stereo) channel */

		/* Internal M-clock counter */
		unsigned long mClocks;
	};
}
