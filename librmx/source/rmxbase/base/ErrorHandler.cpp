/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include <locale>

#ifdef PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include "CleanWindowsInclude.h"

	//#define USE_VISTA_STYLE	// Not defined to reduce external dependencies -- espcially considering that the Vista-style message box code does not even seem to work!
	#ifdef USE_VISTA_STYLE
		// This redefinition is a bit hacky, but seems to be necessary
		#undef NTDDI_VERSION
		#define NTDDI_VERSION NTDDI_VISTA
		#include <Commctrl.h>

		#pragma comment(lib, "Dbghelp.lib")
		#pragma warning(push)
		#pragma warning(disable: 4091)	// Disable warning "C:\Program Files (x86)\Windows Kits\8.1\Include\um\DbgHelp.h(1544): warning C4091: 'typedef ': ignored on left of '' when no variable is declared"
		#include <DbgHelp.h>
		#pragma warning(pop)
	#endif
#endif

#ifdef PLATFORM_VITA
	#include <psp2/kernel/clib.h>
#endif


namespace
{

	static std::set<uint32> gIgnoredAssertHashes;

#ifdef PLATFORM_WINDOWS
#ifdef USE_VISTA_STYLE
	static void* mTaskDialogIndirectProcPointer = nullptr;
	static bool mLoadedProcPointers = false;

	bool canShowVistaStyleMessageBox()
	{
		// Try to load the Windows API function "TaskDialogIndirect"
		if (!mLoadedProcPointers)
		{
			const wchar_t* unicodeFilenameName = L"comctl32.dll";
			HMODULE module = GetModuleHandleW(unicodeFilenameName);
			if (nullptr == module)
			{
				module = LoadLibraryW(unicodeFilenameName);
			}

			if (nullptr != module)
			{
				// Try to get the "TaskDialogIndirect()" function pointer
				mTaskDialogIndirectProcPointer = GetProcAddress(module, "TaskDialogIndirect");
			}

			mLoadedProcPointers = true;
		}

		return (nullptr != mTaskDialogIndirectProcPointer);
	}

	int showVistaStyleMessageBox(const std::wstring& message)
	{
		int result = IDRETRY;

		// Sanity check (for obvious reason, we don't use RMX_CHECK here)
		if (nullptr == mTaskDialogIndirectProcPointer)
			return result;

		// Dialog button definition
		const TASKDIALOG_BUTTON* buttons = nullptr;
		uint32 numButtons = 0;

		static const TASKDIALOG_BUTTON buttonArray[] =
		{
			{ IDABORT,  L"&Break here\nBreak into source code, if a debugger is attached." },
			{ IDRETRY,  L"&Continue execution\nIgnore this message." },
		};
		buttons = buttonArray;
		numButtons = ARRAYSIZE(buttonArray);

		// Setup dialog configuration
		TASKDIALOGCONFIG config = { 0 };
		config.cbSize = sizeof(config);
		config.hwndParent = 0;
		config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;
		config.pszWindowTitle = L"Assert Break";
		config.pszMainIcon = TD_ERROR_ICON;
		config.pszMainInstruction = message.c_str();
		config.pszContent = L"Asset break";
		config.pszFooter = nullptr;
		config.pButtons = buttons;
		config.cButtons = numButtons;
		config.nDefaultButton = buttons[0].nButtonID;
		config.cxWidth = 250;
		config.pfCallback = nullptr;
		config.lpCallbackData = 0;

		// Call "::TaskDialogIndirect" now
		typedef HRESULT(WINAPI* TaskDialogIndirectProc)(const TASKDIALOGCONFIG* pTaskConfig, int* pnButton, int* pnRadioButton, BOOL* pfVerificationFlagChecked);
		reinterpret_cast<TaskDialogIndirectProc>(mTaskDialogIndirectProcPointer)(&config, &result, nullptr, nullptr);

		return result;
	}
#endif

