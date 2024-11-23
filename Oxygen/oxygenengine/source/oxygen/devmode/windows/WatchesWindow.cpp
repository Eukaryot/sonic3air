/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/WatchesWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


WatchesWindow::WatchesWindow() :
	DevModeWindowBase("Watches")
{
}

void WatchesWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(50.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(400.0f, 200.0f), ImGuiCond_FirstUseEver);

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();
	DebugTracking& debugTracking = codeExec.getDebugTracking();

	const ImVec4 grayColor(0.6f, 0.6f, 0.6f, 1.0f);
	const ImVec4 lightGrayColor(0.75f, 0.75f, 0.75f, 1.0f);
	uint64 nextHoveredKey = ~(uint64)0;

	const std::vector<DebugTracking::Watch*>& watches = debugTracking.getWatches();
	if (watches.empty())
	{
		ImGui::Text("No watches set");
	}
	else
	{
		for (const DebugTracking::Watch* watch : watches)
		{
			// Display 0xffff???? instead of 0x00ff????
			uint32 displayAddress = watch->mAddress;
			if ((displayAddress & 0x00ff0000) == 0x00ff0000)
				displayAddress |= 0xff000000;

			String watchTitle;
			if (watch->mBytes <= 4)
				watchTitle = String(0, "u%d[0x%08x]", watch->mBytes * 8, displayAddress);
			else
				watchTitle = String(0, "0x%02x bytes at 0x%08x", watch->mBytes, displayAddress);

			if (ImGui::TreeNodeEx(*watchTitle, ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (watch->mBytes <= 4)
				{
					if (watch->mHits.empty())
						ImGui::TextColored(grayColor, "       %s   (no changes this frame)", rmx::hexString(watch->mInitialValue, watch->mBytes * 2).c_str());
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

					ImGui::PushID(hitIndex);
					if (ImGui::TreeNodeEx("Call Stack", 0, *hitTitle))
					{
						ImGui::TextColored(lightGrayColor, "   Call Stack:");
						std::vector<DebugTracking::Location> callStack;
						debugTracking.getCallStackFromCallFrameIndex(callStack, hit.mCallFrameIndex, hit.mLocation.mProgramCounter);

						for (size_t k = 0; k < callStack.size(); ++k)
						{
							const DebugTracking::Location& loc = callStack[k];
							const std::string& functionName = loc.toString(debugTracking.getCodeExec());
							if (loc.mLineNumber >= 0)
							{
								ImGui::TextColored(lightGrayColor, "        %s, line %d", functionName.c_str(), loc.mLineNumber);

							#if defined(PLATFORM_WINDOWS)
								if (loc.mProgramCounter.has_value())
								{
									ImGui::SameLine();
									ImGui::PushID((int)k);
									if (ImGui::SmallButton("VC"))
									{
										LemonScriptProgram::ResolvedLocation location;
										codeExec.getLemonScriptProgram().resolveLocation(location, *loc.mFunction, (uint32)loc.mProgramCounter.value());
										if (location.mSourceFileInfo)
										{
											// TODO: Add the actual full path to Visual Studio Code
											//  -> This should be configurable in the settings.json - and if it's not set, don't show the VC button in the first place
											std::wstring applicationPath = Configuration::instance().mAppDataPath + L"../../Local/Programs/Microsoft VS Code/Code.exe";

											// TODO: The full path here is not necessarily actually the full path, but can be a relative one
											std::wstring arguments = L"-r -g \"" + location.mSourceFileInfo->mFullPath + L"\":" + std::to_wstring(loc.mLineNumber);

											PlatformFunctions::openApplicationExternal(applicationPath, arguments);
										}
									}
									ImGui::PopID();
								}
							#endif
							}
							else
							{
								ImGui::TextColored(lightGrayColor, "        %s", functionName.c_str());
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
	}
}

#endif
