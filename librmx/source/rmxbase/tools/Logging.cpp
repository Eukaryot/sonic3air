/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"

#if defined(PLATFORM_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <CleanWindowsInclude.h>
#elif defined(PLATFORM_ANDROID)
	#include <android/log.h>
#endif

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>


namespace rmx
{
	namespace detail
	{
		std::string getTimestampString()
		{
			time_t now = time(0);
			struct tm tstruct;
			char buf[80];
		#if defined(PLATFORM_WINDOWS)
			localtime_s(&tstruct, &now);
		#else
			tstruct = *localtime(&now);
		#endif
			// Format example: "[2022-06-29 11:42:48] "
			std::strftime(buf, sizeof(buf), "[%Y-%m-%d %T] ", &tstruct);
			return buf;
		}

		std::string getFilenameString()
		{
			time_t now = time(0);
			struct tm tstruct;
			char buf[80];
		#if defined(PLATFORM_WINDOWS)
			localtime_s(&tstruct, &now);
		#else
			tstruct = *localtime(&now);
		#endif
			// Format example: "2022-06-29_11-42-48"
			std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tstruct);
			return buf;
		}
	}


	StdCoutLogger::StdCoutLogger(bool addTimestamp) :
		mAddTimestamp(addTimestamp)
	{
	}

	void StdCoutLogger::log(LogLevel logLevel, const std::string& string)
	{
		// Write to std::cout
		if (mAddTimestamp)
		{
			std::cout << detail::getTimestampString();
		}
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


	FileLogger::FileLogger(const std::wstring& filename, bool addTimestamp, bool renameExisting) :
		mAddTimestamp(addTimestamp)
	{
		// Create directory if needed
		const size_t slashPosition = filename.find_last_of(L"/\\");
		if (slashPosition != std::wstring::npos)
		{
			FTX::FileSystem->createDirectory(filename.substr(0, slashPosition));
		}

		if (renameExisting && FTX::FileSystem->exists(filename))
		{
			const time_t time = FTX::FileSystem->getFileTime(filename);
			std::wstring directory;
			std::wstring name;
			std::wstring extension;
			FTX::FileSystem->splitPath(filename, &directory, &name, &extension);
			name += L"_" + String(detail::getFilenameString()).toStdWString();
			if (!directory.empty())
				directory += L'/';
			FTX::FileSystem->renameFile(filename, directory + name + L'.' + extension);
		}

		mFileHandle.open(filename, FILE_ACCESS_WRITE);
	}

	void FileLogger::log(LogLevel logLevel, const std::string& string)
	{
		if (mAddTimestamp)
		{
			std::string timestampString = detail::getTimestampString();
			mFileHandle.write(timestampString.c_str(), timestampString.length());
		}

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
