/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/ScriptBuildWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/modding/Mod.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Module.h>


ScriptBuildWindow::ScriptBuildWindow() :
	DevModeWindowBase("Script Build", Category::SIMULATION, 0)
{
}

void ScriptBuildWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(5.0f, 450.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	Simulation& simulation = Application::instance().getSimulation();
	const LemonScriptProgram& program = simulation.getCodeExec().getLemonScriptProgram();

	if (ImGui::Button("Reload scripts"))
	{
		HighResolutionTimer timer;
		timer.start();
		if (simulation.triggerFullScriptsReload())
		{
			LogDisplay::instance().setLogDisplay(String(0, "Reloaded scripts in %0.2f sec", timer.getSecondsSinceStart()));
		}
	}

	ImGui::Spacing();

	static ImGuiHelpers::FilterString filterString;
	filterString.draw();
	ImGui::SameLine();
	ImGui::Text("Filter by module name or author");

	if (ImGui::BeginTable("Watches Table", 1, ImGuiTableFlags_Borders, ImVec2(0.0f, 0.0f)))
	{
		for (const lemon::Module* module : program.getModules())
		{
			const Mod* mod = program.getModByModule(*module);

			if (!filterString.shouldInclude(module->getModuleName()))
			{
				if (nullptr == mod || !filterString.shouldInclude(mod->mAuthor))
					continue;
			}

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Spacing();

			const ImVec4 textColor = (nullptr == mod) ? ImGuiHelpers::COLOR_WHITE : ImGuiHelpers::COLOR_LIGHT_CYAN;

			ImGui::PushID(module);

			ImGui::PushStyleColor(ImGuiCol_Text, textColor);
			const bool isOpen = ImGui::TreeNodeEx(&module, 0, "%s", module->getModuleName().c_str());
			ImGui::PopStyleColor();

			if (!module->getWarnings().empty())
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d warning%s", module->getWarnings().size(), (module->getWarnings().size() == 1) ? "" : "s");
			}

			if (isOpen)
			{
				if (module->getWarnings().empty())
				{
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.5f), "       No warnings");
				}
				else
				{
					for (const lemon::CompilerWarning& warning : module->getWarnings())
					{
						if (ImGui::TreeNodeEx(&warning, ImGuiTreeNodeFlags_DefaultOpen, "%s", warning.mMessage.c_str()))
						{
							if (PlatformFunctions::hasClipboardSupport())
							{
								// Context menu
								if (ImGui::BeginPopupContextItem())
								{
									if (ImGui::Button("Copy warning text to clipboard"))
									{
										PlatformFunctions::copyToClipboard(warning.mMessage);
										ImGui::CloseCurrentPopup();
									}
									ImGui::EndPopup();
								}
								ImGui::SetItemTooltip("Right-click for options");
							}

							// List of occurences
							for (const lemon::CompilerWarning::Occurrence& occurrence : warning.mOccurrences)
							{
								if (nullptr != occurrence.mSourceFileInfo)
								{
									ImGui::BulletText("In '%s', line %d", WString(occurrence.mSourceFileInfo->mFilename).toStdString().c_str(), occurrence.mLineNumber);
									ImGui::PushID(&occurrence);
									ImGuiHelpers::OpenCodeLocation::drawButton(occurrence.mSourceFileInfo->getFullFilePath(), occurrence.mLineNumber);
									ImGui::PopID();
								}
							}
							ImGui::TreePop();
						}
					}
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
			ImGui::Spacing();
		}

		ImGui::EndTable();
	}
}

#endif
