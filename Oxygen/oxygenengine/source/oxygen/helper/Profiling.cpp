/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Profiling.h"


namespace profiling
{
	Profiling::Region mRootRegion;
	std::map<uint16, Profiling::Region> mRegionsById;		// Does not contain root region
	std::vector<Profiling::Region*> mAllRegions;
	std::vector<Profiling::Region*> mRegionStack;
	Profiling::AdditionalData mAdditionalData;
	int mAccumulatedFrames = 0;

	Profiling::Region* getRegionById(uint16 id)
	{
		const auto it = mRegionsById.find(id);
		return (it == mRegionsById.end()) ? nullptr : &it->second;
	}
}

using namespace profiling;


void Profiling::startup()
{
	// No need to do things twice
	if (!mAllRegions.empty())
		return;

	mRootRegion.mName = "root";
	mAllRegions.push_back(&mRootRegion);
	mRegionStack.push_back(&mRootRegion);
}

void Profiling::registerRegion(uint16 id, const char* name, const Color& color)
{
	RMX_ASSERT(mRegionsById.count(id) == 0, "Profiling region with id " << id << " registered twice, second time with name '" << name << "'");

	Profiling::Region& region = mRegionsById[id];
	region.mName = name;
	region.mColor = color;
	mAllRegions.push_back(&region);
}

void Profiling::pushRegion(uint16 id)
{
	Region* region = getRegionById(id);
	RMX_ASSERT(nullptr != region, "Profiling region with id " << id << " not found");
	RMX_ASSERT(!region->mOnStack, "Profiling region with name '" << region->mName << "' is already on the stack");
	RMX_ASSERT(mRegionStack.size() >= 1, "Profiling region stack got emptied before");

	mRegionStack.push_back(region);
	region->mOnStack = true;
	region->mTimer.Start();

	if (region->mParent == nullptr)
	{
		region->mParent = mRegionStack[mRegionStack.size() - 2];
		region->mParent->mChildren.push_back(region);
	}
	else
	{
		RMX_ASSERT(region->mParent == mRegionStack[mRegionStack.size() - 2], "Profiling region '" << region->mName << "' has different parents on the stack: '" << region->mParent->mName << "' and '" << mRegionStack[mRegionStack.size() - 2] << "'");
	}
}

void Profiling::popRegion(uint16 id)
{
	Region* region = getRegionById(id);
	RMX_ASSERT(nullptr != region, "Profiling region with id " << id << " not found");
	RMX_ASSERT(mRegionStack.size() >= 2, "Can't pop another profiling region from stack, that would remove the root region");
	RMX_ASSERT(mRegionStack.back() == region, "Profiling region to be popped must be top of stack");

	mRegionStack.pop_back();
	region->mOnStack = false;
	region->mTimer.Stop();
}

void Profiling::nextFrame(int simulationFrameNumber)
{
	RMX_ASSERT(mRegionStack.size() == 1, "Profiling region stack must only contain the root on frame end");

	for (Region* region : mAllRegions)
	{
		const double lastTime = region->mTimer.GetCurrentSeconds();		// For root timer (which is usually still running), this will perform a stop and immediate restart
		while (region->mFrameTimes.size() >= MAX_FRAMES)
			region->mFrameTimes.pop_front();
		region->mFrameTimes.emplace_back();
		Region::Frame& frame = region->mFrameTimes.back();
		frame.mInclusiveTime = lastTime;
		frame.mExclusiveTime = lastTime;
		region->mAccumulatedTime += lastTime;
		region->mTimer.Reset();
	}
	for (Region* region : mAllRegions)
	{
		if (nullptr != region->mParent)
		{
			region->mParent->mFrameTimes.back().mExclusiveTime -= region->mFrameTimes.back().mExclusiveTime;
		}
	}
	mRootRegion.mTimer.Start();		// Needed for first frame

	static const PerFrameData dummy;
	const PerFrameData& oldData = mAdditionalData.mFrames.empty() ? dummy : mAdditionalData.mFrames.back();
	while (mAdditionalData.mFrames.size() >= MAX_FRAMES)
		mAdditionalData.mFrames.pop_front();
	mAdditionalData.mFrames.emplace_back();
	PerFrameData& data = mAdditionalData.mFrames.back();
	data.mSimulationFrameNumber = simulationFrameNumber;
	data.mNumSimulationFrames = simulationFrameNumber - oldData.mSimulationFrameNumber;
	mAdditionalData.mAccumulatedSimulationFrames += data.mNumSimulationFrames;

	++mAccumulatedFrames;
	if (mAccumulatedFrames >= 30 || mRootRegion.mAccumulatedTime >= 1.0)
	{
		mAdditionalData.mAverageSimulationsPerSecond = (float)mAdditionalData.mAccumulatedSimulationFrames / (float)mRootRegion.mAccumulatedTime;
		mAdditionalData.mAccumulatedSimulationFrames = 0;

		while (mAdditionalData.mSimulationsPerSecondDeque.size() >= 10)
			mAdditionalData.mSimulationsPerSecondDeque.pop_front();
		mAdditionalData.mSimulationsPerSecondDeque.push_back(mAdditionalData.mAverageSimulationsPerSecond);
		mAdditionalData.mSmoothedSimulationsPerSecond = 0.0f;
		for (float value : mAdditionalData.mSimulationsPerSecondDeque)
			mAdditionalData.mSmoothedSimulationsPerSecond += value;
		mAdditionalData.mSmoothedSimulationsPerSecond /= (float)std::max<size_t>(1, mAdditionalData.mSimulationsPerSecondDeque.size());

		for (Region* region : mAllRegions)
		{
			region->mAverageTime = region->mAccumulatedTime / (float)mAccumulatedFrames;
			region->mAccumulatedTime = 0.0;
		}
		mAccumulatedFrames = 0;
	}
}

Profiling::Region& Profiling::getRootRegion()
{
	return mRootRegion;
}

void Profiling::listRegionsRecursive(std::vector<std::pair<Region*, int>>& outRegions)
{
	outRegions.clear();
	listRegionsRecursiveInternal(outRegions, mRootRegion, 1);
}

Profiling::AdditionalData& Profiling::getAdditionalData()
{
	return mAdditionalData;
}

void Profiling::listRegionsRecursiveInternal(std::vector<std::pair<Region*, int>>& outRegions, Region& parent, int level)
{
	for (Region* child : parent.mChildren)
	{
		outRegions.emplace_back(child, level);
		listRegionsRecursiveInternal(outRegions, *child, level + 1);
	}
}
