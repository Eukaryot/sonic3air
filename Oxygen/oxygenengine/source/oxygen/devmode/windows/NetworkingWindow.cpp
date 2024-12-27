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

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/network/EngineServerClient.h"
#include "oxygen/network/netplay/NetplayClient.h"
#include "oxygen/network/netplay/NetplayManager.h"
#include "oxygen/network/netplay/NetplayHost.h"


NetworkingWindow::NetworkingWindow() :
	DevModeWindowBase("Networking", Category::MISC, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void NetworkingWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(250.0f, 10.0f), ImGuiCond_FirstUseEver);

	// Engine server
	{
		ImGui::SeparatorText("Server");
		ImGuiHelpers::ScopedIndent si;

		EngineServerClient& engineServerClient = EngineServerClient::instance();

		switch (engineServerClient.getServerConnection().getState())
		{
			case NetConnection::State::EMPTY:					ImGui::Text("Inactive");  break;
			case NetConnection::State::TCP_READY:				ImGui::Text("TCP ready");  break;
			case NetConnection::State::REQUESTED_CONNECTION:	ImGui::Text("Connecting...");  break;
			case NetConnection::State::ACCEPTED:				ImGui::Text("Connection accepted");  break;
			case NetConnection::State::CONNECTED:				ImGui::Text("Connected");  break;
			case NetConnection::State::DISCONNECTED:			ImGui::Text("Disconnected");  break;
		}

		if (engineServerClient.getServerConnection().getState() == NetConnection::State::CONNECTED)
		{
			ImGui::SameLine();
			ImGui::Text("|  Server IP = %s, port = %d", engineServerClient.getServerConnection().getRemoteAddress().getIP().c_str(), engineServerClient.getServerConnection().getRemoteAddress().getPort());

			if (ImGui::Button("Disconnect"))
			{
				engineServerClient.disconnectFromServer();
			}
		}
		else
		{
			if (ImGui::Button("Start connection"))
			{
				engineServerClient.connectToServer();
			}
		}
	}

	// Netplay
	{
		ImGui::SeparatorText("Netplay");
		ImGuiHelpers::ScopedIndent si;

		NetplayManager& netplayManager = NetplayManager::instance();
		NetplayHost* netplayHost = netplayManager.getNetplayHost();
		NetplayClient* netplayClient = netplayManager.getNetplayClient();

		if (nullptr != netplayHost)
		{
			ImGui::Text("Host: %d connections", (int)netplayHost->getPlayerConnections().size());
		}
		else if (nullptr != netplayClient)
		{
			switch (netplayClient->getHostConnection().getState())
			{
				case NetConnection::State::EMPTY:				 ImGui::Text("Client: No connection");  break;
				case NetConnection::State::TCP_READY:			 ImGui::Text("Client: TCP ready");  break;
				case NetConnection::State::REQUESTED_CONNECTION: ImGui::Text("Client: Requested connection");  break;
				case NetConnection::State::ACCEPTED:			 ImGui::Text("Client: Connection accepted");  break;
				case NetConnection::State::CONNECTED:			 ImGui::Text("Client: Connected");  break;
				case NetConnection::State::DISCONNECTED:		 ImGui::Text("Client: Disconnected");  break;
			}
		}
		else
		{
			ImGui::Text("Inactive");
		}
		
		ImGui::BeginDisabled(nullptr != netplayHost);
		{
			if (ImGui::Button("Host via server"))
			{
				netplayManager.setupAsHost(true, NetplayManager::DEFAULT_PORT);
			}

			ImGui::SameLine();
			if (ImGui::Button("Host directly"))
			{
				netplayManager.setupAsHost(false, NetplayManager::DEFAULT_PORT);
			}
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(nullptr != netplayClient);
		{
			static char ipBuffer[32] = { 0 };
			static int port = NetplayManager::DEFAULT_PORT;
			if (ipBuffer[0] == 0)
				strcpy_s(ipBuffer, 32, netplayManager.isUsingIPv6() ? "::1" : "127.0.0.1");

			if (ImGui::Button("Join via server"))
			{
				netplayManager.startJoinViaServer();
			}

			ImGui::SameLine();
			if (ImGui::Button("Join directly"))
			{
				netplayManager.startJoinDirect(ipBuffer, port);
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

		ImGui::BeginDisabled(nullptr == netplayHost && nullptr == netplayClient);
		if (ImGui::Button("Close connection"))
		{
			netplayManager.closeConnections();
		}
		ImGui::EndDisabled();

		if (!netplayManager.getExternalAddressQuery().mOwnExternalIP.empty())
		{
			ImGui::Text("Retrieved own external address: IP = %s, port = %d", netplayManager.getExternalAddressQuery().mOwnExternalIP.c_str(), netplayManager.getExternalAddressQuery().mOwnExternalPort);
		}

		if (nullptr != netplayHost)
		{
			const NetplayHost::HostState state = netplayHost->getHostState();
			switch (state)
			{
				case NetplayHost::HostState::NONE:					ImGui::Text("Inactive");  break;
				case NetplayHost::HostState::CONNECT_TO_SERVER:		ImGui::Text("Connecting to game server");  break;
				case NetplayHost::HostState::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case NetplayHost::HostState::GAME_RUNNING:			ImGui::Text("Game running");  break;
				case NetplayHost::HostState::FAILED:				ImGui::Text("Failed");  break;
			}

			ImGui::BeginDisabled(state < NetplayHost::HostState::REGISTERED || state > NetplayHost::HostState::GAME_RUNNING);
			if (state == NetplayHost::HostState::REGISTERED)
			{
				if (ImGui::Button("Start game"))
				{
					netplayHost->startGame();
				}
			}
			else
			{
				if (ImGui::Button("Stop game"))
				{
					// TODO: This is not implemented yet
					//netplayHost->stopGame();
				}

				if (state == NetplayHost::HostState::GAME_RUNNING)
				{
					int frameNumber = 0;
					const uint32 checksum = netplayHost->getRegularInputChecksum(frameNumber);
					ImGui::Text("Input checksum:  %08x for frame #%d", checksum, frameNumber);
				}
			}
			ImGui::EndDisabled();
		}

		if (nullptr != netplayClient)
		{
			const NetplayClient::State state = netplayClient->getState();
			switch (state)
			{
				case NetplayClient::State::NONE:				ImGui::Text("Inactive");  break;
				case NetplayClient::State::CONNECT_TO_SERVER:	ImGui::Text("Connecting to game server");  break;
				case NetplayClient::State::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case NetplayClient::State::CONNECT_TO_HOST:		ImGui::Text("Connecting to host");  break;
				case NetplayClient::State::CONNECTED:			ImGui::Text("Connected to host");  break;
				case NetplayClient::State::GAME_RUNNING:		ImGui::Text("Game running");  break;
				case NetplayClient::State::FAILED:				ImGui::Text("Failed");  break;
			}

			if (state == NetplayClient::State::GAME_RUNNING)
			{
				int frameNumber = 0;
				const uint32 checksum = netplayClient->getRegularInputChecksum(frameNumber);
				ImGui::Text("Input checksum:  %08x for frame #%d", checksum, frameNumber);
			}
		}
	}
}

#endif
