/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/UpdateCheck.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/version.inc"

#include "oxygen_netcore/network/NetConnection.h"


namespace
{
	const char* getPlatformName()
	{
	#if defined(PLATFORM_WINDOWS)
		return "windows";
	#elif defined(PLATFORM_LINUX)
		return "linux";
	#elif defined(PLATFROM_MAC)
		return "mac";
	#elif defined(PLATFORM_ANDROID)
		return "android";
	#elif defined(PLATFROM_WEB)
		return "web";
	#elif defined(PLATFROM_SWITCH)
		return "switch";
	#else
		return "unknown";
	#endif
	}

	const char* getReleaseChannelNameForBuild()
	{
		const constexpr uint32 buildVariantHash = rmx::compileTimeFNV_32(BUILD_VARIANT);
		if (buildVariantHash == rmx::compileTimeFNV_32("TEST"))
			return "test";
		else if (buildVariantHash == rmx::compileTimeFNV_32("PREVIEW") || buildVariantHash == rmx::compileTimeFNV_32("BETA"))	// Treat betas as previews
			return "preview";
		else
			return "stable";
	}
}


bool UpdateCheck::hasUpdate() const
{
	if (mState != State::HAS_RESPONSE)
		return false;

	return mAppUpdateCheckRequest.mResponse.mHasUpdate;
}

const network::AppUpdateCheckRequest::Response* UpdateCheck::getResponse() const
{
	if (!hasUpdate())
		return nullptr;

	return &mAppUpdateCheckRequest.mResponse;
}

void UpdateCheck::startUpdateCheck()
{
	if (mState != State::READY_TO_START)
		return;

	// No update check if the last update check in the last 60 seconds
	if (mLastUpdateCheckTimestamp != 0 && mGameClient.getCurrentTimestamp() < mLastUpdateCheckTimestamp + 60 * 1000)
		return;

	mState = State::SEND_QUERY;
}

void UpdateCheck::performUpdate()
{
	if (mState == State::INACTIVE)
		return;

	switch (mState)
	{
		case State::SEND_QUERY:
		{
			mAppUpdateCheckRequest.mQuery.mAppName = "sonic3air";
			mAppUpdateCheckRequest.mQuery.mPlatform = ::getPlatformName();
			mAppUpdateCheckRequest.mQuery.mReleaseChannel = ::getReleaseChannelNameForBuild();	// TODO: Save selection in config and make it configurable in Options
			mAppUpdateCheckRequest.mQuery.mInstalledAppVersion = BUILD_NUMBER;
			mAppUpdateCheckRequest.mQuery.mInstalledContentVersion = BUILD_NUMBER;
			mGameClient.getServerConnection().sendRequest(mAppUpdateCheckRequest);

			mLastUpdateCheckTimestamp = mGameClient.getCurrentTimestamp();
			mState = State::WAITING_FOR_RESPONSE;
			break;
		}

		case State::WAITING_FOR_RESPONSE:
		{
			if (mAppUpdateCheckRequest.hasResponse())
			{
				if (mAppUpdateCheckRequest.hasError())
				{
					// TODO: Retry again later?
					mState = State::FAILED;
				}
				else
				{
					mState = State::HAS_RESPONSE;
				}
			}
			break;
		}

		default:
			break;
	}
}

void UpdateCheck::evaluateServerFeaturesResponse(const network::GetServerFeaturesRequest& request)
{
	bool supportsUpdate = false;
	for (const network::GetServerFeaturesRequest::Response::Feature& feature : request.mResponse.mFeatures)
	{
		if (feature.mIdentifier == "app-update-check" && feature.mVersions.contains(1))
		{
			supportsUpdate = true;
		}
	}

	if (supportsUpdate)
	{
		if (mState == State::INACTIVE)
			mState = State::READY_TO_START;
	}
	else
	{
		mState = State::INACTIVE;
	}
}
