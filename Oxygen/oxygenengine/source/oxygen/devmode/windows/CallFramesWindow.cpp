/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/CallFramesWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


CallFramesWindow::CallFramesWindow() :
	DevModeWindowBase("Call Frames", Category::SIMULATION, 0)
{
}

void CallFramesWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(500.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();

	codeExec.processCallFrames();
	const auto& callFrames = codeExec.getCallFrames();

	ImGui::Checkbox("Show all hit functions", &mShowAllHitFunctions);

	if (mShowAllHitFunctions)
	{
		ImGui::Checkbox("Sort by filename", &mVisualizationSorting);
		ImGui::Spacing();
		ImGui::Spacing();

		std::map<uint32, const lemon::Function*> functions;
		for (const CodeExec::CallFrame& callFrame : callFrames)
		{
			if (nullptr != callFrame.mFunction)
			{
				const uint32 key = (callFrame.mAddress != 0xffffffff) ? callFrame.mAddress : (0x80000000 + callFrame.mFunction->getID());
				functions.emplace(key, callFrame.mFunction);
			}
		}

		std::vector<std::pair<uint32, const lemon::Function*>> sortedFunctions;
		for (const auto& pair : functions)
		{
			sortedFunctions.emplace_back(pair);
		}
		std::sort(sortedFunctions.begin(), sortedFunctions.end(),
			[&](const std::pair<uint32, const lemon::Function*>& a, const std::pair<uint32, const lemon::Function*>& b)
		{
			if (mVisualizationSorting)
			{
				const std::wstring& filenameA = (a.second->getType() == lemon::Function::Type::SCRIPT) ? static_cast<const lemon::ScriptFunction*>(a.second)->mSourceFileInfo->mFilename : L"";
				const std::wstring& filenameB = (b.second->getType() == lemon::Function::Type::SCRIPT) ? static_cast<const lemon::ScriptFunction*>(b.second)->mSourceFileInfo->mFilename : L"";
				if (filenameA != filenameB)
				{
					return (filenameA < filenameB);
				}
			}
			if ((a.first & 0x80000000) && (b.first & 0x80000000))
			{
				return a.second->getName().getString() < b.second->getName().getString();
			}
			else
			{
				return a.first < b.first;
			}
		}
		);

		for (const auto& pair : sortedFunctions)
		{
			const String filename = (pair.second->getType() == lemon::Function::Type::SCRIPT) ? WString(static_cast<const lemon::ScriptFunction*>(pair.second)->mSourceFileInfo->mFilename).toString() : "";
			String line;
			if (mVisualizationSorting && !filename.empty())
				line << filename << " | ";
			line << ((pair.first < 0x80000000) ? String(0, "0x%06x: ", pair.first) : "");
			line << pair.second->getName().getString();
			if (!mVisualizationSorting && !filename.empty())
				line << " | " << filename;

			ImGui::Text("%s", *line);
		}
	}
	else
	{
		// TODO: Profiling samples are very imprecise and not really useful at all, so they're disabled
	#ifdef DEBUG
		ImGui::Checkbox("Show profiling samples", &mShowProfilingSamples);
	#endif
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		const std::vector<uint32>& unknownAddresses = codeExec.getUnknownAddresses();
		if (unknownAddresses.size() != mSortedUnknownAddressed.size())
		{
			mSortedUnknownAddressed = unknownAddresses;
			std::sort(mSortedUnknownAddressed.begin(), mSortedUnknownAddressed.end(), [](uint32 a, uint32 b) { return a < b; } );
		}

		if (!mSortedUnknownAddressed.empty())
		{
			if (ImGui::CollapsingHeader("Unknown called addresses", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGuiHelpers::ScopedIndent si;
				for (uint32 address : mSortedUnknownAddressed)
				{
					ImGui::Bullet();
					ImGui::SameLine();
					ImGui::TextColored(ImGuiHelpers::COLOR_RED, "0x%06x", address);

					if (PlatformFunctions::hasClipboardSupport())
					{
						ImGui::SameLine();
						ImGui::PushID(address);
						if (ImGui::SmallButton("Copy"))
						{
							PlatformFunctions::copyToClipboard(*String(0, "0x%06x", address));
						}
						ImGui::PopID();
					}
				}
				ImGui::Separator();
			}
			ImGui::Spacing();
		}
		ImGui::Spacing();

		// Help marker
		ImGui::Text("Call frame hierarchy");
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::BeginItemTooltip())
		{
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::Text("This is the call hierarchy of script functions that were executed in the last frame.");

			if (mShowProfilingSamples)
			{
				ImGui::Text("Profiling samples count how long execution of a function took.");
			}
			else
			{
				ImGui::TextColored(ImGuiHelpers::COLOR_WHITE, "White: ");
				ImGui::SameLine();
				ImGui::Text("Function called directly by name.");

				ImGui::TextColored(ImGuiHelpers::COLOR_LIGHT_YELLOW, "Yellow: ");
				ImGui::SameLine();
				ImGui::Text("Function called indirectly by address hook.");

				ImGui::TextColored(ImGuiHelpers::COLOR_RED, "Red: ");
				ImGui::SameLine();
				ImGui::Text("Failed address hook call.");

				ImGui::TextColored(ImGuiHelpers::COLOR_GRAY60, "Gray: ");
				ImGui::SameLine();
				ImGui::Text("Already called at the start of the last frame.");
			}

			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		ImGui::Spacing();

		// Display (arrow) buttons without frame and default background
		ImGui::PushStyleColor(ImGuiCol_Button, ImGuiHelpers::COLOR_TRANSPARENT);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

		std::unordered_map<uint64, int> keyCounters;

		int ignoreDepth = -1;
		for (size_t callFrameIndex = 0; callFrameIndex < callFrames.size(); ++callFrameIndex)
		{
			const CodeExec::CallFrame& callFrame = callFrames[callFrameIndex];

			// Ignore debugging stuff
			if (nullptr != callFrame.mFunction && rmx::startsWith(callFrame.mFunction->getName().getString(), "debug"))
				continue;

			if (ignoreDepth != -1)
			{
				if (callFrame.mDepth > ignoreDepth)
					continue;

				ignoreDepth = -1;
			}

			// Indentation is linear at first, but switched to inverse exponential for deeper call farmes
			constexpr float DEFAULT_INDENT = 16.0f;
			constexpr int EXP_MIN_DEPTH = 10;
			constexpr float EXP_FALLOFF = 0.05f;
			float indent = (float)clamp(callFrame.mDepth, 0, EXP_MIN_DEPTH);
			if (callFrame.mDepth > EXP_MIN_DEPTH)
				indent += (1.0f - std::expf((float)(EXP_MIN_DEPTH - callFrame.mDepth) * EXP_FALLOFF)) / EXP_FALLOFF;
			ImGuiHelpers::ScopedIndent si(4.0f + std::floor(indent * DEFAULT_INDENT));

			uint64 key = (nullptr == callFrame.mFunction) ? 0xffffffffffffffffULL : callFrame.mFunction->getID();
			const int keyCount = ++keyCounters[key];
			key += (uint64)(keyCount - 1) << 32;

			bool isOpen = false;
			const bool hasAnyChildren = (callFrameIndex + 1 < callFrames.size()) && (callFrames[callFrameIndex + 1].mDepth > callFrame.mDepth);
			if (hasAnyChildren)
			{
				bool* openState = mapFind(mOpenState, key);
				if (nullptr != openState)
				{
					isOpen = *openState;
				}
				else
				{
					const bool defaultOpen = (callFrame.mAnyChildFailed || callFrame.mDepth < 2);
					isOpen = defaultOpen;
				}

				ImGui::PushStyleColor(ImGuiCol_Text, isOpen ? ImGuiHelpers::COLOR_GRAY60 : ImGuiHelpers::COLOR_WHITE);
				if (ImGui::ArrowButton(("##" + std::to_string(key)).c_str(), isOpen ? ImGuiDir_Down : ImGuiDir_Right))
				{
					mOpenState[key] = !isOpen;
				}
				ImGui::PopStyleColor();
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelpers::COLOR_GRAY30);
				ImGui::Bullet();
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();

			if (!isOpen)
			{
				ignoreDepth = callFrame.mDepth;
			}

			if (callFrame.mType == CodeExec::CallFrame::Type::FAILED_HOOK)
			{
				ImGui::TextColored(ImGuiHelpers::COLOR_RED, "0x%06x", callFrame.mAddress);
			}
			else
			{
				ImVec4 color = ImGuiHelpers::COLOR_WHITE;
				std::string postfix;
				if (mShowProfilingSamples)
				{
					postfix = " <" + std::to_string(callFrame.mSteps) + ">";
					const float log = log10f((float)clamp((int)callFrame.mSteps, 100, 1000000));
					Color rgba;
					rgba.setFromHSL(Vec3f((0.75f - log / 6.0f) * 360.0f, 1.0f, 0.5f));
					color.x = rgba.r;
					color.y = rgba.g;
					color.z = rgba.b;
				}
				else
				{
					color = (callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_STACK)  ? ImGuiHelpers::COLOR_GRAY60 :
							(callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_DIRECT) ? ImGuiHelpers::COLOR_WHITE : ImGuiHelpers::COLOR_LIGHT_YELLOW;
				}
				RMX_ASSERT(nullptr != callFrame.mFunction, "Invalid function pointer");
				ImGui::TextColored(color, "%s%s", std::string(callFrame.mFunction->getName().getString()).c_str(), postfix.c_str());
			}
		}

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(1);

		if (callFrames.size() == CodeExec::CALL_FRAMES_LIMIT)
		{
			ImGui::TextColored(ImGuiHelpers::COLOR_RED, "[!] Reached call frames limit");
		}
	}

}

#endif
