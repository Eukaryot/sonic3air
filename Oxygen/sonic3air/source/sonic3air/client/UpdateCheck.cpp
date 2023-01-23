/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
	#elif defined(PLATFORM_MAC)
		return "mac";
	#elif defined(PLATFORM_ANDROID)
		return "android";
	#elif defined(PLATFORM_IOS)
		return "ios";
	#elif defined(PLATFORM_WEB)
		return "web";
	#elif defined(PLATFORM_SWITCH)
		return "switch";
	#else
		return "unknown";
	#endif
	}

	const char* getReleaseChannelName()
	{
		const char* RELEASE_CHANNEL_NAMES[3] = { "stable", "preview", "test" };
		int& releaseChannel = ConfigurationImpl::instance().mGameServer.mUpdateCheck.mReleaseChannel;
		releaseChannel = clamp(releaseChannel, 0, 2);
		return RELEASE_CHANNEL_NAMES[releaseChannel];
	}
}


void UpdateCheck::reset()
{
	if (mState > State::READY_TO_START && mState != State::FAILED)
		mState = State::READY_TO_START;
	mUpdateRequested = false;
	mAppUpdateCheckRequest = network::AppUpdateCheckRequest();
	mLastUpdateCheckTimestamp = 0;
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
	if (!mGameClient.isConnected())
	{
		// Start connecting, the rest is done later in "performUpdate"
		mGameClient.connectToServer();
		mState = State::CONNECTING;
	}
	mUpdateRequested = true;
}

void UpdateCheck::performUpdate()
{
	if (!mGameClient.isConnected())
	{
		// TODO: Start a connection if needed
		return;
	}

	switch (mState)
	{
		case State::INACTIVE:
			return;

		case State::CONNECTING:
		{
			// Wait for "evaluateServerFeaturesResponse" to be called, and only check for errors here
			if (mGameClient.getConnectionState() == GameClient::ConnectionState::FAILED)
			{
				mState = State::FAILED;
			}
			break;
		}

		case State::READY_TO_START:
		{
			if (!mUpdateRequested)
				break;

			mUpdateRequested = false;

			// Don't start a new update check if the last one was in the last 20 seconds (for the same update channel)
			if (mLastUpdateCheckTimestamp != 0 && mGameClient.getCurrentTimestamp() < mLastUpdateCheckTimestamp + 20 * 1000)
				break;

			mState = State::SEND_QUERY;
			[[fallthrough]];
		}

		case State::SEND_QUERY:
		{
			mAppUpdateCheckRequest.mQuery.mAppName = "sonic3air";
			mAppUpdateCheckRequest.mQuery.mPlatform = ::getPlatformName();
			mAppUpdateCheckRequest.mQuery.mReleaseChannel = ::getReleaseChannelName();
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
		if (mState <= State::CONNECTING)
			mState = State::READY_TO_START;
	}
	else
	{
		mState = State::INACTIVE;
	}
}
