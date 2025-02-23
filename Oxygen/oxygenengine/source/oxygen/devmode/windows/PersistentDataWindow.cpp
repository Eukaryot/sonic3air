/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/PersistentDataWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/simulation/PersistentData.h"


PersistentDataWindow::PersistentDataWindow() :
	DevModeWindowBase("Persistent Data", Category::MISC, 0)
{
}

void PersistentDataWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(200.0f, 450.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 200.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	const PersistentData& persistentData = PersistentData::instance();

	// TODO: Add some caching

	for (const auto& pair : persistentData.getFiles())
	{
		const PersistentData::File& file = pair.second;

		const bool isOpen = ImGui::TreeNodeEx(&file, 0, "%s (%d %s)", file.mFilePath.c_str(), file.mEntries.size(), (file.mEntries.size() == 1) ? "entry" : "entries");
		if (isOpen)
		{
			for (const PersistentData::Entry& entry : file.mEntries)
			{
				ImGui::BulletText("%s:   %d bytes", entry.mKey.c_str(), entry.mData.size());
			}

			ImGui::TreePop();
		}
	}

}

#endif
