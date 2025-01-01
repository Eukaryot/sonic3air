/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/ImGuiHelpers.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/platform/PlatformFunctions.h"


namespace ImGuiHelpers
{

	void WideInputString::set(std::wstring_view str)
	{
		if (str == mWideString)
			return;

		mWideString = str;
		const String utf8 = mWideString.toUTF8();
		if (utf8.length() < 255)
		{
			memcpy(mInternalUTF8, *utf8, utf8.length() + 1);
		}
		else
		{
			memcpy(mInternalUTF8, *utf8, 255);
			mInternalUTF8[255] = 0;
		}
	}

	void WideInputString::refreshFromInternal()
	{
		mWideString.fromUTF8(std::string_view(mInternalUTF8));
	}


	bool FilterString::draw()
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Filter:");
		ImGui::SameLine();
		const bool result = ImGui::InputText("##Filter", mString, 256, 0);

		ImGui::BeginDisabled(mString[0] == 0);
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
			mString[0] = 0;
		ImGui::EndDisabled();
		return result;
	}

	bool FilterString::shouldInclude(std::string_view str) const
	{
		if (mString[0] == 0)
			return true;
		return rmx::containsCaseInsensitive(str, mString);
	}


	bool OpenCodeLocation::drawButton()
	{
		const Configuration::ExternalCodeEditor& config = Configuration::instance().mDevMode.mExternalCodeEditor;
		if (config.mActiveType.empty())
			return false;

	#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
		ImGui::SameLine();
		return ImGui::SmallButton("Open");
	#else
		// Not implemented
		return false;
	#endif
	}

	bool OpenCodeLocation::drawButton(const std::wstring& path, int lineNumber)
	{
		if (drawButton())
			return open(path, lineNumber);
		return false;
	}

	bool OpenCodeLocation::open(const std::wstring& path, int lineNumber)
	{
		const Configuration::ExternalCodeEditor& config = Configuration::instance().mDevMode.mExternalCodeEditor;

		std::wstring execPath;
		std::wstring_view argumentsFormat;
		if (config.mActiveType == "vscode")
		{
			execPath = config.mVisualStudioCodePath;
			argumentsFormat = L"-r -g \"{file}\":{line}";
		}
		else if (config.mActiveType == "npp")
		{
			execPath = config.mNotepadPlusPlusPath;
			argumentsFormat = L"\"{file}\" -n{line}";
		}
		else
		{
			execPath = config.mCustomEditorPath;
			argumentsFormat = config.mCustomEditorArgs;
		}

		if (!execPath.empty() && FTX::FileSystem->isFile(execPath))
		{
			WString arguments = argumentsFormat;
			arguments.replace(L"{file}", path);
			arguments.replace(L"{line}", std::to_wstring(lineNumber));
			return PlatformFunctions::openApplicationExternal(execPath, arguments.toStdWString());
		}
		return false;
	}

}

#endif
