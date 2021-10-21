/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <chrono>


struct HighResolutionTimer
{
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
	typedef std::chrono::duration<double> Duration;

	TimePoint mStart;
	Duration mCurrentTime = Duration::zero();
	Duration mLastTime = Duration::zero();
	bool mRunning = false;

	void Reset();
	void Restart();
	void Start();
	void Stop();
	double GetCurrentSeconds();
	double GetLastSeconds() const;
};
