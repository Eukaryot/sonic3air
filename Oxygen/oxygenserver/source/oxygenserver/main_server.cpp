/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/server/Server.h"
#include "oxygenserver/server/CrashHandler.h"
#include "oxygenserver/Configuration.h"


#if defined(PLATFORM_WINDOWS)

	#include <windows.h>
	#undef ERROR

	BOOL WINAPI ConsoleHandler(DWORD CEvent)
	{
		if (CEvent == CTRL_C_EVENT || CEvent == CTRL_BREAK_EVENT)	// Unfortunately, this does not work with CTRL_CLOSE_EVENT, as it would lead to immediate shutdown in any case
		{
			Server::mReceivedCloseEvent = true;
			return TRUE;
		}
		return FALSE;
	}

#endif


namespace
{
	// Copy of "PlatformFunctions::changeWorkingDirectory"
	void changeWorkingDirectory(const std::string& execCallPath)
	{
	#if defined(PLATFORM_WINDOWS)
		// Move out of "bin", "build" or "_vstudio" directory
		//  -> This is added only because with my Visual Studio setup, binaries get placed in such a target directory (don't ask why...)
		const std::wstring path = rmx::FileSystem::getCurrentDirectory();
		std::vector<std::wstring> parts;
		for (size_t pos = 0; pos < path.length(); ++pos)
		{
			const size_t start = pos;

			// Find next separator
			while (pos < path.length() && !(path[pos] == L'\\' || path[pos] == L'/'))
				++pos;

			// Get part as string
			parts.emplace_back(path.substr(start, pos-start));
		}

		for (size_t index = 0; index < parts.size(); ++index)
		{
			if (parts[index] == L"bin" || parts[index] == L"build" || parts[index] == L"_vstudio")
			{
				std::wstring wd;
				for (size_t i = 0; i < index; ++i)
					wd += parts[i] + L'/';
				rmx::FileSystem::setCurrentDirectory(wd);
				break;
			}
		}
	#elif defined(PLATFORM_LINUX)
		// Take the working directory from command line if possible
		//  -> This seems to be needed in some cases, like when using a .desktop file as launcher
		WString path;
		path.fromUTF8(execCallPath);
		const int pos = path.findChar(L'/', path.length()-1, -1);
		if (pos >= 0)
		{
			rmx::FileSystem::setCurrentDirectory(*path.getSubString(0, pos));
		}
	#endif
	}

	class ErrorLogger : public rmx::ErrorHandling::LoggerInterface
	{
		void logMessage(rmx::ErrorSeverity errorSeverity, const std::string& message) override
		{
			switch (errorSeverity)
			{
				default:
				case rmx::ErrorSeverity::INFO:	  rmx::Logging::log(rmx::LogLevel::INFO, message);	  break;
				case rmx::ErrorSeverity::WARNING: rmx::Logging::log(rmx::LogLevel::WARNING, message); break;
				case rmx::ErrorSeverity::ERROR:	  rmx::Logging::log(rmx::LogLevel::ERROR, message);	  break;
			}
		}
	};

	static ErrorLogger mErrorLogger;
}



int main(int argc, char** argv)
{
	INIT_RMX;

	// Make sure we're in the correct working directory
	changeWorkingDirectory(argc == 0 ? "" : argv[0]);

	// Setup crash handling
	CrashHandler::initializeCrashHandler();

	randomize();

	// Logging
	{
		rmx::LoggerBase* logger = new rmx::StdCoutLogger(true);
		logger->setLogLevelRange(rmx::LogLevel::INFO);
		rmx::Logging::addLogger(*logger);

		logger = new rmx::FileLogger(L"log/server.log", true, true);
		logger->setLogLevelRange(rmx::LogLevel::INFO);
		rmx::Logging::addLogger(*logger);

	#if 0
		// Trace log, only for debugging
		logger = new rmx::FileLogger(L"log/trace.log", true, true);
		logger->setLogLevelRange(rmx::LogLevel::TRACE);
		rmx::Logging::addLogger(*logger);
	#endif

		rmx::ErrorHandling::mLogger = &mErrorLogger;
		rmx::ErrorHandling::mShowAssertMessageBox = false;	// Don't ever show assert message boxes for the server, just log the error
	}

#if defined(PLATFORM_WINDOWS)
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);
#endif

#ifndef DEBUG
	// To protect users' privacy, do not log IPs at all on production servers
	SocketAddress::mPreventIPLogging = true;
#endif

	Configuration config;
	config.loadConfiguration(L"config.json");

	Sockets::startupSockets();
	{
		Server server;
		server.runServer();
	}
	Sockets::shutdownSockets();
	return 0;
}
