/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/HighResolutionTimer.h"


void HighResolutionTimer::Reset()
{
	mLastTime = mCurrentTime;
	mCurrentTime = Duration::zero();
	mRunning = false;
}

void HighResolutionTimer::Restart()
{
	mLastTime = mCurrentTime;
	mCurrentTime = Duration::zero();
	mStart = std::chrono::high_resolution_clock::now();
	mRunning = true;
}

void HighResolutionTimer::Start()
{
	if (!mRunning)
	{
		mStart = std::chrono::high_resolution_clock::now();
		mRunning = true;
	}
}

void HighResolutionTimer::Stop()
{
	if (mRunning)
	{
		const TimePoint end = std::chrono::high_resolution_clock::now();
		mCurrentTime += (end - mStart);
		mRunning = false;
	}
}

double HighResolutionTimer::GetCurrentSeconds()
{
	if (mRunning)
	{
		const TimePoint end = std::chrono::high_resolution_clock::now();
		mCurrentTime += (end - mStart);
		mStart = end;
	}
	return mCurrentTime.count();
}

double HighResolutionTimer::GetLastSeconds() const
{
	return mLastTime.count();
}
