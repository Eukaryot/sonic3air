/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{

	enum class LogLevel
	{
		TRACE,
		INFO,
		WARNING,
		ERROR
	};


	class LoggerBase
	{
	public:
		virtual ~LoggerBase() {}
		virtual void log(LogLevel logLevel, const std::string& string) = 0;
	};


	class StdCoutLogger final : public LoggerBase
	{
	public:
		explicit StdCoutLogger(bool addTimestamp = false);
		void log(LogLevel logLevel, const std::string& string) override;

	private:
		bool mAddTimestamp = false;
	};


	class FileLogger final : public LoggerBase
	{
	public:
		FileLogger(const std::wstring& filename, bool addTimestamp = false, bool renameExisting = false);
		void log(LogLevel logLevel, const std::string& string) override;

	private:
		FileHandle mFileHandle;
		bool mAddTimestamp = false;
	};


	class Logging
	{
	public:
		static void clear();
		static void addLogger(LoggerBase& logger);
		static void log(LogLevel logLevel, const std::string& string);

	private:
		static inline std::vector<LoggerBase*> mLoggers;
	};

}


#define RMX_LOG_TRACE(_message_) \
{ \
	std::ostringstream stream; \
	stream << _message_; \
	rmx::Logging::log(rmx::LogLevel::TRACE, stream.str()); \
}

#define RMX_LOG_INFO(_message_) \
{ \
	std::ostringstream stream; \
	stream << _message_; \
	rmx::Logging::log(rmx::LogLevel::INFO, stream.str()); \
}

#define RMX_LOG_WARNING(_message_) \
{ \
	std::ostringstream stream; \
	stream << _message_; \
	rmx::Logging::log(rmx::LogLevel::WARNING, stream.str()); \
}

#define RMX_LOG_ERROR(_message_) \
{ \
	std::ostringstream stream; \
	stream << _message_; \
	rmx::Logging::log(rmx::LogLevel::ERROR, stream.str()); \
}
