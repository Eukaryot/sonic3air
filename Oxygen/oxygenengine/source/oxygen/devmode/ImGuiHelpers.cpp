/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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

	void FilterString::draw()
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Filter:");
		ImGui::SameLine();
		ImGui::InputText("##Filter", mString, 256, 0);
		const bool disabled = (mString[0] == 0);
		if (disabled)
			ImGui::BeginDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
			mString[0] = 0;
		if (disabled)
			ImGui::EndDisabled();
	}

	bool FilterString::shouldInclude(std::string_view str) const
	{
		if (mString[0] == 0)
			return true;
		return rmx::containsCaseInsensitive(str, mString);
	}


	bool OpenCodeLocation::drawButton()
	{
	#if defined(PLATFORM_WINDOWS)
		ImGui::SameLine();
		return ImGui::SmallButton("VC");
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
	#if defined(PLATFORM_WINDOWS)
		// TODO: Add the actual full path to Visual Studio Code
		//  -> This should be configurable in the settings.json - and if it's not set, don't show the VC button in the first place
		std::wstring applicationPath = Configuration::instance().mAppDataPath + L"../../Local/Programs/Microsoft VS Code/Code.exe";

		// TODO: The full path here is not necessarily actually the full path, but can be a relative one
		// TODO: Also, the path might be inside a zip file
		std::wstring arguments = L"-r -g \"" + path + L"\":" + std::to_wstring(lineNumber);

		return PlatformFunctions::openApplicationExternal(applicationPath, arguments);
	#else
		// Not implemented
		return false;
	#endif
	}

}

#endif
