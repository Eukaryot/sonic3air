/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <chrono>


class HighResolutionTimer
{
public:
	void reset();
	void start();

	inline bool isRunning() const  { return mRunning; }
	double getSecondsSinceStart() const;

protected:
	typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
	typedef std::chrono::duration<double> Duration;

	TimePoint mStart;
	bool mRunning = false;
};


class AccumulativeTimer : protected HighResolutionTimer
{
public:
	void resetTiming();
	void resumeTiming();
	void pauseTiming();

	double getAccumulatedSeconds() const;
	double getAccumulatedSecondsAndRestart();

private:
	Duration mAccumulatedTime;
};
