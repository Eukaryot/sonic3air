/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/CallFramesWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"

#include <lemon/program/Function.h>


CallFramesWindow::CallFramesWindow() :
	DevModeWindowBase("Call Frames", Category::DEBUGGING, 0)
{
}

void CallFramesWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(500.0f, 240.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(450.0f, 300.0f), ImGuiCond_FirstUseEver);

	const float uiScale = ImGui::GetIO().FontGlobalScale;

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();

	codeExec.processCallFrames();
	const auto& callFrames = codeExec.getCallFrames();

	ImGui::Checkbox("Show all hit functions", &mShowAllHitFunctions);

	if (mShowAllHitFunctions)
	{
		ImGui::Checkbox("Sort by filename", &mVisualizationSorting);
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
		ImGui::Checkbox("Show profiling samples", &mShowOpcodesExecuted);
		ImGui::Spacing();

		int openTreeNodeDepth = -1;		// TODO: This is maybe somewhat redundant with "ignoreDepth"?
		int ignoreDepth = 0;
		for (size_t callFrameIndex = 0; callFrameIndex < callFrames.size(); ++callFrameIndex)
		{
			const CodeExec::CallFrame& callFrame = callFrames[callFrameIndex];

			// Ignore debugging stuff
			if (nullptr != callFrame.mFunction && rmx::startsWith(callFrame.mFunction->getName().getString(), "debug"))
				continue;

			if (ignoreDepth != 0)
			{
				if (callFrame.mDepth > ignoreDepth)
					continue;

				ignoreDepth = 0;
			}

			if (openTreeNodeDepth >= 0 && callFrame.mDepth <= openTreeNodeDepth)
			{
				ImGui::TreePop();
				--openTreeNodeDepth;
			}

			const uint64 key = (nullptr == callFrame.mFunction) ? 0xffffffffffffffffULL : callFrame.mFunction->getID();

			bool isOpen = false;
			const bool hasAnyChildren = (callFrameIndex + 1 < callFrames.size()) && (callFrames[callFrameIndex + 1].mDepth > callFrame.mDepth);
			if (hasAnyChildren)
			{
				const bool defaultOpen = (callFrame.mAnyChildFailed || callFrame.mDepth < 1);
				isOpen = ImGui::TreeNodeEx(("##" + std::to_string(key)).c_str(), defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
			}
			else
			{
				ImGui::BulletText(" ");
			}
			ImGui::SameLine();

			if (isOpen)
			{
				++openTreeNodeDepth;
			}
			else
			{
				ignoreDepth = callFrame.mDepth;
			}

			if (callFrame.mType == CodeExec::CallFrame::Type::FAILED_HOOK)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%06x", callFrame.mAddress);
			}
			else
			{
				ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
				std::string postfix;
				if (mShowOpcodesExecuted)
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
					color = (callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_STACK) ? ImVec4(0.6f, 0.6f, 0.6f, 1.0f) :
							(callFrame.mType == CodeExec::CallFrame::Type::SCRIPT_DIRECT) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
				}
				RMX_ASSERT(nullptr != callFrame.mFunction, "Invalid function pointer");
				ImGui::TextColored(color, "%s%s", std::string(callFrame.mFunction->getName().getString()).c_str(), postfix.c_str());
			}
		}

		while (openTreeNodeDepth >= 0)
		{
			ImGui::TreePop();
			--openTreeNodeDepth;
		}

		if (callFrames.size() == CodeExec::CALL_FRAMES_LIMIT)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[!] Reached call frames limit");
		}
	}

}

#endif
