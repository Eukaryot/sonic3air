/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/HighResolutionTimer.h"
#include "oxygen/helper/Logging.h"

#include <thread>

#ifdef PLATFORM_WINDOWS
	#include <CleanWindowsInclude.h>
	#include <shlobj.h>		// For "SHGetKnownFolderPath"
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC) || defined(PLATFORM_ANDROID) || defined(PLATFORM_SWITCH) || defined(PLATFORM_IOS)
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
#endif
#ifdef PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
#endif


namespace
{
#ifdef PLATFORM_WINDOWS

	std::wstring GetStringRegKey(HKEY hKey, const wchar_t* valueName)
	{
		WCHAR szBuffer[512];
		DWORD dwBufferSize = sizeof(szBuffer);
		ULONG nError;
		nError = RegQueryValueExW(hKey, valueName, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
		return (ERROR_SUCCESS == nError) ? szBuffer : L"";
	}

	std::wstring getSteamInstallationPath()
	{
		HKEY hKey;
		LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", 0, KEY_READ, &hKey);
		if (lRes == ERROR_SUCCESS)
		{
			return GetStringRegKey(hKey, L"SteamPath");
		}
		return L"";
	}

	void collectSteamInstallFolders(std::vector<WString>& outPaths, const std::wstring& steamPath)
	{
		// First try to get the base install folder from "config.vdf"
		//  -> In my latest test, this did not work any more for me :(
		WString text;
		if (text.loadFile(steamPath + L"/config/config.vdf"))
		{
			int pos = 0;
			while (pos < text.length())
			{
				WString line;
				pos = text.getLine(line, pos);
				int i = line.findChar('"', 0, 1);
				int k = line.findChar('"', i + 1, 1);
				if (k + 1 < line.length() && line[i + 1] == 'B')
				{
					const WString key = line.getSubString(i + 1, k - i - 1);
					if (key.startsWith(L"BaseInstallFolder"))
					{
						i = line.findChar('"', k + 1, 1);
						k = line.findChar('"', i + 1, 1);
						if (k < line.length())
						{
							WString value = line.getSubString(i + 1, k - i - 1);
							value.replace(L"\\\\", L"\\");
							outPaths.push_back(value);
							break;
						}
					}
				}
			}
		}

		// Also check the "libraryfolders.vdf" for search paths
		if (text.loadFile(steamPath + L"/config/libraryfolders.vdf"))
		{
			int pos = 0;
			while (pos < text.length())
			{
				WString line;
				pos = text.getLine(line, pos);
				int i = line.findChar('"', 0, 1);
				int k = line.findChar('"', i + 1, 1);
				if (k + 1 < line.length() && line[i + 1] == 'p')
				{
					const WString key = line.getSubString(i + 1, k - i - 1);
					if (key.startsWith(L"path"))
					{
						i = line.findChar('"', k + 1, 1);
						k = line.findChar('"', i + 1, 1);
						if (k < line.length())
						{
							WString value = line.getSubString(i + 1, k - i - 1);
							value.replace(L"\\\\", L"\\");
							value.replace(L"\\\\", L"\\");
							outPaths.push_back(value);
						}
					}
				}
			}
		}
	}
#endif

#ifdef PLATFORM_LINUX
	bool isValidNonRootDir(const char* dir)
	{
		return (nullptr != dir && dir[0] != 0 && (dir[0] != '/' || dir[1] != 0));
	}

	String getLinuxHomeDir()
	{
		const char* homedir = getenv("HOME");
		if (!isValidNonRootDir(homedir))
		{
			homedir = getpwuid(getuid())->pw_dir;
		}
		if (isValidNonRootDir(homedir))
		{
			return String(homedir);
		}
		return String();
	}

	String getLinuxAppDataDir()
	{
		const char* xdgDataHome = getenv("XDG_DATA_HOME");
		if (isValidNonRootDir(xdgDataHome))
		{
			return String(xdgDataHome);
		}
		const String homeDir = getLinuxHomeDir();
		if (!homeDir.empty())
		{
			return homeDir + "/.local/share";
		}
		return String();
	}
#endif

	WString lookForROMFileInSearchPaths(const std::vector<WString>& searchPaths, const WString& localPath)
	{
		for (const WString& searchPath : searchPaths)
		{
			WString romFilename = searchPath + localPath;
			RMX_LOG_INFO("Searching ROM at location: " << romFilename.toStdString());

			if (FTX::FileSystem->exists(*romFilename))
			{
				RMX_LOG_INFO("Success!");
				return *romFilename;
			}
			RMX_LOG_INFO("Not found");
		}
		return WString();
	}
}



void PlatformFunctions::preciseDelay(double milliseconds)
{
	// This function is based on work by Sewer56, and used with his permission here
	//  -> For the original, see https://github.com/Sewer56/sonic3air

	const double timerGranularity = getTimerGranularityMilliseconds();	// As a side effect, this function sets the optimal timer granularity

	HighResolutionTimer timer;
	timer.start();
	while (true)
	{
		const double timeLeft = milliseconds - timer.getSecondsSinceStart() * 1000.0;
		if (timeLeft <= 0.0)
			break;

		const double sleepTimeLeft = timeLeft - timerGranularity;

		// Don't spin on mobile platforms, accept some imprecision to avoid battery drain
		#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_LINUX)
		{
			// Spin if below granularity
			if (sleepTimeLeft < 0.0)
			{
				double lastYieldTimeMs = std::numeric_limits<double>::max();
				while (true)
				{
					const double timeLeft = milliseconds - timer.getSecondsSinceStart() * 1000.0;
					if (timeLeft <= 0.0)
						break;

					if (timeLeft > lastYieldTimeMs)		// Otherwise it's essentially busy waiting
					{
						HighResolutionTimer yieldTimer;
						yieldTimer.start();
						std::this_thread::yield();
						lastYieldTimeMs = yieldTimer.getSecondsSinceStart();
					}
				}
				break;
			}
		}
		#endif

		// Is the remaining time rounded down to full milliseconds above the timer granularity?
		if (sleepTimeLeft >= 1.0)
		{
			// Sleep the thread if above granularity
			std::this_thread::sleep_for(std::chrono::milliseconds((int)sleepTimeLeft));
		}
		else
		{
			// Yield the thread if below granularity
			std::this_thread::yield();
		}
	}
}

double PlatformFunctions::getTimerGranularityMilliseconds()
{
	// This function is based on work by Sewer56, and used with his permission here
	//  -> For the original, see https://github.com/Sewer56/sonic3air

	static double timerGranularity = 0.0;
	static bool initialized = false;
	if (!initialized)
	{
	#ifdef PLATFORM_WINDOWS
		// Query range of possible timer granularities and use the minimum if possible
		const HINSTANCE hLibrary = LoadLibrary("NTDLL.dll");
		if (nullptr != hLibrary)
		{
			typedef NTSTATUS(NTAPI* pQueryTimerResolution)(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution);
			typedef NTSTATUS(NTAPI* pSetTimerResolution)(ULONG RequestedResolution, BOOLEAN Set, PULONG ActualResolution);

			pSetTimerResolution setFunction = (pSetTimerResolution)GetProcAddress(hLibrary, "NtSetTimerResolution");
			pQueryTimerResolution queryFunction = (pQueryTimerResolution)GetProcAddress(hLibrary, "NtQueryTimerResolution");
			if (nullptr != setFunction && nullptr != queryFunction)
			{
				// Note that these resolutions are measured in units of 100 nanoseconds
				ULONG minResolution, maxResolution, actualResolution;
				queryFunction(&minResolution, &maxResolution, &actualResolution);
				const NTSTATUS status = setFunction(maxResolution, TRUE, &actualResolution);
				if (status == 0)
				{
					timerGranularity = actualResolution / 10000.0;	// Convert to milliseconds
					initialized = true;
				}
			}
		}
	#endif

		// For other platforms, or when something went wrong, assume a timer granularity of 1 millisecond
		if (!initialized)
		{
			timerGranularity = 1.0;
			initialized = true;
		}
	}
	return timerGranularity;
}

void PlatformFunctions::changeWorkingDirectory(const std::string& execCallPath)
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

void PlatformFunctions::onEngineStartup()
{
#if defined(PLATFORM_WINDOWS)
	// Handle DPI scaling by Windows
	SetProcessDPIAware();
#endif
}

void PlatformFunctions::setAppIcon(int iconResource)
{
#ifdef PLATFORM_WINDOWS
	if (iconResource != 0)
	{
		HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(iconResource));
		SendMessage((HWND)FTX::Video->getNativeWindowHandle(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
#endif
}

#if defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
	std::wstring PlatformFunctions::mExAppDataPath = L"";
#endif

std::wstring PlatformFunctions::getAppDataPath()
{
#ifdef PLATFORM_WINDOWS
	PWSTR path = NULL;
	if (S_OK == SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DONT_UNEXPAND | KF_FLAG_CREATE, nullptr, &path))
	{
		std::wstring result(path);
		CoTaskMemFree(path);
		return result;
	}
#elif defined(PLATFORM_LINUX)
	const String appDataDir = getLinuxAppDataDir();
	if (!appDataDir.empty())
	{
		return appDataDir.toStdWString();
	}
#elif defined(PLATFORM_MAC) || defined(PLATFORM_IOS)
	return mExAppDataPath;
#endif
	return L"";
}

std::wstring PlatformFunctions::tryGetSteamRomPath(const std::wstring& romName)
{
#ifdef PLATFORM_WINDOWS
	const std::wstring steamPath = getSteamInstallationPath();
	if (!steamPath.empty())
	{
		RMX_LOG_INFO("Steam installation found: " << WString(steamPath).toStdString());
		std::vector<WString> searchPaths;
		searchPaths.push_back(steamPath);
		collectSteamInstallFolders(searchPaths, steamPath);
		return *lookForROMFileInSearchPaths(searchPaths, WString(L"\\steamapps\\common\\Sega Classics\\uncompressed ROMs\\") + romName);
	}
	return L"";
#elif defined(PLATFORM_LINUX)
	std::vector<WString> searchPaths;
	const String homeDir = getLinuxHomeDir();
	if (!homeDir.empty())
	{
		searchPaths.push_back(homeDir.toWString() + L"/.local/share/Steam");			// The usual location
		searchPaths.push_back(homeDir.toWString() + L"/.steam/steam");					// Some possible alternative
		searchPaths.push_back(homeDir.toWString() + L"/.steam/root");					// Symlink set on some distros
		searchPaths.push_back(homeDir.toWString() + L"/.steam/debian-installation");	// Another alternative, at least for Debian system
		searchPaths.push_back(homeDir.toWString() + L"/Steam");							// Yet another alternative, no idea if that is or was actually used
		searchPaths.push_back(homeDir.toWString() + L"/.var/app/com.valvesoftware.Steam/.local/share/Steam");	// When having the sandboxed Flatpak version of Steam
	}
	return *lookForROMFileInSearchPaths(searchPaths, WString(L"/steamapps/common/Sega Classics/uncompressed ROMs/") + romName);
#else
	return L"";
#endif
}

std::string PlatformFunctions::getCompactSystemTimeString()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
#if defined(PLATFORM_WINDOWS)
	localtime_s(&tstruct, &now);
#else
	tstruct = *localtime(&now);
#endif
	// Format example: "220629_114248"
	std::strftime(buf, sizeof(buf), "%y%m%d_%H%M%S", &tstruct);
	return buf;
}

void PlatformFunctions::showMessageBox(const std::string& caption, const std::string& text)
{
#ifdef PLATFORM_WINDOWS

	MessageBoxA(nullptr, text.c_str(), caption.c_str(), MB_OK | MB_ICONEXCLAMATION);

#else

	// A more platform-independent version provided by SDL; should be used as a fallback if there's nothing better
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, caption.c_str(), text.c_str(), nullptr);

#endif
}

