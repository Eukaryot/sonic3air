/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/CustomSidePanelWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/overlays/DebugSidePanel.h"
#include "oxygen/application/overlays/DebugSidePanelCategory.h"


CustomSidePanelWindow::CustomSidePanelWindow() :
	DevModeWindowBase("Custom Pages", Category::MISC, 0)
{
}

bool CustomSidePanelWindow::shouldBeAvailable()
{
	return !Application::instance().getDebugSidePanel()->getCustomCategories().empty();
}

void CustomSidePanelWindow::onChangedIsWindowOpen(bool open)
{
	if (!open)
	{
		// Make sure all categories are set to invisible
		const std::vector<CustomDebugSidePanelCategory*>& categories = Application::instance().getDebugSidePanel()->getCustomCategories();
		for (CustomDebugSidePanelCategory* category : categories)
		{
			category->setVisibleInDevModeWindow(false);
		}
	}
}

void CustomSidePanelWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(700.0f, 100.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(350.0f, 500.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	const std::vector<CustomDebugSidePanelCategory*>& categories = Application::instance().getDebugSidePanel()->getCustomCategories();

	if (ImGui::BeginTabBar("Tab Bar", ImGuiTabBarFlags_AutoSelectNewTabs))
	{
		for (CustomDebugSidePanelCategory* category : categories)
		{
			if (ImGui::BeginTabItem(category->mHeader.c_str(), nullptr, 0))
			{
				if (ImGui::BeginChild("Child"))
				{
					// Category tab content
					category->setVisibleInDevModeWindow(true);

					ImGui::Spacing();

					// Add checkboxes for the options
					if (!category->mOptions.empty())
					{
						for (CustomDebugSidePanelCategory::Option& option : category->mOptions)
						{
							if (!option.mUpdated)
								continue;

							option.mChecked = category->mOpenKeys.count(option.mKey);
							if (ImGui::Checkbox(option.mText.c_str(), &option.mChecked))
							{
								if (option.mChecked)
									category->mOpenKeys.insert(option.mKey);
								else
									category->mOpenKeys.erase(option.mKey);
							}
						}
						ImGui::Spacing();
						ImGui::Spacing();
					}

					// Sort entries if needed
					if (category->mEntriesNeedSorting)
					{
						std::sort(category->mEntries.begin(), category->mEntries.end(), [](CustomDebugSidePanelCategory::Entry& a, CustomDebugSidePanelCategory::Entry& b) { return a.mKey < b.mKey; } );
						category->mEntriesNeedSorting = false;
					}

					// Add entries
					if (ImGui::BeginTable("Entries", 1, ImGuiTableFlags_BordersH | ImGuiTableFlags_HighlightHoveredColumn, ImVec2(0, 0)))
					{
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 2.0f));

						for (CustomDebugSidePanelCategory::Entry& entry : category->mEntries)
						{
							if (!entry.mUpdated || entry.mLines.empty())
								continue;

							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(0);

							const ImVec2 startPos = ImGui::GetCursorScreenPos();

							ImGui::Spacing();
							ImGui::Spacing();

							if (entry.mCanBeHovered && entry.mIsHovered)
							{
								const uint32 cellBGColorHovered = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_TabHovered));
								ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cellBGColorHovered);
							}

							for (CustomDebugSidePanelCategory::Line& line : entry.mLines)
							{
								ImGui::Indent((float)(line.mIndent + 6));
								const ImVec4 color(line.mColor.r, line.mColor.g, line.mColor.b, line.mColor.a);
								ImGui::TextColored(color, "%s", line.mText.c_str());
								ImGui::Unindent((float)(line.mIndent + 6));
							}

							ImGui::Spacing();
							ImGui::Spacing();

							// Check if mouse cursor is inside table cell
							ImVec2 endPos = ImGui::GetCursorScreenPos();
							endPos.x += ImGui::GetColumnWidth();
							//ImGui::GetWindowDrawList()->AddRectFilled(startPos, endPos, 0x400080ff);	// Just for debugging
							entry.mIsHovered = ImGui::IsMouseHoveringRect(startPos, endPos);
						}
						ImGui::PopStyleVar();

						ImGui::EndTable();
					}
					ImGui::EndChild();
				}

				ImGui::EndTabItem();
			}
			else
			{
				category->setVisibleInDevModeWindow(false);
			}
		}
		ImGui::EndTabBar();
	}
}

#endif
