/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/DebugLogWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


DebugLogWindow::DebugLogWindow() :
	DevModeWindowBase("Debug Log", Category::SIMULATION, 0)
{
}

void DebugLogWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(350.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 200.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	DebugTracking& debugTracking = codeExec.getDebugTracking();

	ImGui::Checkbox("Show old values", &mShowOldValues);
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::Text("Decide whether values not logged in the last frame are shown (grayed out) or hidden.");
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	ImGui::Spacing();

	const auto& entries = debugTracking.getScriptLogEntries();
	if (!entries.empty())
	{
		if (ImGui::BeginTable("Debug Log Table", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f * uiScale);
			ImGui::TableSetupColumn("Logged");

			for (const auto& pair : entries)
			{
				const DebugTracking::ScriptLogEntry& entry = pair.second;
				if (entry.mEntries.empty())
					continue;

				const bool hasCurrent = (entry.mLastUpdate >= Application::instance().getSimulation().getFrameNumber() - 1);
				if (!mShowOldValues && !hasCurrent)
					continue;

				const ImVec4 color = hasCurrent ? ImGuiHelpers::COLOR_WHITE : ImGuiHelpers::COLOR_GRAY60;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextColored(color, "%s", pair.first.c_str());

				ImGui::TableSetColumnIndex(1);
				for (size_t i = 0; i < entry.mEntries.size(); ++i)
				{
					const DebugTracking::ScriptLogSingleEntry& singleEntry = entry.mEntries[i];

					if (!hasCurrent)
					{
						ImGui::Bullet();
						ImGui::PushStyleColor(ImGuiCol_Text, color);
						ImGui::Text("  %s", singleEntry.mValue.c_str());
						ImGui::PopStyleColor();
						continue;
					}

					const uint32 key = (((uint32)entry.mEntries.size() << 16) + ((uint32)i << 24)) ^ (uint32)rmx::getMurmur2_64(singleEntry.mValue) ^ (uint32)(rmx::getMurmur2_64(pair.first) << 1);

					ImGui::PushID(key);
					ImGui::PushStyleColor(ImGuiCol_Text, color);
					const bool isOpen = ImGui::TreeNodeEx("Call Stack", 0, "%s", singleEntry.mValue.c_str());
					ImGui::PopStyleColor();

					if (isOpen)
					{
						ImGui::TextColored(ImGuiHelpers::COLOR_GRAY80, "   Call Stack:");
						std::vector<DebugTracking::Location> callStack;
						debugTracking.getCallStackFromCallFrameIndex(callStack, singleEntry.mCallFrameIndex, singleEntry.mLocation.mProgramCounter);

						for (size_t k = 0; k < callStack.size(); ++k)
						{
							const DebugTracking::Location& loc = callStack[k];
							const std::string& functionName = loc.toString(debugTracking.getCodeExec());
							if (loc.mLineNumber >= 0)
							{
								ImGui::TextColored(ImGuiHelpers::COLOR_GRAY80, "        %s, line %d", functionName.c_str(), loc.mLineNumber);

								if (loc.mProgramCounter.has_value())
								{
									ImGui::PushID((int)k);
									if (ImGuiHelpers::OpenCodeLocation::drawButton())
									{
										LemonScriptProgram::ResolvedLocation location;
										codeExec.getLemonScriptProgram().resolveLocation(location, *loc.mFunction, (uint32)loc.mProgramCounter.value());
										if (nullptr != location.mSourceFileInfo)
										{
											ImGuiHelpers::OpenCodeLocation::open(location.mSourceFileInfo->getFullFilePath(), loc.mLineNumber);
										}
									}
									ImGui::PopID();
								}
							}
							else
							{
								ImGui::TextColored(ImGuiHelpers::COLOR_GRAY80, "        %s", functionName.c_str());
							}
						}

						ImGui::TreePop();
						ImGui::Spacing();
					}
					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}
	}
}

#endif
