/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/subsystems/UpdateCheck.h"
#include "oxygenserver/server/ServerNetConnection.h"

#include "oxygen_netcore/network/LagStopwatch.h"
#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/serverclient/Packets.h"


UpdateCheck::UpdateCheck()
{
	// Stable version
	{
		UpdateDefinition& definition = vectorAdd(mUpdateDefinitions);
		definition.mVersionNumber = 0x22091000;
		definition.mReleaseChannel = ReleaseChannel::STABLE;
		definition.addPlatform(Platform::WINDOWS);
		definition.addPlatform(Platform::MAC);
		definition.addPlatform(Platform::LINUX);
		definition.addPlatform(Platform::ANDROID);
		definition.addPlatform(Platform::WEB);
		definition.mUpdateURL = "https://sonic3air.org";
	}

	// Stable version (Switch)
	{
		UpdateDefinition& definition = vectorAdd(mUpdateDefinitions);
		definition.mVersionNumber = 0x21091200;
		definition.mReleaseChannel = ReleaseChannel::STABLE;
		definition.addPlatform(Platform::SWITCH);
		definition.mUpdateURL = "https://sonic3air.org";
	}

	// Preview version
	{
		UpdateDefinition& definition = vectorAdd(mUpdateDefinitions);
		definition.mVersionNumber = 0x22082700;
		definition.mReleaseChannel = ReleaseChannel::PREVIEW;
		definition.addPlatform(Platform::WINDOWS);
		definition.addPlatform(Platform::MAC);
		definition.addPlatform(Platform::LINUX);
		definition.addPlatform(Platform::ANDROID);
		definition.addPlatform(Platform::WEB);
		definition.mUpdateURL = "https://sonic3air.org";
	}

	// Test version (Windows only)
	{
		UpdateDefinition& definition = vectorAdd(mUpdateDefinitions);
		definition.mVersionNumber = 0x23011500;
		definition.mReleaseChannel = ReleaseChannel::TEST;
		definition.addPlatform(Platform::WINDOWS);
		definition.mUpdateURL = "https://github.com/Eukaryot/sonic3air/releases";
	}
}

UpdateCheck::Platform UpdateCheck::getPlatformFromString(const std::string& platformString)
{
	if (platformString == "windows")	return Platform::WINDOWS;
	if (platformString == "linux")		return Platform::LINUX;
	if (platformString == "mac")		return Platform::MAC;
	if (platformString == "android")	return Platform::ANDROID;
	if (platformString == "ios")		return Platform::IOS;
	if (platformString == "web")		return Platform::WEB;
	if (platformString == "switch")		return Platform::SWITCH;
	return Platform::UNKNOWN;
}

UpdateCheck::ReleaseChannel UpdateCheck::getReleaseChannelFromString(const std::string& releaseChannelString)
{
	if (releaseChannelString == "stable")	return ReleaseChannel::STABLE;
	if (releaseChannelString == "preview")	return ReleaseChannel::PREVIEW;
	if (releaseChannelString == "test")		return ReleaseChannel::TEST;
	return ReleaseChannel::UNKNOWN;
}

bool UpdateCheck::onReceivedRequestQuery(ReceivedQueryEvaluation& evaluation)
{
	LAG_STOPWATCH("UpdateCheck::onReceivedRequestQuery", 1000);

	switch (evaluation.mPacketType)
	{
		case network::AppUpdateCheckRequest::Query::PACKET_TYPE:
		{
			using Request = network::AppUpdateCheckRequest;
			Request request;
			if (!evaluation.readQuery(request))
				return false;

			ServerNetConnection& connection = static_cast<ServerNetConnection&>(evaluation.mConnection);
			RMX_LOG_INFO("AppUpdateCheckRequest: " << request.mQuery.mAppName << ", " << request.mQuery.mPlatform << ", " << request.mQuery.mReleaseChannel << ", " << rmx::hexString(request.mQuery.mInstalledAppVersion, 8) << " (from " << connection.getHexPlayerID() << ")");

			request.mResponse.mHasUpdate = false;
			if (request.mQuery.mAppName == "sonic3air")
			{
				uint32 latestVersion = request.mQuery.mInstalledAppVersion;
				const UpdateDefinition* bestDefinition = nullptr;

				const Platform platform = getPlatformFromString(request.mQuery.mPlatform);
				const uint64 platformFlag = ((uint64)1 << (int)platform);
				const ReleaseChannel releaseChannel = getReleaseChannelFromString(request.mQuery.mReleaseChannel);

				for (const UpdateDefinition& definition : mUpdateDefinitions)
				{
					if ((definition.mVersionNumber > latestVersion) &&
						(definition.mReleaseChannel <= releaseChannel) &&
						(definition.mPlatforms & platformFlag) != 0)
					{
						latestVersion = definition.mVersionNumber;
						bestDefinition = &definition;
					}
				}

				if (nullptr != bestDefinition)
				{
					request.mResponse.mHasUpdate = true;
					request.mResponse.mAvailableAppVersion = latestVersion;
					request.mResponse.mAvailableContentVersion = latestVersion;
					request.mResponse.mUpdateInfoURL = bestDefinition->mUpdateURL;
				}
			}
			return evaluation.respond(request);
		}
	}

	return false;
}
