/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"

#if defined(PLATFORM_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <CleanWindowsInclude.h>
#elif defined(PLATFORM_ANDROID)
	#include <android/log.h>
#endif


namespace rmx
{

	void StdCoutLogger::log(LogLevel logLevel, const std::string& string)
	{
		// Write to std::cout
		std::cout << string << "\r\n";
		std::cout << std::flush;

		// Write to debug output, depending on platform
	#if defined(PLATFORM_WINDOWS)
		if (IsDebuggerPresent() != 0)
		{
			OutputDebugString((string + "\r\n").c_str());
		}
	#elif defined(PLATFORM_ANDROID)
		{
			__android_log_print(ANDROID_LOG_INFO, "rmx", "%s", string.c_str());
		}
	#endif
	}


	FileLogger::FileLogger(const std::wstring& filename)
	{
		// Create directory if needed
		const size_t slashPosition = filename.find_last_of(L"/\\");
		if (slashPosition != std::wstring::npos)
		{
			FTX::FileSystem->createDirectory(filename.substr(0, slashPosition));
		}

		mFileHandle.open(filename, FILE_ACCESS_WRITE);
	}

	void FileLogger::log(LogLevel logLevel, const std::string& string)
	{
		// Write to file
		mFileHandle.write(string.c_str(), string.length());
		mFileHandle.write("\r\n", 2);
		mFileHandle.flush();
	}



	void Logging::clear()
	{
		for (LoggerBase* logger : mLoggers)
			delete logger;
		mLoggers.clear();
	}

	void Logging::addLogger(LoggerBase& logger)
	{
		mLoggers.emplace_back(&logger);
	}

	void Logging::log(LogLevel logLevel, const std::string& string)
	{
		for (LoggerBase* logger : mLoggers)
		{
			logger->log(logLevel, string);
		}
	}

}
