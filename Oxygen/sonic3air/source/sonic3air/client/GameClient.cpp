/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/client/GameClient.h"
#include "sonic3air/ConfigurationImpl.h"


GameClient::GameClient() :
	mGhostSync(*this),
	mUpdateCheck(*this)
{
}

GameClient::~GameClient()
{
}

void GameClient::setupClient()
{
	EngineServerClient& engineServerClient = EngineServerClient::instance();

	engineServerClient.setListener(this);
	engineServerClient.setupClient();
}

void GameClient::updateClient(float timeElapsed)
{
	EngineServerClient& engineServerClient = EngineServerClient::instance();

	engineServerClient.updateClient(timeElapsed);

	if (!mEvaluatedServerFeatures)
	{
		// Check if server features were read
		if (engineServerClient.hasReceivedServerFeatures())
		{
			mGhostSync.evaluateServerFeaturesResponse(engineServerClient.getServerFeatures());
			mUpdateCheck.evaluateServerFeaturesResponse(engineServerClient.getServerFeatures());
			mEvaluatedServerFeatures = true;
		}
	}

	// Regular update for the sub-systems
	mGhostSync.performUpdate();
	mUpdateCheck.performUpdate();
}

bool GameClient::onReceivedPacket(ReceivedPacketEvaluation& evaluation)
{
	if (mGhostSync.onReceivedPacket(evaluation))
		return true;
	return false;
}
