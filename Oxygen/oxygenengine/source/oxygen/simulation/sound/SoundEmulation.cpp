/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/sound/SoundEmulation.h"
#include "oxygen/simulation/sound/blip_buf.h"
#include "oxygen/simulation/sound/sn76489.h"
#include "oxygen/simulation/sound/ym2612.h"
#include "oxygen/helper/FileHelper.h"


struct SoundEmulation::Internal
{
	soundemulation::SN76489 mSN76489;
	soundemulation::YM2612  mYM2612;
};

static const constexpr uint32 MCYCLES_PER_FRAME = 3420 * 262;


SoundEmulation::SoundEmulation() :
	mInternal(*new Internal())
{
}

SoundEmulation::~SoundEmulation()
{
	delete &mInternal;
	shutdown();
}

bool SoundEmulation::init(int samplerate, double framerate)
{
	// Initialize blip buffers
	blips[0] = blip_new(samplerate / 10);
	blips[1] = blip_new(samplerate / 10);

	// Initialize FM chip (YM2612)
	mInternal.mYM2612.init();
	mInternal.mYM2612.config(14);
	fm_cycles_ratio = 144 * 7;		// Chip is running a VCLK / 144 = MCLK / 7 / 144

	// Initialize PSG chip
	mInternal.mSN76489.init(blips);
	mInternal.mSN76489.config(0, 150, 1, 0xff);

	reset();

	// Initialize resampler internal rates
	const double mclk = (double)MCYCLES_PER_FRAME * framerate;
	blip_set_rates(blips[0], mclk, samplerate);
	blip_set_rates(blips[1], mclk, samplerate);

	return true;
}

void SoundEmulation::reset()
{
	// Reset sound chips
	mInternal.mYM2612.resetChip();
	mInternal.mSN76489.reset();
	mInternal.mSN76489.config(0, 150, 1, 0xff);

	// Reset FM buffer ouput
	fm_last[0] = fm_last[1] = 0;

	// Reset FM buffer pointer
	fm_ptr = fm_buffer;

	// Reset FM cycle counters
	fm_cycles_start = fm_cycles_count = 0;

	// Clear blip buffers
	for (int j = 0; j < 2; ++j)
	{
		blip_clear(blips[j]);
	}
}

void SoundEmulation::shutdown()
{
	// Delete blip buffers
	for (int j = 0; j < 2; ++j)
	{
		blip_delete(blips[j]);
		blips[j] = nullptr;
	}
}

int SoundEmulation::update(int16* outBuffer, const std::vector<SoundChipWrite>& inputData)
{
	// Run sound chips until end of frame
	const int size = internalUpdate(MCYCLES_PER_FRAME, inputData);

	// Resample FM & PSG mixed stream to output buffer
	blip_read_samples(blips[0], outBuffer, size);
	blip_read_samples(blips[1], outBuffer + 1, size);

	return size;
}

int SoundEmulation::internalUpdate(uint32 cycles, const std::vector<SoundChipWrite>& inputData)
{
	// Inject chip writes calculated by the sound driver
	for (size_t i = 0; i < inputData.size(); ++i)
	{
		const SoundChipWrite& write = inputData[i];
		if (write.mTarget == SoundChipWrite::Target::NONE)
			break;

		if (write.mTarget == SoundChipWrite::Target::SN76489)
		{
			mInternal.mSN76489.write(write.mCycles, write.mData);
		}
		else
		{
			fmUpdate(write.mCycles);
			const uint32 addressPort = (write.mTarget == SoundChipWrite::Target::YAMAHA_FMII) ? 2 : 0;
			mInternal.mYM2612.write(addressPort, write.mAddress);	// Address port write
			mInternal.mYM2612.write(1, write.mData);				// Data port write
		}
	}

	// Run PSG & FM chips until end of frame
	mInternal.mSN76489.update(cycles);
	fmUpdate(cycles);

	// FM output pre-amplification
	const int preamp = 100;

	// FM frame initial timestamp
	uint32 time = fm_cycles_start;

	// Restore last FM outputs from previous frame
	int l = fm_last[0];
	int r = fm_last[1];

	// FM buffer start pointer
	int* ptr = fm_buffer;

	// Flush FM samples
#if 1
	{
		// High-quality band-limited synthesis
		do
		{
			// Left channel
			int delta = ((*ptr++ * preamp) / 100) - l;
			l += delta;
			blip_add_delta(blips[0], time, delta);

			// Right channel
			delta = ((*ptr++ * preamp) / 100) - r;
			r += delta;
			blip_add_delta(blips[1], time, delta);

			// Increment time counter
			time += fm_cycles_ratio;
		}
		while (time < cycles);
	}
#else
	{
		// Faster linear interpolation
		do
		{
			// Left channel
			int delta = ((*ptr++ * preamp) / 100) - l;
			l += delta;
			blip_add_delta_fast(blips[0], time, delta);

			// Right channel
			delta = ((*ptr++ * preamp) / 100) - r;
			r += delta;
			blip_add_delta_fast(blips[1], time, delta);

			// Increment time counter
			time += fm_cycles_ratio;
		}
		while (time < cycles);
	}
#endif

	// Reset FM buffer pointer
	fm_ptr = fm_buffer;

	// Save last FM output for next frame
	fm_last[0] = l;
	fm_last[1] = r;

	// Adjust FM cycle counters for next frame
	fm_cycles_count = fm_cycles_start = time - cycles;

	// End of blip buffers time frame
	blip_end_frame(blips[0], cycles);
	blip_end_frame(blips[1], cycles);

	// Return number of available samples
	return blip_samples_avail(blips[0]);
}

void SoundEmulation::fmUpdate(uint32 cycles)
{
	/* Run FM chip until required M-cycles */
	if (cycles > fm_cycles_count)
	{
		// Number of samples to run
		uint32 samples = (cycles - fm_cycles_count + fm_cycles_ratio - 1) / fm_cycles_ratio;
		const uint32 limit = 1080 - (uint32)(fm_ptr - fm_buffer) / 2;
		if (samples > limit)
		{
		#ifdef DEBUG
			RMX_ERROR("Too many samples, would cause buffer overflow", );
		#endif
			samples = limit;
		}

		// Run FM chip to sample buffer
		mInternal.mYM2612.update(fm_ptr, samples);

		// Update FM buffer pointer
		fm_ptr += (samples * 2);

		// Update FM cycle counter
		fm_cycles_count += samples * fm_cycles_ratio;
	}
}
