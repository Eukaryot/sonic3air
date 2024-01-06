/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#define RMX_LIB

#include "oxygen_netcore/network/ConnectionManager.h"
#include "oxygen_netcore/network/RequestBase.h"
#include "oxygen_netcore/network/NetConnection.h"
#include "oxygen_netcore/network/ServerClientBase.h"
#include "oxygen_netcore/serverclient/Packets.h"
#include "oxygen_netcore/serverclient/ProtocolVersion.h"

#include "PrivatePackets.h"
#include "Shared.h"

#include <thread>
#include <Windows.h>
#include <TlHelp32.h>
#undef ERROR


void killServerProcess()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	if (Process32First(snapshot, &process))
	{
		do
		{
			if (std::wstring(process.szExeFile) == L"oxygenserver.exe")
			{
				if (GetCurrentProcessId() != process.th32ProcessID)
				{
					HANDLE handle = OpenProcess(PROCESS_TERMINATE, false, process.th32ProcessID);
					if (nullptr != handle)
					{
						TerminateProcess(handle, 0);
						CloseHandle(handle);
					}
					break;
				}
			}
		}
		while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
}


class Restarter : public ServerClientBase
{
public:
	void runRestarter();

protected:
	virtual NetConnection* createNetConnection(ConnectionManager& connectionManager, const SocketAddress& senderAddress) override
	{
		// Do not allow incoming connections
		return nullptr;
	}

	virtual void destroyNetConnection(NetConnection& connection) override
	{
		RMX_ASSERT(false, "This should never get called");
	}

private:
	void updateClient(uint64 currentTimestamp);

private:
	enum class State
	{
		NONE,
		START_CONNECTION,
		WAITING_FOR_CONNECTION,
		CONNECTED,
		REQUEST_SENT,
		TRIGGER_RESTART,
		RESTART_STEP_1,
		RESTART_STEP_2,
		EXIT
	};

	State mState = State::NONE;
	NetConnection mConnection;
	ConnectionManager* mConnectionManager = nullptr;
	network::GetServerFeaturesRequest mGetServerFeaturesRequest;
	uint64 mTimestampOfLastSend = 0;
	uint64 mTimestampOfLastStateChange = 0;
};


void Restarter::runRestarter()
{
	// Switch between UDP and TCP usage
	UDPSocket udpSocket;
	if (!udpSocket.bindToAnyPort())
		RMX_ERROR("Socket bind to any port failed", return);
	ConnectionManager connectionManager(&udpSocket, nullptr, *this, network::HIGHLEVEL_PROTOCOL_VERSION_RANGE);
	mConnectionManager = &connectionManager;

	mState = State::START_CONNECTION;

	uint64 lastTimestamp = getCurrentTimestamp();
	uint64 lastMessageTimestamp = 0;
	while (mState != State::EXIT)
	{
		const uint64 currentTimestamp = getCurrentTimestamp();
		const uint64 millisecondsElapsed = currentTimestamp - lastTimestamp;
		lastTimestamp = currentTimestamp;

		// Check for new packets
		if (!updateReceivePackets(connectionManager))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		connectionManager.updateConnections(currentTimestamp);

		// Update client state
		updateClient(currentTimestamp);
	}
}

void Restarter::updateClient(uint64 currentTimestamp)
{
	switch (mState)
	{
		case State::START_CONNECTION:
		{
			// Connect to server
			//RMX_LOG_INFO("Starting connection");	// Already logged inside "startConnectTo"
			const SocketAddress serverAddress("127.0.0.1", UDP_SERVER_PORT);
			if (!mConnection.startConnectTo(*mConnectionManager, serverAddress, getCurrentTimestamp()))
			{
				RMX_ERROR("Starting a connection failed", return);
			}
			mState = State::WAITING_FOR_CONNECTION;
			break;
		}

		case State::WAITING_FOR_CONNECTION:
		{
			if (mConnection.getState() == NetConnection::State::CONNECTED)
			{
				RMX_LOG_INFO("Now connected");
				mState = State::CONNECTED;
			}
			else if (mConnection.getState() == NetConnection::State::DISCONNECTED)
			{
				RMX_LOG_INFO("Disconnected");
				mState = State::TRIGGER_RESTART;
			}
			break;
		}

		case State::CONNECTED:
		{
			if (currentTimestamp - mTimestampOfLastSend > 30000)	// 30 seconds
			{
				// Send request
				mConnection.sendRequest(mGetServerFeaturesRequest);
				mTimestampOfLastSend = currentTimestamp;
				mState = State::REQUEST_SENT;
			}
			break;
		}

		case State::REQUEST_SENT:
		{
			if (mGetServerFeaturesRequest.hasResponse())
			{
				mState = State::CONNECTED;
			}
			else if (currentTimestamp - mTimestampOfLastSend > 15000)	// 15 seconds
			{
				RMX_LOG_INFO("Timeout for the response");
				const SocketAddress serverAddress("127.0.0.1", UDP_SERVER_PORT);
				mConnection.startConnectTo(*mConnectionManager, serverAddress, getCurrentTimestamp());
				mState = State::WAITING_FOR_CONNECTION;
			}
			break;
		}

		case State::TRIGGER_RESTART:
		{
			RMX_LOG_INFO("");
			RMX_LOG_INFO("-----");

			// Kill the old server instance
			RMX_LOG_INFO("Killing running server instance");
			killServerProcess();

			mTimestampOfLastStateChange = currentTimestamp;
			mState = State::RESTART_STEP_1;
			break;
		}

		case State::RESTART_STEP_1:
		{
			if (currentTimestamp - mTimestampOfLastSend > 5000)		// 5 seconds
			{
				// Restart the server now
				RMX_LOG_INFO("Starting server instance again");

				char commandLine[] = "oxygenserver.exe";
				STARTUPINFOA si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				ZeroMemory(&pi, sizeof(pi));
				si.cb = sizeof(si);
				if (!CreateProcessA(nullptr, commandLine, nullptr, nullptr, false, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) 
				{
					RMX_LOG_INFO("CreateProcess failed with error code: " << GetLastError());
				}

				mState = State::START_CONNECTION;
			}
			break;
		}
	}
}


int main(int argc, char** argv)
{
	rmx::Logging::addLogger(*new rmx::StdCoutLogger(true));
	Sockets::startupSockets();
	{
		Restarter restarter;
		restarter.runRestarter();
	}
	Sockets::shutdownSockets();
	return 0;
}
