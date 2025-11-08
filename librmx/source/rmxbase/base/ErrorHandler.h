/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <sstream>
#include <stdexcept> // for std::runtime_error
#include <functional>


// Debug break (platform specific)
#ifdef _MSC_VER
	#define RMX_DEBUG_BREAK	__debugbreak()
#elif __APPLE__
	#define RMX_DEBUG_BREAK __builtin_trap()
#elif defined(__ANDROID__)
	#define RMX_DEBUG_BREAK assert(false)
#else
	#define RMX_DEBUG_BREAK {}
#endif


// Asserts
#define RMX_CONDITIONAL_ERROR(severity, condition, explanation, reaction) \
{ \
	if (!(condition)) \
	{ \
		std::ostringstream _stream_; \
		_stream_ << explanation; \
		if (rmx::ErrorHandling::handleAssertBreak(severity, _stream_.str(), __FILE__, __LINE__)) \
			RMX_DEBUG_BREAK; \
		reaction; \
	} \
}

#ifdef PLATFORM_WINDOWS
	#define RMX_REACT_THROW throw std::runtime_error(_stream_.str())
#else
	#define RMX_REACT_THROW
#endif

#ifdef DEBUG
	#define RMX_ASSERT(condition, message)		RMX_CONDITIONAL_ERROR(rmx::ErrorSeverity::ERROR, condition, message, )
#else
	#define RMX_ASSERT(condition, message)		{}
#endif

#define RMX_CHECK(condition, message, reaction)	RMX_CONDITIONAL_ERROR(rmx::ErrorSeverity::ERROR, condition, message, reaction)
#define RMX_ERROR(message, reaction)			RMX_CONDITIONAL_ERROR(rmx::ErrorSeverity::ERROR, false, message, reaction)


namespace rmx
{
	enum class ErrorSeverity
	{
		INFO,
		WARNING,
		ERROR
	};

	struct ErrorHandling
	{
	public:
		class LoggerInterface
		{
		public:
			virtual void logMessage(ErrorSeverity errorSeverity, const std::string& message) = 0;
		};

		class MessageBoxInterface
		{
		public:
			enum class DialogType
			{
				OK,
				OK_CANCEL,
				YES_NO_CANCEL
			};
			enum class Result
			{
				ACCEPT,
				ABORT,
				IGNORE
			};

		public:
			virtual Result showMessageBox(DialogType dialogType, ErrorSeverity errorSeverity, const std::string& message, const char* filename, int line) = 0;
		};

	public:
		static bool isDebuggerAttached();
		static void printToLog(ErrorSeverity errorSeverity, const std::string& message);
		static bool handleAssertBreak(ErrorSeverity errorSeverity, const std::string& message, const char* filename, int line);
		static bool isIgnoringAssertsWithHash(uint64 hash);
		static void setIgnoreAssertsWithHash(uint64 hash, bool ignore);

	public:
		static inline LoggerInterface* mLogger = nullptr;
		static inline MessageBoxInterface* mMessageBoxImplementation = nullptr;
		static inline std::function<uint64()> mNativeWindowHandleProvider;
		static inline bool mShowAssertMessageBox = true;
	};
}
