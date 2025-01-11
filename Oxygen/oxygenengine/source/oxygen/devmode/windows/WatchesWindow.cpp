/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/WatchesWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/devmode/DevModeMainWindow.h"
#include "oxygen/application/Application.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


WatchesWindow::WatchesWindow() :
	DevModeWindowBase("Watches", Category::SIMULATION, 0)
{
}

void WatchesWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(50.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();
	DebugTracking& debugTracking = codeExec.getDebugTracking();

	if (ImGui::CollapsingHeader("Add Watch"))
	{
		ImGuiHelpers::ScopedIndent si;

		static ImGuiHelpers::FilterString filterString;
		filterString.draw();

		if (ImGui::BeginTable("Global Defines Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 150.0f * uiScale)))
		{
			ImGui::TableSetupColumn("Identifier");
			ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 100 * uiScale);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80 * uiScale);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 92 * uiScale);

			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			const std::vector<LemonScriptProgram::GlobalDefine>& globalDefines = codeExec.getLemonScriptProgram().getGlobalDefines();
			for (const LemonScriptProgram::GlobalDefine& globalDefine : globalDefines)
			{
				if (!filterString.shouldInclude(globalDefine.mName.getString()))
					continue;

				ImGui::PushID(&globalDefine);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%.*s", (int)globalDefine.mName.getString().length(), globalDefine.mName.getString().data());

				ImGui::TableSetColumnIndex(1);
				if (globalDefine.mAddress <= 0xffffff)
					ImGui::Text("%su%d[0x%06x]", (globalDefine.mBytes == 1) ? "  " : "", globalDefine.mBytes * 8, globalDefine.mAddress);
				else
					ImGui::Text("%su%d[0x%08x]", (globalDefine.mBytes == 1) ? "  " : "", globalDefine.mBytes * 8, globalDefine.mAddress);

				ImGui::TableSetColumnIndex(2);
				switch (globalDefine.mBytes)
				{
					case 1:
					{
						const uint8 value = emulatorInterface.readMemory8(globalDefine.mAddress);
						ImGui::Text("0x%02x", value);
						break;
					}
					case 2:
					{
						const uint16 value = emulatorInterface.readMemory16(globalDefine.mAddress);
						ImGui::Text("0x%04x", value);
						break;
					}
					case 4:
					{
						const uint32 value = emulatorInterface.readMemory32(globalDefine.mAddress);
						ImGui::Text("0x%08x", value);
						break;
					}
				}

				ImGui::TableSetColumnIndex(3);
				if (debugTracking.hasWatch(globalDefine.mAddress, globalDefine.mBytes))
				{
					if (ImGui::SmallButton("Remove Watch"))
					{
						debugTracking.removeWatch(globalDefine.mAddress, globalDefine.mBytes);
					}
				}
				else
				{
					if (ImGui::SmallButton("Add Watch"))
					{
						debugTracking.addWatch(globalDefine.mAddress, globalDefine.mBytes, true, globalDefine.mName.getString());
						mDevModeMainWindow->openWatchesWindow();
					}
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	ImGui::Spacing();

	uint64 nextHoveredKey = ~(uint64)0;

	const std::vector<DebugTracking::Watch*>& watches = debugTracking.getWatches();
	if (watches.empty())
	{
		ImGui::Text("No watches set");
	}
	else
	{
		const DebugTracking::Watch* watchToRemove = nullptr;

		if (ImGui::BeginTable("Watches Table", 1, ImGuiTableFlags_Borders, ImVec2(0.0f, 0.0f)))
		{
			for (const DebugTracking::Watch* watch : watches)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Spacing();		// TODO: Can this be achieved with setting the padding right somewhere instead?

				// Display 0xffff???? instead of 0x00ff????
				uint32 displayAddress = watch->mAddress;
				if ((displayAddress & 0x00ff0000) == 0x00ff0000)
					displayAddress |= 0xff000000;

				String watchTitle;
				if (watch->mBytes <= 4)
					watchTitle = String(0, "u%d[0x%08x]", watch->mBytes * 8, displayAddress);
				else
					watchTitle = String(0, "0x%02x bytes at 0x%08x", watch->mBytes, displayAddress);
				if (!watch->mName.empty())
					watchTitle = String(watch->mName) + " -- " + watchTitle;

				if (ImGui::TreeNodeEx(*watchTitle, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SameLine();
					ImGui::Text("  ");
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove"))
					{
						watchToRemove = watch;
					}

					if (watch->mBytes <= 4)
					{
						if (watch->mHits.empty())
							ImGui::TextColored(ImGuiHelpers::COLOR_GRAY60, "       %s   (no changes this frame)", rmx::hexString(watch->mInitialValue, watch->mBytes * 2).c_str());
						else
							ImGui::Text("       %s   initially", rmx::hexString(watch->mInitialValue, watch->mBytes * 2).c_str());
					}

					for (size_t hitIndex = 0; hitIndex < watch->mHits.size(); ++hitIndex)
					{
						const auto& hit = *watch->mHits[hitIndex];
						const uint64 key = ((uint64)watch->mAddress << 32) + hitIndex;

						String hitTitle;
						if (watch->mBytes <= 4)
						{
							const std::string& functionName = hit.mLocation.toString(codeExec);
							hitTitle = String(0, "%s   at %s, line %d", rmx::hexString(hit.mWrittenValue, watch->mBytes * 2).c_str(), functionName.c_str(), hit.mLocation.mLineNumber);
						}
						else
						{
							hitTitle = String(0, "u%d[0xffff%04x] = %s   at %s", hit.mBytes * 8, hit.mAddress, rmx::hexString(hit.mWrittenValue, std::min(hit.mBytes * 2, 8)).c_str(), hit.mLocation.toString(codeExec).c_str());
						}

						ImGui::PushID((int)hitIndex);
						if (ImGui::TreeNodeEx("Call Stack", 0, "%s", *hitTitle))
						{
							ImGui::TextColored(ImGuiHelpers::COLOR_GRAY80, "   Call Stack:");
							std::vector<DebugTracking::Location> callStack;
							debugTracking.getCallStackFromCallFrameIndex(callStack, hit.mCallFrameIndex, hit.mLocation.mProgramCounter);

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
						}
						ImGui::PopID();
					}
					ImGui::TreePop();
				}
				ImGui::Spacing();
			}
			ImGui::EndTable();
		}

		if (nullptr != watchToRemove)
		{
			debugTracking.removeWatch(watchToRemove->mAddress, watchToRemove->mBytes);
		}
	}
}

#endif
