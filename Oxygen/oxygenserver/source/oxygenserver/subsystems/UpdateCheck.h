/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/ConnectionListener.h"


class UpdateCheck
{
public:
	UpdateCheck();

	bool onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation);

private:
	enum class Platform
	{
		UNKNOWN = 0,
		WINDOWS,
		MAC,
		LINUX,
		ANDROID,
		IOS,
		WEB,
		SWITCH
	};

	enum class ReleaseChannel
	{
		UNKNOWN,
		STABLE,
		PREVIEW,
		TEST
	};

	struct UpdateDefinition
	{
		uint32 mVersionNumber = 0;
		uint64 mPlatforms = 0;										// Platforms for which this definition is valid; it's a bitmask using bit-shifted "Platform" entries
		ReleaseChannel mReleaseChannel = ReleaseChannel::UNKNOWN;	// Release channel for which this definition is valid; including all "higher" relesae channels (stable < preview < test)
		std::string mUpdateURL;

		inline void addPlatform(Platform platform)  { mPlatforms |= ((uint64)1 << (int)platform); }
	};

private:
	static Platform getPlatformFromString(const std::string& platformString);
	static ReleaseChannel getReleaseChannelFromString(const std::string& releaseChannelString);

private:
	std::vector<UpdateDefinition> mUpdateDefinitions;
};
