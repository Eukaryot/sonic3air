/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
	// Objective C already has YES and NO defined which causes the compiler to try and expand NO below.
	#pragma push_macro("NO")
	#undef NO
#endif


class PlatformFunctions
{
public:
	enum class DialogButtons
	{
		OK,
		OK_CANCEL,
		YES_NO_CANCEL
	};
	enum class DialogResult
	{
		OK		= 0,	// Also used for result "YES"
		NO		= 1,
		CANCEL	= 2
	};

public:
#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
	#pragma pop_macro("NO")
	static std::wstring mExAppDataPath;
#endif

public:
	static void preciseDelay(double milliseconds);
	static double getTimerGranularityMilliseconds();

	static void changeWorkingDirectory(std::wstring_view executableCallPath);

	static void onEngineStartup();

	static void setAppIcon(int iconResource);
	static std::wstring getAppDataPath();
	static std::wstring tryGetSteamRomPath(const std::wstring& romName);

	static std::string getCompactSystemTimeString();

	static void showMessageBox(const std::string& caption, const std::string& text);
	static DialogResult showDialogBox(rmx::ErrorSeverity severity, DialogButtons dialogButtons, const std::string& caption, const std::string& text);
	static std::wstring openFileSelectionDialog(const std::wstring& title, const std::wstring& defaultFilename, const wchar_t* filter);

	static void openFileExternal(const std::wstring& path);
	static void openDirectoryExternal(const std::wstring& path);
	static void openURLExternal(const std::string& url);
	static bool openApplicationExternal(const std::wstring& path, const std::wstring& arguments, const std::wstring& directory = L"");

	static bool hasClipboardSupport();
	static bool copyToClipboard(const std::string& string);
	static bool copyToClipboard(std::wstring_view string);
	static bool pasteFromClipboard(std::wstring& outString);

	static bool isDebuggerPresent();
};
