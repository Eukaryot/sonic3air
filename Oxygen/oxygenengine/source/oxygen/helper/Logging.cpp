/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/simulation/LemonScriptRuntime.h"


namespace
{

	class ErrorLogger : public rmx::ErrorHandling::LoggerInterface, public rmx::ErrorHandling::MessageBoxInterface
	{
	public:
		std::string mCaption;

	protected:
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

		Result showMessageBox(rmx::ErrorHandling::MessageBoxInterface::DialogType dialogType, rmx::ErrorSeverity errorSeverity, const std::string& message, const char* filename, int line) override
		{
		#if defined(DEBUG)
			std::string name;
			std::string ext;
			rmx::FileIO::splitPath(filename, nullptr, &name, &ext);
			std::string text = message + "\n[" + name + "." + ext + ", line " + std::to_string(line) + "]";
		#else
			std::string text = message;
		#endif

			// Check if it was caused inside a script function
			{
				std::string_view functionName;
				std::wstring fileName;
				uint32 lineNumber = 0;
				std::string moduleName;
				if (LemonScriptRuntime::getCurrentScriptFunction(&functionName, &fileName, &lineNumber, &moduleName))
				{
					text += "\n\nCaused during script execution in function '" + std::string(functionName) + "' at line " + std::to_string(lineNumber) + " of file '" + WString(fileName).toStdString() + "' in module '" + moduleName + "'.";
				}
			}

			std::string caption = (errorSeverity == rmx::ErrorSeverity::ERROR) ? "Error" : "Warning";
			if (!mCaption.empty())
			{
				caption = mCaption + " - " + caption;
			}

			if (rmx::ErrorHandling::isDebuggerAttached())
			{
				text += "\n\nBreak here?";
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



namespace oxygen
{
	void Logging::startup(const std::wstring& filename)
	{
		rmx::Logging::addLogger(*new rmx::StdCoutLogger());
		rmx::Logging::addLogger(*new rmx::FileLogger(filename, true));

		// Register as logger and message box callback for rmx error handling
		rmx::ErrorHandling::mLogger = &mErrorLogger;
		rmx::ErrorHandling::mMessageBoxImplementation = &mErrorLogger;
	}

	void Logging::shutdown()
	{
		rmx::Logging::clear();
	}

	void Logging::setAssertBreakCaption(const std::string& caption)
	{
		::mErrorLogger.mCaption = caption;
	}
}
