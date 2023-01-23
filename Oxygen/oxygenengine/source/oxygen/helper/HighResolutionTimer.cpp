/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/HighResolutionTimer.h"


void HighResolutionTimer::reset()
{
	mRunning = false;
}

void HighResolutionTimer::start()
{
	mStart = std::chrono::high_resolution_clock::now();
	mRunning = true;
}

double HighResolutionTimer::getSecondsSinceStart() const
{
	if (mRunning)
	{
		const Duration duration = (std::chrono::high_resolution_clock::now() - mStart);
		return duration.count();
	}
	return 0.0;
}


void AccumulativeTimer::resetTiming()
{
	reset();
	mAccumulatedTime = Duration::zero();
}

void AccumulativeTimer::resumeTiming()
{
	if (!mRunning)
	{
		start();
	}
}

void AccumulativeTimer::pauseTiming()
{
	if (mRunning)
	{
		mAccumulatedTime += (std::chrono::high_resolution_clock::now() - mStart);
		mRunning = false;
	}
}

double AccumulativeTimer::getAccumulatedSeconds() const
{
	Duration totalDuration = mAccumulatedTime;
	if (mRunning)
	{
		totalDuration += (std::chrono::high_resolution_clock::now() - mStart);
	}
	return totalDuration.count();
}

double AccumulativeTimer::getAccumulatedSecondsAndRestart()
{
	const TimePoint now = std::chrono::high_resolution_clock::now();
	Duration totalDuration = mAccumulatedTime;
	if (mRunning)
	{
		totalDuration += (now - mStart);
	}

	mStart = now;
	mAccumulatedTime = Duration::zero();
	mRunning = true;
	return totalDuration.count();
}
