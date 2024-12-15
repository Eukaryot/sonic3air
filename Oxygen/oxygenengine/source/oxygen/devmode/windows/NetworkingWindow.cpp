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
	//ImGui::SetWindowSize(ImVec2(400.0f, 150.0f), ImGuiCond_FirstUseEver);

	// Gameplay connection
	{
		ImGui::SeparatorText("Gameplay Connector");
		ImGuiHelpers::ScopedIndent si;

		NetplayManager& netplayManager = NetplayManager::instance();

		if (nullptr != netplayManager.getNetplayHost())
		{
			ImGui::Text("Host: %d connections", netplayManager.getNetplayHost()->getPlayerConnections().size());
		}
		else if (nullptr != netplayManager.getNetplayClient())
		{
			switch (netplayManager.getNetplayClient()->getHostConnection().getState())
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
		
		ImGui::BeginDisabled(nullptr != netplayManager.getNetplayHost());
		{
			const bool USE_IPV6 = false;	// TODO: IPv6 does not work, the the server doesn't support it
			if (ImGui::Button("Host via server"))
			{
				netplayManager.setupAsHost(true, NetplayManager::DEFAULT_PORT, USE_IPV6);
			}

			ImGui::SameLine();
			if (ImGui::Button("Host directly"))
			{
				netplayManager.setupAsHost(false, NetplayManager::DEFAULT_PORT, USE_IPV6);
			}
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(nullptr != netplayManager.getNetplayClient());
		{
			static char ipBuffer[32] = "127.0.0.1";
			static int port = NetplayManager::DEFAULT_PORT;

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

		ImGui::BeginDisabled(nullptr == netplayManager.getNetplayHost() && nullptr == netplayManager.getNetplayClient());
		if (ImGui::Button("Close connection"))
		{
			netplayManager.closeConnections();
		}
		ImGui::EndDisabled();

		if (!netplayManager.getExternalAddressQuery().mOwnExternalIP.empty())
		{
			ImGui::Text("Retrieved own external address: IP = %s, port = %d", netplayManager.getExternalAddressQuery().mOwnExternalIP.c_str(), netplayManager.getExternalAddressQuery().mOwnExternalPort);
		}

		if (nullptr != netplayManager.getNetplayHost())
		{
			switch (netplayManager.getNetplayHost()->getState())
			{
				case NetplayHost::State::NONE:					ImGui::Text("Inactive");  break;
				case NetplayHost::State::CONNECT_TO_SERVER:		ImGui::Text("Connecting to game server");  break;
				case NetplayHost::State::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case NetplayHost::State::PUNCHTHROUGH:			ImGui::Text("NAT punchthrough");  break;
				case NetplayHost::State::RUNNING:				ImGui::Text("Session running");  break;
				case NetplayHost::State::FAILED:				ImGui::Text("Failed");  break;
			}
		}

		if (nullptr != netplayManager.getNetplayClient())
		{
			switch (netplayManager.getNetplayClient()->getState())
			{
				case NetplayClient::State::NONE:				ImGui::Text("Inactive");  break;
				case NetplayClient::State::CONNECT_TO_SERVER:	ImGui::Text("Connecting to game server");  break;
				case NetplayClient::State::REGISTERED:			ImGui::Text("Registered at game server");  break;
				case NetplayClient::State::CONNECT_TO_HOST:		ImGui::Text("Connecting to host");  break;
				case NetplayClient::State::RUNNING:				ImGui::Text("Session running");  break;
				case NetplayClient::State::FAILED:				ImGui::Text("Failed");  break;
			}
		}
	}
}

#endif
