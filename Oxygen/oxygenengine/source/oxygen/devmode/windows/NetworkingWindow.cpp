/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/NetworkingWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/gpconnect/GameplayClient.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/gpconnect/GameplayHost.h"
#include "oxygen/devmode/ImGuiHelpers.h"


NetworkingWindow::NetworkingWindow() :
	DevModeWindowBase("Networking", Category::MISC, ImGuiWindowFlags_AlwaysAutoResize)
{
	// Just for testing
	mIsWindowOpen = true;
}

void NetworkingWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(250.0f, 10.0f), ImGuiCond_FirstUseEver);
	//ImGui::SetWindowSize(ImVec2(400.0f, 150.0f), ImGuiCond_FirstUseEver);

	// Gameplay connection
	{
		ImGui::SeparatorText("Gameplay Connector");
		ImGuiHelpers::ScopedIndent si;

		GameplayConnector& gameplayConnector = GameplayConnector::instance();

		if (nullptr != gameplayConnector.getGameplayHost())
		{
			ImGui::Text("Host: %d connections", gameplayConnector.getGameplayHost()->getPlayerConnections().size());
		}
		else if (nullptr != gameplayConnector.getGameplayClient())
		{
			switch (gameplayConnector.getGameplayClient()->getHostConnection().getState())
			{
				case NetConnection::State::EMPTY:				 ImGui::Text("Client: No connection");  break;
				case NetConnection::State::TCP_READY:			 ImGui::Text("Client: TCP ready");  break;
				case NetConnection::State::REQUESTED_CONNECTION: ImGui::Text("Client: Requested connection");  break;
				case NetConnection::State::CONNECTED:			 ImGui::Text("Client: Connected");  break;
				case NetConnection::State::DISCONNECTED:		 ImGui::Text("Client: Disconnected");  break;
			}
		}
		else
		{
			ImGui::Text("Inactive");
		}
		
		ImGui::BeginDisabled(nullptr != gameplayConnector.getGameplayHost());
		{
			const bool USE_IPV6 = false;	// TODO: IPv6 does not work, the the server doesn't support it
			if (ImGui::Button("Host via server"))
			{
				gameplayConnector.setupAsHost(true, GameplayConnector::DEFAULT_PORT, USE_IPV6);
			}

			ImGui::SameLine();
			if (ImGui::Button("Host directly"))
			{
				gameplayConnector.setupAsHost(false, GameplayConnector::DEFAULT_PORT, USE_IPV6);
			}
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(nullptr != gameplayConnector.getGameplayClient());
		{
			static char ipBuffer[32] = "127.0.0.1";
			static int port = GameplayConnector::DEFAULT_PORT;

			if (ImGui::Button("Join via server"))
			{
				gameplayConnector.startJoinViaServer();
			}

			ImGui::SameLine();
			if (ImGui::Button("Join directly"))
			{
				gameplayConnector.startJoinDirect(ipBuffer, port);
			}

			ImGui::SameLine();
			ImGui::Text(" IP:");
			ImGui::SameLine();
			ImGui::PushItemWidth(90);
			ImGui::InputText("##IP", ipBuffer, 32);
			ImGui::PopItemWidth();

			ImGui::SameLine();
			ImGui::Text("Port:");
			ImGui::SameLine();
			ImGui::PushItemWidth(50);
			ImGui::InputInt("##Port", &port, 0, 0, ImGuiInputTextFlags_CharsDecimal);
			ImGui::PopItemWidth();
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(nullptr == gameplayConnector.getGameplayHost() && nullptr == gameplayConnector.getGameplayClient());
		if (ImGui::Button("Close connection"))
		{
			gameplayConnector.closeConnections();
		}
		ImGui::EndDisabled();

		if (!gameplayConnector.getExternalAddressQuery().mOwnExternalIP.empty())
		{
			ImGui::Text("Retrieved own external address: IP = %s, port = %d", gameplayConnector.getExternalAddressQuery().mOwnExternalIP.c_str(), gameplayConnector.getExternalAddressQuery().mOwnExternalPort);
		}

		if (nullptr != gameplayConnector.getGameplayHost())
		{
			switch (gameplayConnector.getGameplayHost()->getState())
			{
				case GameplayHost::State::NONE:					ImGui::Text("Inactive");  break;
				case GameplayHost::State::CONNECT_TO_SERVER:	ImGui::Text("Connecting to game server");  break;
				case GameplayHost::State::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case GameplayHost::State::PUNCHTHROUGH:			ImGui::Text("NAT punchthrough");  break;
				case GameplayHost::State::RUNNING:				ImGui::Text("Session running");  break;
				case GameplayHost::State::FAILED:				ImGui::Text("Failed");  break;
			}
		}

		if (nullptr != gameplayConnector.getGameplayClient())
		{
			switch (gameplayConnector.getGameplayClient()->getState())
			{
				case GameplayClient::State::NONE:				ImGui::Text("Inactive");  break;
				case GameplayClient::State::CONNECT_TO_SERVER:  ImGui::Text("Connecting to game server");  break;
				case GameplayClient::State::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case GameplayClient::State::CONNECT_TO_HOST:	ImGui::Text("Connecting to host");  break;
				case GameplayClient::State::RUNNING:			ImGui::Text("Session running");  break;
				case GameplayClient::State::FAILED:				ImGui::Text("Failed");  break;
			}
		}
	}
}

#endif
