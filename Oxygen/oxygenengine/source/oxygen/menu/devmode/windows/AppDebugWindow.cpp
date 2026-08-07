/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/AppDebugWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/imgui/ImGuiHelpers.h"


AppDebugWindow::AppDebugWindow() :
	DevModeWindowBase("App Debug", Category::DEBUGGING, 0)
{
}

void AppDebugWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(500.0f, 50.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 200.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	// ImGui demo window for testing
	ImGui::Checkbox("ImGui Demo", &mShowImGuiDemo);
	if (mShowImGuiDemo)
	{
		ImGui::ShowDemoWindow();
	}

	ImGui::Spacing();

	// Output of GuiBase hierarchy
	if (ImGui::CollapsingHeader("GuiBase hierarchy"))
	{
		std::vector<std::pair<GuiBase*, int>> stack;
		stack.emplace_back(&FTX::System->getRoot(), 0);

		while (!stack.empty())
		{
			GuiBase& element = *stack.back().first;
			const int level = stack.back().second;
			stack.pop_back();

			if (level > 0)	// Ignore root
			{
				ImGuiHelpers::ScopedIndent si(level * 20.0f - 18.0f);
				ImGui::BulletText("%s", element.getInternalName().c_str());
			}

			for (int k = (int)element.getChildren().size() - 1; k >= 0; --k)
			{
				stack.emplace_back(element.getChildren()[k], level + 1);
			}
		}
	}
}

#endif
