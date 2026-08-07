/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/platform/CommandForwarder.h"
#include "oxygen/application/Application.h"

#ifdef PLATFORM_WINDOWS
	#include <windows.h>
	#undef ERROR
#endif


// Command forwarding implementation is platform-specific, and not available on all platforms
//  - Windows uses Named Pipes from the Windows API (I tried others like sending WM_COPYDATA, but receiving just did not work for some reason, so it's pipes now)

struct CommandForwarder::Internal
{
#ifdef PLATFORM_WINDOWS
	static inline const size_t MAX_BUFFER_SIZE = 1024;
	static inline std::string mPipeName;

	static void setApplicationName(std::string_view name)
	{
		mPipeName = "\\\\.\\Pipe\\" + std::string(name) + "_CommandForwarder";
	}

	static bool send(std::string_view command)
	{
		// TODO: Do a seperate check whether an instance is already running

		RMX_CHECK(command.length() + 1 < MAX_BUFFER_SIZE, "Buffer size of " << MAX_BUFFER_SIZE << " is too small for a command of length " << command.length(), return false);

		const HANDLE pipeHandle = CreateFile(mPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (pipeHandle == INVALID_HANDLE_VALUE)
		{
			// Pipe does not exist yet, so there's no other instance running
			return false;
		}

		char buffer[MAX_BUFFER_SIZE];
		memcpy(buffer, command.data(), command.length());
		buffer[command.length()] = 0;

		DWORD bytesSent = 0;
		const bool result = WriteFile(pipeHandle, buffer, (DWORD)(strlen(buffer)+1), &bytesSent, nullptr);
		RMX_CHECK(result && command.length() + 1 == bytesSent, "Error sending command: " << GetLastError(), );

		CloseHandle(pipeHandle);
		return true;
	}

	void startup()
	{
		mPipeHandle = CreateNamedPipe(mPipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
		RMX_CHECK(mPipeHandle != INVALID_HANDLE_VALUE, "Error creating named pipe: " << GetLastError(), );
	}

	void shutdown()
	{
		CloseHandle(mPipeHandle);
		mPipeHandle = INVALID_HANDLE_VALUE;
	}

	void update(CommandForwarder& owner, float deltaSeconds)
	{
		char buffer[MAX_BUFFER_SIZE];
		DWORD bytesRead = 0;
		const bool result = ReadFile(mPipeHandle, buffer, sizeof(buffer), &bytesRead, nullptr);
		if (result && bytesRead > 0 && bytesRead <= MAX_BUFFER_SIZE && buffer[bytesRead-1] == 0)
		{
			owner.handleReceivedCommand(std::string_view(buffer, bytesRead - 1));
		}
	}

	HANDLE mPipeHandle = INVALID_HANDLE_VALUE;

#else
	// Fallback implementation (does nothing at all)
	static void setApplicationName(std::string_view name) {}
	static bool send(std::string_view command) { return false; }
	void startup() {}
	void shutdown() {}
	void update(CommandForwarder& owner, float deltaSeconds) {}

#endif
};


void CommandForwarder::setApplicationName(std::string_view name)
{
	Internal::setApplicationName(name);
}

bool CommandForwarder::trySendCommand(std::string_view command)
{
	return Internal::send(command);
}

CommandForwarder::CommandForwarder() :
	mInternal(*new Internal())
{
}

CommandForwarder::~CommandForwarder()
{
	delete &mInternal;
}

void CommandForwarder::startup()
{
	mInternal.startup();
}

void CommandForwarder::shutdown()
{
	mInternal.shutdown();
}

void CommandForwarder::update(float deltaSeconds)
{
	mInternal.update(*this, deltaSeconds);
}

void CommandForwarder::handleReceivedCommand(std::string_view command)
{
	if (rmx::startsWith(command, "ForwardedCommand:"))
	{
		const std::string_view withoutPrefix = command.substr(17);
		if (rmx::startsWith(withoutPrefix, "Url:"))
		{
			const std::string_view url = withoutPrefix.substr(4);
			Application::instance().processUrl(url);
		}
		else
		{
			const std::string_view command = withoutPrefix;
			Application::instance().processForwardedCommand(command);
		}
	}
}
