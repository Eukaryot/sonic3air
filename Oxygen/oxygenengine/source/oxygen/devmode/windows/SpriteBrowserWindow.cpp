/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/SpriteBrowserWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/modding/Mod.h"


SpriteBrowserWindow::SpriteBrowserWindow() :
	DevModeWindowBase("Sprite Browser")
{
}

void SpriteBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(350.0f, 150.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	// Refresh list if needed
	const SpriteCollection& spriteCollection = SpriteCollection::instance();
	if (mSpriteCollectionChangeCounter != spriteCollection.getGlobalChangeCounter())
	{
		mSpriteCollectionChangeCounter = spriteCollection.getGlobalChangeCounter();

		mSortedItems.clear();
		for (const auto& pair : spriteCollection.getAllSprites())
		{
			const SpriteCollection::Item& item = pair.second;

			// Filter out redirections and ROM data sprites
			if (nullptr == item.mRedirect || item.mSourceInfo.mType == SpriteCollection::SourceInfo::Type::ROM_DATA)
			{
				mSortedItems.push_back(&item);
			}
		}

		std::sort(mSortedItems.begin(), mSortedItems.end(),
			[](const SpriteCollection::Item* a, const SpriteCollection::Item* b)
			{
				return a->mSourceInfo.mSourceIdentifier < b->mSourceInfo.mSourceIdentifier;
			}
		);
	}
	
	// TODO: Cache filter results
	static char filterString[64] = { 0 };
	ImGui::InputText("Filter", filterString, 64, 0);

	if (ImGui::BeginTable("Sprite Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 200.0f)))
	{
		ImGui::TableSetupColumn("Identifier");
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 75);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("Source");

		ImGui::TableHeadersRow();

		for (const SpriteCollection::Item* item : mSortedItems)
		{
			if (filterString[0] && item->mSourceInfo.mSourceIdentifier.find(filterString) == std::string::npos)
				continue;

			const ImVec4 textColor = (nullptr == item->mSourceInfo.mMod) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 1.0f, 1.0f, 1.0f);

			ImGui::TableNextRow();
			
			ImGui::TableSetColumnIndex(0);
			ImGui::TextColored(textColor, "%s", item->mSourceInfo.mSourceIdentifier.c_str());

			ImGui::TableSetColumnIndex(1);
			if (nullptr != item->mSprite)
			{
				ImGui::TextColored(textColor, "%d x %d", item->mSprite->getSize().x, item->mSprite->getSize().y);
			}
			else
			{
				ImGui::TextColored(textColor, "n/a");
			}

			ImGui::TableSetColumnIndex(2);
			ImGui::TextColored(textColor, "%s", item->mUsesComponentSprite ? "Component" : "Palette");

			ImGui::TableSetColumnIndex(3);
			if (nullptr == item->mSourceInfo.mMod)
			{
				ImGui::TextColored(textColor, "%s", (item->mSourceInfo.mType == SpriteCollection::SourceInfo::Type::ROM_DATA) ? "ROM Data" : "Base Game Files");
			}
			else
			{
				ImGui::TextColored(textColor, "%s", item->mSourceInfo.mMod->mDisplayName.c_str());
			}
		}
		ImGui::EndTable();
	}
}

#endif
