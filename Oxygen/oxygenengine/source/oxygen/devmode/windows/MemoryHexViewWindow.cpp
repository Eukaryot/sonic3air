/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/MemoryHexViewWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/devmode/windows/DevModeMainWindow.h"
#include "oxygen/application/Application.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/Simulation.h"


MemoryHexViewWindow::MemoryHexViewWindow() :
	DevModeWindowBase("Memory Hex View", Category::DEBUGGING, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void MemoryHexViewWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(50.0f, 450.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(520.0f, 270.0f), ImGuiCond_FirstUseEver);

	const float uiScale = ImGui::GetIO().FontGlobalScale;

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();
	DebugTracking& debugTracking = codeExec.getDebugTracking();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Number of rows:");
	{
		#define ADD_ROWS_OPTION(__num__)	\
			ImGui::SameLine();				\
			if (ImGui::Button(#__num__))	\
				mNumRows = __num__;
		ADD_ROWS_OPTION(1);
		ADD_ROWS_OPTION(2);
		ADD_ROWS_OPTION(4);
		ADD_ROWS_OPTION(8);
		ADD_ROWS_OPTION(16);
		#undef ADD_ROWS_OPTION
	}

	ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
	if (ImGui::ArrowButton("Up", ImGuiDir_Up))
	{
		mStartAddress -= 16;
	}
	ImGui::SameLine();
	if (ImGui::ArrowButton("Down", ImGuiDir_Down))
	{
		mStartAddress += 16;
	}
	ImGui::PopItemFlag();

	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Start Address:");
	ImGui::SameLine();
	ImGui::InputScalar("##StartAddress", ImGuiDataType_U32, &mStartAddress, nullptr, nullptr, "%08x", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank);
	ImGui::Spacing();

	if (ImGui::BeginTable("Hex View", 17, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoSavedSettings))
	{
		uint32 address = mStartAddress;
		static uint32 hoveredAddress = 0xffffffff;
		uint32 newHoveredAddress = 0xffffffff;

		const ImVec4 cellColorTitle     = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
		const uint32 cellBGColorTitle   = ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		const uint32 cellBGColorNormal  = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
		const uint32 cellBGColorHovered = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.0f, 0.6f) );

		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 70.0f * uiScale);
		for (int column = 1; column <= 16; ++column)
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 15.0f * uiScale);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cellBGColorTitle);
		ImGui::TextColored(cellColorTitle, "Address");
		for (int column = 1; column <= 16; ++column)
		{
			ImGui::TableSetColumnIndex(column);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cellBGColorTitle);
			ImGui::TextColored(cellColorTitle, "%02x", column - 1);
		}

		for (int row = 0; row < mNumRows; ++row)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cellBGColorNormal);
			ImGui::Text("%08x", address);

			for (int column = 1; column <= 16; ++column)
			{
				const float brightness = 1.0f - (address % 4) * 0.15f;
				const ImVec4 color(brightness, brightness, brightness * ((row % 2) ? 0.6f : 1.0f), 1.0f);

				ImGui::TableSetColumnIndex(column);

				const uint8 memoryBank = (address >> 16) & 0xff;
				const bool isReadableAddress = (memoryBank < 0x40 || (memoryBank & 0xf0) == 0x80 || memoryBank >= 0xff);
				if (isReadableAddress)
				{
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, (hoveredAddress == address) ? cellBGColorHovered : cellBGColorNormal);
					ImGui::TextColored(color, "%02x", emulatorInterface.readMemory8(address));

					if (ImGui::IsItemClicked())
					{
						mClickedAddress = address;
					}
					if (ImGui::IsItemHovered())
					{
						newHoveredAddress = address;
						ImGui::SetItemTooltip("  u8[0x%08x] = 0x%02x\nu16[0x%08x] = 0x%04x\nu32[0x%08x] = 0x%08x", address, emulatorInterface.readMemory8(address), address, emulatorInterface.readMemory16(address), address, emulatorInterface.readMemory32(address));
					}
				}
				else
				{
					ImGui::TextColored(color, "---");
				}

				++address;
			}
		}
		ImGui::EndTable();

		hoveredAddress = newHoveredAddress;
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGuiHelpers::ScopedIndent si;

	if (mClickedAddress == 0xffffffff)
	{
		ImGui::Text("Click any address to show here");
	}
	else
	{
		if (ImGui::BeginTable("Clicked Address", 3, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp, ImVec2(300.0f * uiScale, 0.0f)))
		{
			for (int size = 0; size < 3; ++size)
			{
				ImGui::TableNextRow();
				ImGui::PushID(size);
				const uint8 bytes = 1 << size;

				ImGui::TableSetColumnIndex(0);
				switch (size)
				{
					case 0:  ImGui::Text("  u8[0x%08x]", mClickedAddress); break;
					case 1:  ImGui::Text("u16[0x%08x]", mClickedAddress);  break;
					case 2:  ImGui::Text("u32[0x%08x]", mClickedAddress);  break;
				}

				ImGui::TableSetColumnIndex(1);
				switch (size)
				{
					case 0:  ImGui::Text("0x%02x", emulatorInterface.readMemory8(mClickedAddress));  break;
					case 1:  ImGui::Text("0x%04x", emulatorInterface.readMemory16(mClickedAddress));  break;
					case 2:  ImGui::Text("0x%08x", emulatorInterface.readMemory32(mClickedAddress));  break;
				}

				ImGui::TableSetColumnIndex(2);
				if (debugTracking.hasWatch(mClickedAddress, bytes))
				{
					if (ImGui::SmallButton("Remove Watch"))
					{
						debugTracking.removeWatch(mClickedAddress, bytes);
					}
				}
				else
				{
					if (ImGui::SmallButton("Add Watch"))
					{
						debugTracking.addWatch(mClickedAddress, bytes, true);
						mDevModeMainWindow->openWatchesWindow();
					}
				}
				ImGui::PopID();

			}
			ImGui::EndTable();
		}
	}
}

#endif
