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
#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"
#include "oxygen/drawing/software/SoftwareDrawerTexture.h"


namespace ImGuiHelpers
{

	ImTextureRef getTextureRef(DrawerTexture& drawerTexture)
	{
		Drawer& drawer = EngineMain::instance().getDrawer();
		if (drawer.getType() == Drawer::Type::OPENGL)
			return ImTextureRef(drawerTexture.getImplementation<OpenGLDrawerTexture>()->getTextureHandle());
		else
			return ImTextureRef(drawerTexture.getUniqueID());
	}


	void InputString::set(std::string_view str)
	{
		const size_t len = std::min(str.length(), sizeof(mInternal) - 1);
		if (!str.empty())
			memcpy(mInternal, str.data(), len);
		mInternal[len] = 0;
	}


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


namespace ImGui
{
	void SameLineNoSpace()
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6);
	}

	bool RedButton(const char* label, const ImVec2& size)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
		const bool result = ImGui::Button(label, size);
		ImGui::PopStyleColor(3);
		return result;
	}
}

#endif
