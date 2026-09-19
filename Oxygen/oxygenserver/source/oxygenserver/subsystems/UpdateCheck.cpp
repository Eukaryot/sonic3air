/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/subsystems/UpdateCheck.h"
#include "oxygenserver/server/ServerNetConnection.h"

#include "oxygen_netcore/network/LagStopwatch.h"
#include "oxygen_netcore/serverclient/Packets.h"


UpdateCheck::UpdateCheck()
{
	mSourceJsonFileName = L"update_definitions.json";
	loadSourceJsonFile();
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

bool UpdateCheck::loadSourceJsonFile()
{
	Json::Value root = rmx::JsonHelper::loadFile(mSourceJsonFileName);
	if (!root.isObject() || !root["Updates"].isArray())
		return false;

	// Load definitions
	std::vector<UpdateDefinition> updateDefinitions;
	for (const Json::Value& jsonDefinition : root["Updates"])
	{
		if (!jsonDefinition.isObject())
			continue;

		const Json::Value& version = jsonDefinition["Version"];
		const Json::Value& releaseChannel = jsonDefinition["ReleaseChannel"];
		const Json::Value& platforms = jsonDefinition["Platforms"];
		const Json::Value& updateURL = jsonDefinition["UpdateURL"];

		if (!version.isString() || !releaseChannel.isString() || !platforms.isArray() || !updateURL.isString())
			continue;

		UpdateDefinition newDefinition;

		// Version number
		newDefinition.mVersionNumber = (uint32)rmx::parseInteger(version.asString());

		// Release channel
		newDefinition.mReleaseChannel = getReleaseChannelFromString(releaseChannel.asString());
		if (newDefinition.mReleaseChannel == ReleaseChannel::UNKNOWN)
			continue;

		// Platforms
		for (const Json::Value& jsonPlatform : platforms)
		{
			if (jsonPlatform.isString())
			{
				const Platform platform = getPlatformFromString(jsonPlatform.asString());
				if (platform != Platform::UNKNOWN)
					newDefinition.addPlatform(platform);
			}
		}

		// At least one valid platform is required
		if (newDefinition.mPlatforms == 0)
			continue;

		newDefinition.mUpdateURL = updateURL.asString();

		updateDefinitions.push_back(std::move(newDefinition));
	}

	if (updateDefinitions.empty())
		return false;

	// Only apply if the read definitions were valid and there was no error
	mUpdateDefinitions = std::move(updateDefinitions);

	mSourceJsonFileTime = FTX::FileSystem->getFileTime(mSourceJsonFileName);
	return true;
}

void UpdateCheck::checkSourceJsonFileChanges()
{
	// Check for changes only every 10 seconds
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	const uint64 milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastSourceJsonCheckTime).count();
	if (milliseconds > 10000)
	{
		const time_t fileTime = FTX::FileSystem->getFileTime(mSourceJsonFileName);
		if (fileTime != mSourceJsonFileTime)
		{
			loadSourceJsonFile();
		}

		mLastSourceJsonCheckTime = now;
	}
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

			checkSourceJsonFileChanges();

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
