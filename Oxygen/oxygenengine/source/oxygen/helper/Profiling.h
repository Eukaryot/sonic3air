/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/helper/HighResolutionTimer.h"


// Profiling region IDs used by Oxygen Engine
//  -> Not using enum class here, so that we don't need explicit casts
namespace ProfilingRegion
{
	enum Id
	{
		SIMULATION,
		SIMULATION_USER_CALL,
		AUDIO,
		RENDERING,
		FRAMESYNC
	};
}


class Profiling
{
public:
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_VITA)
	static const constexpr size_t MAX_FRAMES = 90;
#else
	static const constexpr size_t MAX_FRAMES = 240;
#endif

	struct Region
	{
	friend class Profiling;

	public:
		struct Frame
		{
			double mInclusiveTime = 0.0;
			double mExclusiveTime = 0.0;
		};

	public:
		std::string mName;
		Color mColor;

		AccumulativeTimer mTimer;
		std::deque<Frame> mFrameTimes;
		double mAverageTime = 0.0;

		Region* mParent = nullptr;
		std::vector<Region*> mChildren;

	private:
		double mAccumulatedTime = 0.0;
		bool mOnStack = false;
	};

	struct PerFrameData
	{
		int mSimulationFrameNumber = 0;
		int mNumSimulationFrames = 0;
	};
	struct AdditionalData
	{
		std::deque<PerFrameData> mFrames;
		float mAverageSimulationsPerSecond = 0.0f;
		float mSmoothedSimulationsPerSecond = 0.0f;
		int mAccumulatedSimulationFrames = 0;
		std::deque<float> mSimulationsPerSecondDeque;
	};

public:
	static void startup();
	static void registerRegion(uint16 id, const char* name, const Color& color);

	static void pushRegion(uint16 id);
	static void popRegion(uint16 id);
	static void nextFrame(int simulationFrameNumber);

	static Region& getRootRegion();
	static void listRegionsRecursive(std::vector<std::pair<Region*,int>>& outRegions);

	static AdditionalData& getAdditionalData();

private:
	static void listRegionsRecursiveInternal(std::vector<std::pair<Region*,int>>& outRegions, Region& parent, int level);
};
