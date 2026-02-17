/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/PersistentDataWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/imgui/ImGuiHelpers.h"


PersistentDataWindow::PersistentDataWindow() :
	DevModeWindowBase("Persistent Data", Category::MISC, 0)
{
}

PersistentDataWindow::~PersistentDataWindow()
{
	clearNode(mRootNode);
}

void PersistentDataWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(200.0f, 450.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 200.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	// Update cached data if needed
	const PersistentData& persistentData = PersistentData::instance();
	if (mCachedChangeCounter != persistentData.getChangeCounter())
	{
		mCachedChangeCounter = persistentData.getChangeCounter();
		clearNode(mRootNode);

		std::unordered_map<uint64, Node*> nodesByPath;
		for (const auto& pair : persistentData.getFiles())
		{
			const PersistentData::File& file = pair.second;
			const size_t slashPos = file.mFilePath.find_last_of('/');
			const std::string_view path = (slashPos == std::string::npos) ? "" : std::string_view(file.mFilePath).substr(0, slashPos);
			const std::string_view name = (slashPos == std::string::npos) ? std::string_view(file.mFilePath) : std::string_view(file.mFilePath).substr(slashPos + 1);

			// Get or create parent node representing the path
			Node* parentNode;
			if (path.empty())
			{
				parentNode = &mRootNode;
			}
			else
			{
				const uint64 pathHash = rmx::getMurmur2_64(path);
				Node*& foundNodeRef = nodesByPath[pathHash];
				if (nullptr == foundNodeRef)
				{
					foundNodeRef = &mNodePool.rentObject();
					foundNodeRef->mName = '[' + std::string(path) + ']';
					mRootNode.mChildNodes.push_back(foundNodeRef);
				}
				parentNode = foundNodeRef;
			}

			// Create a child node there
			Node& childNode = mNodePool.rentObject();
			childNode.mName = name;
			childNode.mFile = &file;
			parentNode->mChildNodes.push_back(&childNode);
		}

		// Sort entries
		sortNodeChildren(mRootNode);
	}

	buildContentForNode(mRootNode);
}

void PersistentDataWindow::clearNode(Node& node)
{
	node.mFile = nullptr;
	for (Node* child : node.mChildNodes)
	{
		clearNode(*child);
		mNodePool.returnObject(*child);
	}
	node.mChildNodes.clear();
}

void PersistentDataWindow::sortNodeChildren(Node& node)
{
	std::sort(node.mChildNodes.begin(), node.mChildNodes.end(),
		[](const Node* a, const Node* b)
		{
			if ((nullptr == a->mFile) != (nullptr == b->mFile))
				return (nullptr == a->mFile);
			else
				return a->mName < b->mName;
		});

	for (Node* childNode : node.mChildNodes)
	{
		sortNodeChildren(*childNode);
	}
}

void PersistentDataWindow::buildContentForNode(const Node& node)
{
	if (nullptr != node.mFile)
	{
		// List entries in the file
		const PersistentData::File& file = *node.mFile;
		if (!file.mEntries.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
			for (const PersistentData::Entry& entry : file.mEntries)
			{
				ImGui::BulletText("%s:   %d bytes", entry.mKey.c_str(), (int)entry.mData.size());
			}
			ImGui::PopStyleColor();
		}
	}

	// List child nodes recursively
	for (const Node* childNode : node.mChildNodes)
	{
		const bool isFolder = (nullptr == childNode->mFile);
		ImGui::PushStyleColor(ImGuiCol_Text, isFolder ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		const bool isOpen = ImGui::TreeNodeEx(childNode, 0, "%s", childNode->mName.c_str());
		ImGui::PopStyleColor();

		if (isOpen)
		{
			buildContentForNode(*childNode);
			ImGui::TreePop();
		}
	}
}

#endif
