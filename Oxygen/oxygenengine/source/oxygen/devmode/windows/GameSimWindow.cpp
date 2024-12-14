/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/GameSimWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/Application.h"
#include "oxygen/application/gpconnect/GameplayClient.h"
#include "oxygen/application/gpconnect/GameplayConnector.h"
#include "oxygen/application/gpconnect/GameplayHost.h"
#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/simulation/Simulation.h"


GameSimWindow::GameSimWindow() :
	DevModeWindowBase("Game", Category::GAME_CONTROLS, ImGuiWindowFlags_AlwaysAutoResize)
{
#ifdef DEBUG
	// Just for testing
	mIsWindowOpen = true;
#endif
}

void GameSimWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(250.0f, 10.0f), ImGuiCond_FirstUseEver);
	//ImGui::SetWindowSize(ImVec2(400.0f, 150.0f), ImGuiCond_FirstUseEver);

	Simulation& simulation = Application::instance().getSimulation();

	if (simulation.getSpeed() == 0.0f)
	{
		if (ImGui::Button("Play", ImVec2(50, 0)))
		{
			simulation.setSpeed(1.0f);
		}
	}
	else
	{
		if (ImGui::Button("Pause", ImVec2(50, 0)))
		{
			simulation.setSpeed(0.0f);
		}
	}

	ImGui::SameLine();
	ImGui::Text("Game Speed:   %.2fx", simulation.getSpeed());

	if (ImGui::Button("0.05x"))
	{
		simulation.setSpeed(0.05f);
	}
	ImGui::SameLine();
	if (ImGui::Button("0.2x"))
	{
		simulation.setSpeed(0.2f);
	}
	ImGui::SameLine();
	if (ImGui::Button("1x"))
	{
		simulation.setSpeed(1.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("3x"))
	{
		simulation.setSpeed(3.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("5x"))
	{
		simulation.setSpeed(5.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("10x"))
	{
		simulation.setSpeed(10.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Max"))
	{
		simulation.setSpeed(1000.0f);
	}

	ImGui::Text("Frame #%d", simulation.getFrameNumber());

	ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
	if (ImGui::Button("Rewind x100"))
	{
		simulation.setRewind(100);
	}
	ImGui::SameLine();
	if (ImGui::Button("x10"))
	{
		simulation.setRewind(10);
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Step Back", ImGuiDir_Left))
	{
		simulation.setRewind(1);
	}
	ImGui::SameLine();
	ImGui::Text("Step");
	ImGui::SameLine();
	if (ImGui::ArrowButton("Step Forward", ImGuiDir_Right))
	{
		simulation.setNextSingleStep(true, false);
	}
	ImGui::PopItemFlag();

#ifdef DEBUG
	// TEST: Gameplay connection
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
		if (ImGui::Button("Start as host"))
		{
			const bool USE_IPV6 = false;	// TODO: IPv6 does not work, the the server doesn't support it
			gameplayConnector.setupAsHost(GameplayConnector::DEFAULT_PORT, USE_IPV6);
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(nullptr != gameplayConnector.getGameplayClient());
		{
			static char ipBuffer[32] = "127.0.0.1";
			static int port = GameplayConnector::DEFAULT_PORT;
			if (ImGui::Button("Join as client"))
			{
				gameplayConnector.startConnectToHost(ipBuffer, port);
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

		if (!gameplayConnector.getExternalAddressQuery().mOwnExternalIP.empty())
		{
			ImGui::Text("Retrieved own external address: IP = %s, port = %d", gameplayConnector.getExternalAddressQuery().mOwnExternalIP.c_str(), gameplayConnector.getExternalAddressQuery().mOwnExternalPort);
		}
		ImGui::EndDisabled();
	}
#endif
}

#endif
