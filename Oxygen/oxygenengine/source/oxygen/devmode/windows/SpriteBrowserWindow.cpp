/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/SpriteBrowserWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/modding/Mod.h"
#include "oxygen/resources/PaletteCollection.h"


SpriteBrowserWindow::SpriteBrowserWindow() :
	DevModeWindowBase("Sprite Browser", Category::GRAPHICS, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void SpriteBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(640.0f, 60.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	// Refresh list if needed
	const SpriteCollection& spriteCollection = SpriteCollection::instance();
	if (mLastSpriteCollectionChangeCounter != spriteCollection.getGlobalChangeCounter())
	{
		mLastSpriteCollectionChangeCounter = spriteCollection.getGlobalChangeCounter();

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
			[](const SpriteCollection::Item* a, const SpriteCollection::Item* b) { return a->mSourceInfo.mSourceIdentifier < b->mSourceInfo.mSourceIdentifier; });

		mPreviewItem = nullptr;
		mPreviewTexture = Texture();
	}
	
	// TODO: Cache filter results
	static ImGuiHelpers::FilterString filterString;
	filterString.draw();

	const SpriteCollection::Item* clickedItem = nullptr;

	if (ImGui::BeginTable("Sprite Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 200.0f * uiScale)))
	{
		ImGui::TableSetupColumn("Identifier");
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 75);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 90);
		ImGui::TableSetupColumn("Source");

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const SpriteCollection::Item* item : mSortedItems)
		{
			if (!filterString.shouldInclude(item->mSourceInfo.mSourceIdentifier))
				continue;

			const ImVec4 textColor = (nullptr == item->mSourceInfo.mMod) ? ImGuiHelpers::COLOR_WHITE : ImGuiHelpers::COLOR_LIGHT_CYAN;

			ImGui::PushID(item);

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

			ImGui::SameLine();
			if (ImGui::Selectable("", mPreviewItem == item, ImGuiSelectableFlags_SpanAllColumns))
			{
				clickedItem = item;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	// Preview
	if (nullptr != clickedItem && mPreviewItem != clickedItem)
	{
		mPreviewItem = clickedItem;

		if (clickedItem->mUsesComponentSprite)
		{
			const Bitmap& bitmap = static_cast<ComponentSprite*>(clickedItem->mSprite)->getBitmap();
			mPreviewTexture.load(bitmap);
			mPreviewTexture.setFilterNearest();
		}
		else
		{
			const uint64 paletteKey = rmx::getMurmur2_64(mPreviewItem->mSourceInfo.mSourceIdentifier);
			const PaletteBase* palette = PaletteCollection::instance().getPalette(paletteKey, 0);
			if (nullptr != palette)
			{
				const PaletteBitmap& paletteBitmap = static_cast<PaletteSprite*>(clickedItem->mSprite)->getBitmap();
				paletteBitmap.convertToRGBA(mTempBitmap, palette->getRawColors(), palette->getSize());
				mPreviewTexture.load(mTempBitmap);
				mPreviewTexture.setFilterNearest();
			}
			else
			{
				mPreviewTexture = Texture();
			}
		}
	}

	if (nullptr != mPreviewItem)
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGuiHelpers::ScopedIndent si;

		ImGui::Text("%s", mPreviewItem->mSourceInfo.mSourceIdentifier.c_str());

		if (nullptr != mPreviewItem->mSprite)
		{
			ImGui::Text("%s, %d x %d pixels", mPreviewItem->mUsesComponentSprite ? "Component sprite" : "Palette sprite", mPreviewItem->mSprite->getSize().x, mPreviewItem->mSprite->getSize().y);
		}
		else
		{
			ImGui::Text("%s", mPreviewItem->mUsesComponentSprite ? "Component sprite" : "Palette sprite");
		}

		if (mPreviewTexture.getHandle() != 0)
		{
			ImGui::Text("Preview");
			ImGui::SameLine();
			if (ImGui::Button("Auto"))
				mPreviewScale = 0;
			ImGui::SameLine();
			if (ImGui::Button("1x"))
				mPreviewScale = 1;
			ImGui::SameLine();
			if (ImGui::Button("2x"))
				mPreviewScale = 2;
			ImGui::SameLine();
			if (ImGui::Button("4x"))
				mPreviewScale = 4;
			ImGui::SameLine();
			if (ImGui::Button("8x"))
				mPreviewScale = 8;
			ImGui::SameLine();
			if (ImGui::Button("16x"))
				mPreviewScale = 16;

			int scale = mPreviewScale;
			if (scale == 0)
				scale = clamp(std::min(500 / mPreviewTexture.getWidth(), 350 / mPreviewTexture.getHeight()), 1, 8);

			ImGui::BeginChild("Preview Image", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
			ImGui::Image(mPreviewTexture.getHandle(), ImVec2((float)(mPreviewTexture.getWidth() * scale), (float)(mPreviewTexture.getHeight() * scale)));
			ImGui::EndChild();

			if (!mPreviewItem->mUsesComponentSprite)
			{
				ImGui::Checkbox("View BMP palette", &mShowPalette);
				if (mShowPalette)
				{
					const uint64 paletteKey = rmx::getMurmur2_64(mPreviewItem->mSourceInfo.mSourceIdentifier);
					const PaletteBase* palette = PaletteCollection::instance().getPalette(paletteKey, 0);
					if (nullptr != palette)
					{
						const ImVec2 colorEntrySize(roundToFloat(12.0f * uiScale), roundToFloat(12.0f * uiScale));

						ImGui::BeginChild("Palette", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(roundToFloat(2.0f * uiScale), roundToFloat(2.0f * uiScale)));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
						for (int k = 0; k < (int)palette->getSize(); ++k)
						{
							Color color = Color::fromABGR32(palette->getRawColors()[k]);
							if (k & 15)
								ImGui::SameLine();
							ImGui::PushID(k);
							ImGui::ColorButton(*String(0, "Palette color #%d", k), ImVec4(color.r, color.g, color.b, color.a), ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoLabel, colorEntrySize);
							ImGui::PopID();
						}
						ImGui::PopStyleVar(2);
						ImGui::EndChild();
					}
				}
			}
		}
		else
		{
			ImGui::Text("No preview available");
		}
	}
}

#endif
