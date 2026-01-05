/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/imgui/ImGuiHelpers.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/drawing/opengl/OpenGLDrawerTexture.h"
#include "oxygen/drawing/software/SoftwareDrawerTexture.h"


void ImGuiHelpers::resetForNextFrame()
{
	mActiveInputRect = Recti();
}

ImTextureRef ImGuiHelpers::getTextureRef(DrawerTexture& drawerTexture)
{
	Drawer& drawer = EngineMain::instance().getDrawer();
	if (drawer.getType() == Drawer::Type::OPENGL)
		return ImTextureRef(drawerTexture.getImplementation<OpenGLDrawerTexture>()->getTextureHandle());
	else
		return ImTextureRef(drawerTexture.getUniqueID());
}

ImVec4 ImGuiHelpers::getAccentColorMix(float accent, float saturation, float grayValue)
{
	Color accentColor = Configuration::instance().mDevMode.mUIAccentColor;
	return ImVec4(interpolate(grayValue, accentColor.x * accent, saturation), interpolate(grayValue, accentColor.y * accent, saturation), interpolate(grayValue, accentColor.z * accent, saturation), 1.0f);
}


void ImGuiHelpers::InputString::set(std::string_view str)
{
	const size_t len = std::min(str.length(), sizeof(mInternal) - 1);
	if (!str.empty())
		memcpy(mInternal, str.data(), len);
	mInternal[len] = 0;
}


void ImGuiHelpers::WideInputString::set(std::wstring_view str)
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

void ImGuiHelpers::WideInputString::refreshFromInternal()
{
	mWideString.fromUTF8(std::string_view(mInternalUTF8));
}


bool ImGuiHelpers::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	const bool result = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
	if (ImGui::IsItemActive())
	{
		mActiveInputRect.set(roundToInt(ImGui::GetItemRectMin().x), roundToInt(ImGui::GetItemRectMin().y), roundToInt(ImGui::GetItemRectSize().x), roundToInt(ImGui::GetItemRectSize().y));
	}
	return result;
}

bool ImGuiHelpers::InputText(const char* label, ImGuiHelpers::InputString& inputString, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	const bool result = ImGui::InputText(label, inputString.mInternal, sizeof(inputString.mInternal), flags, callback, user_data);
	if (ImGui::IsItemActive())
	{
		mActiveInputRect.set(roundToInt(ImGui::GetItemRectMin().x), roundToInt(ImGui::GetItemRectMin().y), roundToInt(ImGui::GetItemRectSize().x), roundToInt(ImGui::GetItemRectSize().y));
	}
	return result;
}

bool ImGuiHelpers::InputText(const char* label, ImGuiHelpers::WideInputString& inputString, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	const bool result = ImGui::InputText(label, inputString.mInternalUTF8, sizeof(inputString.mInternalUTF8), flags, callback, user_data);
	if (result)
	{
		inputString.refreshFromInternal();
	}
	if (ImGui::IsItemActive())
	{
		mActiveInputRect.set(roundToInt(ImGui::GetItemRectMin().x), roundToInt(ImGui::GetItemRectMin().y), roundToInt(ImGui::GetItemRectSize().x), roundToInt(ImGui::GetItemRectSize().y));
	}
	return result;
}


bool ImGuiHelpers::FilterString::draw()
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

bool ImGuiHelpers::FilterString::shouldInclude(std::string_view str) const
{
	if (mString[0] == 0)
		return true;
	return rmx::containsCaseInsensitive(str, mString);
}


bool ImGuiHelpers::OpenCodeLocation::drawButton()
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

bool ImGuiHelpers::OpenCodeLocation::drawButton(const std::wstring& path, int lineNumber)
{
	if (drawButton())
		return open(path, lineNumber);
	return false;
}

bool ImGuiHelpers::OpenCodeLocation::open(const std::wstring& path, int lineNumber)
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

#endif