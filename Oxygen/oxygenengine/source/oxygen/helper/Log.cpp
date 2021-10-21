/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Log.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/simulation/LemonScriptRuntime.h"

#ifdef PLATFORM_WINDOWS
	#include <CleanWindowsInclude.h>
#endif
#ifdef PLATFORM_ANDROID
	#include <android/log.h>
#endif


namespace
{
	class LoggerBase
	{
	public:
		virtual ~LoggerBase() {}
		virtual void log(const std::string& string) = 0;
	};

	class StdCoutLogger final : public LoggerBase
	{
	public:
		void log(const std::string& string) override
		{
			// Write to std::cout
			std::cout << string << "\r\n";
			std::cout << std::flush;

			// Write to debug output
			if (PlatformFunctions::isDebuggerPresent())
			{
				PlatformFunctions::debugLog(string);
			}

		#ifdef PLATFORM_ANDROID
			__android_log_print(ANDROID_LOG_INFO, "oxygen", "%s", string.c_str());
		#endif
		}
	};

	class FileLogger final : public LoggerBase
	{
	public:
		FileLogger(const std::wstring& filename)
		{
			// Create directory if needed
			const size_t slashPosition = filename.find_last_of(L"/\\");
			if (slashPosition != std::wstring::npos)
			{
				FTX::FileSystem->createDirectory(filename.substr(0, slashPosition));
			}

			mFileHandle.open(filename, FILE_ACCESS_WRITE);
		}

		void log(const std::string& string) override
		{
			// Write to file
			mFileHandle.write(string.c_str(), string.length());
			mFileHandle.write("\r\n", 2);
			mFileHandle.flush();
		}

	private:
		FileHandle mFileHandle;
	};

	std::vector<LoggerBase*> mLoggers;


	class ErrorLogger : public rmx::ErrorHandling::LoggerInterface, public rmx::ErrorHandling::MessageBoxInterface
	{
	protected:
		void logMessage(rmx::ErrorSeverity errorSeverity, const std::string& message) override
		{
			Log::log(message);
		}

		Result showMessageBox(rmx::ErrorHandling::MessageBoxInterface::DialogType dialogType, rmx::ErrorSeverity errorSeverity, const std::string& message, const char* filename, int line) override
		{
		#if defined(DEBUG)
			std::string name;
			std::string ext;
			rmx::FileIO::splitPath(filename, nullptr, &name, &ext);
			std::string text = message + "\nin " + name + "." + ext + ", line " + std::to_string(line);
		#else
			std::string text = message;
		#endif

			// Check if it was caused inside a script function
			{
				std::string functionName;
				std::wstring fileName;
				uint32 lineNumber = 0;
				std::string moduleName;
				if (LemonScriptRuntime::getCurrentScriptFunction(&functionName, &fileName, &lineNumber, &moduleName))
				{
					text += "\n\nCaused during script execution in function '" + functionName + "' at line " + std::to_string(lineNumber) + " of file '" + WString(fileName).toStdString() + "' in module '" + moduleName + "'.";
				}
			}

			std::string caption;
			if (rmx::ErrorHandling::isDebuggerAttached())
			{
				caption = "Assert Break";
				text += "\n\nBreak here?";
			}
			else
			{
				caption = (errorSeverity == rmx::ErrorSeverity::ERROR) ? "Error" : "Warning";
			}

			PlatformFunctions::DialogButtons dialogButtons = PlatformFunctions::DialogButtons::OK_CANCEL;
			switch (dialogType)
			{
				case rmx::ErrorHandling::MessageBoxInterface::DialogType::ACCEPT_ONLY:		dialogButtons = PlatformFunctions::DialogButtons::OK;			  break;
				case rmx::ErrorHandling::MessageBoxInterface::DialogType::ACCEPT_OR_CANCEL:	dialogButtons = PlatformFunctions::DialogButtons::OK_CANCEL;	  break;
				case rmx::ErrorHandling::MessageBoxInterface::DialogType::ALL_OPTIONS:		dialogButtons = PlatformFunctions::DialogButtons::YES_NO_CANCEL;  break;
			}
			const PlatformFunctions::DialogResult result = PlatformFunctions::showDialogBox(errorSeverity, dialogButtons, caption, text);
			return (result == PlatformFunctions::DialogResult::CANCEL) ? Result::IGNORE : (result == PlatformFunctions::DialogResult::OK) ? Result::ACCEPT : Result::ABORT;
		}
	};

	static ErrorLogger mErrorLogger;
}


void Log::startup(const std::wstring& filename)
{
	mLoggers.emplace_back(new StdCoutLogger());
	mLoggers.emplace_back(new FileLogger(filename));

	// Register as logger and message box callback for rmx error handling
	rmx::ErrorHandling::mLogger = &mErrorLogger;
	rmx::ErrorHandling::mMessageBoxImplementation = &mErrorLogger;
}

void Log::shutdown()
{
	for (LoggerBase* logger : mLoggers)
		delete logger;
	mLoggers.clear();
}

void Log::log(const std::string& string)
{
	for (LoggerBase* logger : mLoggers)
	{
		logger->log(string);
	}
}
