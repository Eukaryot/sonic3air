/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/base/CrashHandler.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/Logging.h"

#ifdef PLATFORM_WINDOWS
	#include <windows.h>
	#include <dbghelp.h>
#endif


namespace
{
#ifdef PLATFORM_WINDOWS

	// The following is based on https://www.c-plusplus.net/forum/topic/261827/setunhandledexceptionfilter-und-minidumpwritedump/3

	typedef BOOL (__stdcall *tMDWD)(
		IN HANDLE hProcess,
		IN DWORD ProcessId,
		IN HANDLE hFile,
		IN MINIDUMP_TYPE DumpType,
		IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
		IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
		IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
		);

	static std::string s_ApplicationInfo;
	static tMDWD s_pMDWD;
	static HMODULE s_hDbgHelpMod;

	static LONG __stdcall MyCrashHandlerExceptionFilter(EXCEPTION_POINTERS* pEx)
	{
	#ifdef _M_IX86
		if (pEx->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
		{
			// Be sure that we have enough space...
			static char MyStack[1024*128];
			// It assumes that DS and SS are the same!!! (this is the case for Win32)
			// Change the stack only if the selectors are the same (this is the case for Win32)
			//__asm push offset MyStack[1024*128];
			//__asm pop esp;
			__asm mov eax,offset MyStack[1024*128];
			__asm mov esp,eax;
		}
	#endif

		WString crashDumpPath = L"crashdump.dmp";

		String text;
		text << "Unhandled exception: " << rmx::hexString((uint32)pEx->ExceptionRecord->ExceptionCode, 8) << "\n\n";
		text << "A crash dump with useful data for the developers is written to\n\"" << crashDumpPath.toString() << "\".";
		if (!s_ApplicationInfo.empty())
			text << "\n\n" << s_ApplicationInfo;
		PlatformFunctions::showMessageBox("Application has crashed!", *text);

		// Create a mini dump
		bool success = false;
		const HANDLE hFile = CreateFileW(*crashDumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION stMDEI;
			stMDEI.ThreadId = GetCurrentThreadId();
			stMDEI.ExceptionPointers = pEx;
			stMDEI.ClientPointers = TRUE;
			const MINIDUMP_TYPE dumpTyp = MiniDumpNormal;
			success = (s_pMDWD(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpTyp, &stMDEI, nullptr, nullptr) != FALSE);
			CloseHandle(hFile);

			if (!success)
			{
				PlatformFunctions::showMessageBox("More bad news...", "Crash dump creation failed for some reason...");
			}
		}
		else
		{
			PlatformFunctions::showMessageBox("More bad news...", "Crash dump file could not be written...");
		}

		return success ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
	}

	void InitMiniDumpWriter()
	{
		if (s_hDbgHelpMod != nullptr)
			return;

		// Initialize the member, so we do not load the DLL after the exception has occured which might be not possible anymore...
		s_hDbgHelpMod = LoadLibraryA("dbghelp.dll");
		if (s_hDbgHelpMod != nullptr)
			s_pMDWD = (tMDWD)GetProcAddress(s_hDbgHelpMod, "MiniDumpWriteDump");

		// Register the unhandled exception filter
		SetUnhandledExceptionFilter(MyCrashHandlerExceptionFilter);
	}

#endif
}


void CrashHandler::initializeCrashHandler()
{
#ifdef PLATFORM_WINDOWS
	InitMiniDumpWriter();
#endif
}

void CrashHandler::setApplicationInfo(const std::string& applicationInfo)
{
#ifdef PLATFORM_WINDOWS
	s_ApplicationInfo = applicationInfo;
#endif
}