PlatformFunctions::DialogResult PlatformFunctions::showDialogBox(rmx::ErrorSeverity severity, DialogButtons dialogButtons, const std::string& caption, const std::string& text)
{
#ifdef PLATFORM_WINDOWS

	uint32 type = 0;
	switch (dialogButtons)
	{
		case DialogButtons::OK:			type |= MB_OK;			break;
		case DialogButtons::OK_CANCEL:	type |= MB_OKCANCEL;	break;
		default:						type |= MB_YESNOCANCEL;	break;
	}
	switch (severity)
	{
		case rmx::ErrorSeverity::ERROR:		type |= MB_ICONEXCLAMATION;	break;
		case rmx::ErrorSeverity::WARNING:	type |= MB_ICONWARNING;		break;
		default:							type |= MB_ICONINFORMATION;	break;
	}

	const int result = MessageBoxA(nullptr, text.c_str(), caption.c_str(), type);
	switch (result)
	{
		case IDOK:		return DialogResult::OK;
		case IDABORT:	return DialogResult::NO;
		case IDCANCEL:	return DialogResult::CANCEL;
		case IDIGNORE:	return DialogResult::CANCEL;
		case IDYES:		return DialogResult::OK;
		case IDNO:		return DialogResult::NO;
	}
	return DialogResult::OK;

#else

	// A more platform-independent version provided by SDL
	//  -> Should be used as a fallback if there's nothing better

	const SDL_MessageBoxButtonData buttons_Ok[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "OK" }
	};
	const SDL_MessageBoxButtonData buttons_OkCancel[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel" }
	};
	const SDL_MessageBoxButtonData buttons_YesNoCancel[] =
	{
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes" },
		{ 0,									   1, "No" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel" }
	};
	const SDL_MessageBoxButtonData* buttons = (dialogButtons == DialogButtons::OK) ? buttons_Ok :
											  (dialogButtons == DialogButtons::OK_CANCEL) ? buttons_OkCancel : buttons_YesNoCancel;
	const int numButtons = (dialogButtons == DialogButtons::OK) ? SDL_arraysize(buttons_Ok) :
						   (dialogButtons == DialogButtons::OK_CANCEL) ? SDL_arraysize(buttons_OkCancel) : SDL_arraysize(buttons_YesNoCancel);

	uint32 flags = 0;
	if (severity == rmx::ErrorSeverity::ERROR)
		flags |= SDL_MESSAGEBOX_ERROR;
	else if (severity == rmx::ErrorSeverity::WARNING)
		flags |= SDL_MESSAGEBOX_WARNING;
	else
		flags |= SDL_MESSAGEBOX_INFORMATION;

	const char* textAsCString = text.c_str();
#if defined(PLATFORM_ANDROID)
	std::string shortenedText;
	if (text.length() > 250)
	{
		// Limit text length to avoid it taking too much space so that the buttons get moved out of the screen
		shortenedText = text.substr(0, 250) + "...";
		textAsCString = shortenedText.c_str();
	}
#endif

	const SDL_MessageBoxData messageboxdata = { flags, nullptr, caption.c_str(), textAsCString, numButtons, buttons, nullptr };
	int buttonId = 2;
	SDL_ShowMessageBox(&messageboxdata, &buttonId);		// Ignoring return value
	return (buttonId == 2) ? DialogResult::CANCEL : (buttonId == 1) ? DialogResult::NO : DialogResult::OK;

#endif
}

