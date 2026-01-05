/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/imgui/ImGuiExtensions.h"

#if defined(SUPPORT_IMGUI)

namespace ImGui
{
	void SameLineNoSpace()
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 6 * Configuration::instance().mDevMode.mUIScale);
	}

	void SameLineRelSpace(float relativeSpace)
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (relativeSpace - 1.0f) * 6 * Configuration::instance().mDevMode.mUIScale);
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