	int showFallbackMessageBox(rmx::ErrorHandling::MessageBoxInterface::DialogType dialogType, rmx::ErrorSeverity errorSeverity, const std::string& message)
	{
		// Build output message
		std::stringstream stringBuilder;
		stringBuilder << message << "\n\n";

		const uint32 type = (dialogType == rmx::ErrorHandling::MessageBoxInterface::DialogType::ALL_OPTIONS) ? MB_YESNOCANCEL :
							(dialogType == rmx::ErrorHandling::MessageBoxInterface::DialogType::ACCEPT_OR_CANCEL) ? MB_OKCANCEL : MB_OK;
		const uint32 icon = (errorSeverity == rmx::ErrorSeverity::ERROR) ? MB_ICONERROR : MB_ICONWARNING;

		std::string caption;
		if (rmx::ErrorHandling::isDebuggerAttached())
		{
			stringBuilder << "Do you want to break here?\n";
			caption = "Assert Break";
		}
		else
		{
			caption = (errorSeverity == rmx::ErrorSeverity::ERROR) ? "Error" : "Warning";
		}

		// Show the message box
		return MessageBoxA(nullptr, stringBuilder.str().c_str(), caption.c_str(), type | icon);
	}

	int showWindowsMessageBox(rmx::ErrorHandling::MessageBoxInterface::DialogType dialogType, rmx::ErrorSeverity errorSeverity, const std::string& message)
	{
		// Show a message box, preferably Vista-style
	#ifdef USE_VISTA_STYLE
		if (canShowVistaStyleMessageBox())
		{
			const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &message.at(0), (int)message.size(), nullptr, 0);
			if (size_needed <= 0)
				return 0;

			std::wstring result(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, &message.at(0), (int)message.size(), &result.at(0), size_needed);
			return showVistaStyleMessageBox(result);
		}
		else
	#endif
		{
			// Fallback for Windows XP and below: Show default message box
			return showFallbackMessageBox(dialogType, errorSeverity, message);
		}
	}
#endif

}



namespace rmx
{

	bool ErrorHandling::isDebuggerAttached()
	{
	#if defined(PLATFORM_WINDOWS)
		return IsDebuggerPresent();
	#else
		return false;
	#endif
	}

	void ErrorHandling::printToLog(ErrorSeverity errorSeverity, const std::string& message)
	{
	#if !defined(PLATFORM_VITA)
		if (nullptr != mLogger)
		{
			mLogger->logMessage(errorSeverity, message);
		}
	#else
		sceClibPrintf("[ERROR] %s\n", message.c_str());
	#endif
	}

	bool ErrorHandling::handleAssertBreak(ErrorSeverity errorSeverity, const std::string& message, const char* filename, int line)
	{
		// Log message in any case
		printToLog(errorSeverity, message);

		// Check if ignored
		const uint32 hash = (uint32)getMurmur2_64(filename) ^ (uint32)line;
		if (gIgnoredAssertHashes.count(hash) != 0)
			return false;

		static bool isInsideAssertBreakHandler = false;
		if (isInsideAssertBreakHandler)
			return false;
		isInsideAssertBreakHandler = true;

		MessageBoxInterface::DialogType dialogType = MessageBoxInterface::DialogType::ACCEPT_OR_CANCEL;
		if (isDebuggerAttached())
		{
			dialogType = MessageBoxInterface::DialogType::ALL_OPTIONS;
		}

		MessageBoxInterface::Result result = MessageBoxInterface::Result::ABORT;
		if (nullptr != mMessageBoxImplementation)
		{
			result = mMessageBoxImplementation->showMessageBox(dialogType, errorSeverity, message, filename, line);
		}
	#ifdef PLATFORM_WINDOWS
		// Fallback, for Windows only
		else
		{
			const int mbResult = showWindowsMessageBox(dialogType, errorSeverity, message);
			result = (mbResult == IDCANCEL) ? MessageBoxInterface::Result::IGNORE :
					 (mbResult == IDYES) ? MessageBoxInterface::Result::ACCEPT : MessageBoxInterface::Result::ABORT;	// Both IDOK and IDNO count as ABORT
		}
	#endif

		// Ignore from now on?
		if (result == MessageBoxInterface::Result::IGNORE)
		{
			gIgnoredAssertHashes.insert(hash);
		}

		isInsideAssertBreakHandler = false;

		// If aborted, break now
		return (result == MessageBoxInterface::Result::ACCEPT && isDebuggerAttached());
	}
}