std::wstring PlatformFunctions::openFileSelectionDialog(const std::wstring& title, const std::wstring& defaultFilename, const wchar_t* filter)
{
#if defined(PLATFORM_WINDOWS)

	// This seems to be needed to prevent "GetOpenFileNameW" from randomly crashing
	HRESULT hresult = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
	(void)hresult;	// Ignore output

	wchar_t buffer[2048];
	memcpy(buffer, defaultFilename.c_str(), (defaultFilename.length() + 1) * sizeof(wchar_t));
	OPENFILENAMEW open;
	ZeroMemory(&open, sizeof(open));
	open.lStructSize = sizeof(OPENFILENAMEW);
	open.lpstrFilter = filter;
	open.nFileOffset = 1;
	open.lpstrFile = buffer;
	open.nMaxFile = 2048;
	open.lpstrTitle = title.c_str();
	open.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_NONETWORKBUTTON;
	GetOpenFileNameW(&open);

	const std::wstring result(buffer);
	CoUninitialize();
	return result;

#else
	// Not implemented
	return L"";
#endif
}

void PlatformFunctions::openFileExternal(const std::wstring& path)
{
#if defined(PLATFORM_WINDOWS)
	::ShellExecuteW(nullptr, nullptr, path.c_str(), nullptr, nullptr, SW_SHOW);
#elif defined(PLATFORM_LINUX)
	// TODO: What if the path has spaces or non-ASCII characters?
	const int result = system(*(String("xdg-open ") + WString(path).toString()));
	RMX_CHECK(result >= 0, "System call failed", );
#endif
}

void PlatformFunctions::openDirectoryExternal(const std::wstring& path)
{
#if defined(PLATFORM_WINDOWS)
	::ShellExecuteW(nullptr, L"open", L"explorer", (L"file://" + path).c_str(), nullptr, SW_SHOW);
#elif defined(PLATFORM_MAC)
	system(*(String("open \"") + WString(path).toString() + "\""));
#elif defined(PLATFORM_LINUX)
	// TODO: What if the path has spaces or non-ASCII characters?
	const int result = system(*(String("xdg-open ") + WString(path).toString()));
	RMX_CHECK(result >= 0, "System call failed", );
#endif
}

void PlatformFunctions::openURLExternal(const std::string& url)
{
#ifdef PLATFORM_WEB
	std::string command = "window.location.href = \"" + url + "\"";
	emscripten_run_script(command.c_str());
#elif SDL_VERSION_ATLEAST(2, 0, 14)
	SDL_OpenURL(url.c_str());
#endif
}

bool PlatformFunctions::isDebuggerPresent()
{
#ifdef PLATFORM_WINDOWS
	return IsDebuggerPresent() != 0;
#else
	return false;
#endif
}
