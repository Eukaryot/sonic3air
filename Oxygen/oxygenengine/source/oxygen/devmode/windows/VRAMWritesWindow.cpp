/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/VRAMWritesWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


VRAMWritesWindow::VRAMWritesWindow() :
	DevModeWindowBase("VRAM Writes", Category::DEBUGGING, 0)
{
}

void VRAMWritesWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(500.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);

	const float uiScale = ImGui::GetIO().FontGlobalScale;
	const ImVec4 lightGrayColor(0.75f, 0.75f, 0.75f, 1.0f);

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();
	DebugTracking& debugTracking = codeExec.getDebugTracking();

	const PlaneManager& planeManager = VideoOut::instance().getRenderParts().getPlaneManager();
	const uint16 startAddressPlaneA = planeManager.getPlaneBaseVRAMAddress(PlaneManager::PLANE_A);
	const uint16 startAddressPlaneB = planeManager.getPlaneBaseVRAMAddress(PlaneManager::PLANE_B);
	const uint16 endAddressPlaneA = startAddressPlaneA + (uint16)planeManager.getPlaneSizeInVRAM(PlaneManager::PLANE_A);
	const uint16 endAddressPlaneB = startAddressPlaneB + (uint16)planeManager.getPlaneSizeInVRAM(PlaneManager::PLANE_B);
	const ScrollOffsetsManager& scrollOffsetsManager = VideoOut::instance().getRenderParts().getScrollOffsetsManager();
	const uint16 startAddressScrollOffsets = scrollOffsetsManager.getHorizontalScrollTableBase();
	const uint16 endAddressScrollOffsets = startAddressScrollOffsets + 0x200;

	// Create a sorted list
	std::vector<DebugTracking::VRAMWrite*> writes = debugTracking.getVRAMWrites();
	std::sort(writes.begin(), writes.end(), [](const DebugTracking::VRAMWrite* a, const DebugTracking::VRAMWrite* b) { return a->mAddress < b->mAddress; } );

	ImGui::Checkbox("Plane A", &mShowPlaneA);
	ImGui::Checkbox("Plane B", &mShowPlaneB);
	ImGui::Checkbox("Scroll Offsets", &mShowScroll);
	ImGui::Checkbox("Patterns and others", &mShowOthers);
	ImGui::Spacing();

	if (ImGui::BeginTable("VRAM Writes Table", 1, ImGuiTableFlags_Borders, ImVec2(0.0f, 0.0f)))
	{
		for (const DebugTracking::VRAMWrite* write : writes)
		{
			const uint64 key = ((uint64)write->mAddress << 32) + write->mSize;
			String line(0, "0x%04x (0x%02x bytes) at %s", write->mAddress, write->mSize, write->mLocation.toString(codeExec).c_str());
			ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
			if (write->mAddress >= startAddressPlaneA && write->mAddress < endAddressPlaneA)
			{
				if (!mShowPlaneA)
					continue;
				line = String("[Plane A] ") + line;
				color = ImVec4(1.0f, 1.0f, 0.75f, 1.0f);
			}
			else if (write->mAddress >= startAddressPlaneB && write->mAddress < endAddressPlaneB)
			{
				if (!mShowPlaneB)
					continue;
				line = String("[Plane B] ") + line;
				color = ImVec4(1.0f, 0.75f, 1.0f, 1.0f);
			}
			else if (write->mAddress >= startAddressScrollOffsets && write->mAddress < endAddressScrollOffsets)
			{
				if (!mShowScroll)
					continue;
				line = String("[Scroll Offsets] ") + line;
				color = ImVec4(0.75f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				if (!mShowOthers)
					continue;
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImGui::PushID(write);

			const bool openTree = ImGui::TreeNodeEx("##", 0);
			ImGui::SameLine();
			ImGui::TextColored(color, "%s", *line);

			if (openTree)
			{
				ImGui::TextColored(lightGrayColor, "   Call Stack:");
				std::vector<DebugTracking::Location> callStack;
				debugTracking.getCallStackFromCallFrameIndex(callStack, write->mCallFrameIndex, write->mLocation.mProgramCounter);

				for (size_t k = 0; k < callStack.size(); ++k)
				{
					const DebugTracking::Location& loc = callStack[k];
					const std::string& functionName = loc.toString(debugTracking.getCodeExec());
					if (loc.mLineNumber >= 0)
					{
						ImGui::TextColored(lightGrayColor, "        %s, line %d", functionName.c_str(), loc.mLineNumber);

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
						ImGui::TextColored(lightGrayColor, "        %s", functionName.c_str());
					}
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (writes.size() == debugTracking.getVRAMWrites().capacity())
		{
			ImGui::Text("[Reached the limit]");
		}

		ImGui::EndTable();
	}
}

#endif
