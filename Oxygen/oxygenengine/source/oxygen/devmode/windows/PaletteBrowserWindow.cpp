/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/PaletteBrowserWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/resources/PaletteCollection.h"


PaletteBrowserWindow::PaletteBrowserWindow() :
	DevModeWindowBase("Palette Browser", Category::GRAPHICS, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void PaletteBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(5.0f, 500.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	// Refresh list if needed
	const PaletteCollection& paletteCollection = PaletteCollection::instance();
	if (mLastPaletteCollectionChangeCounter != paletteCollection.getGlobalChangeCounter())
	{
		mLastPaletteCollectionChangeCounter = paletteCollection.getGlobalChangeCounter();

		mSortedPalettes.clear();
		for (const auto& pair : paletteCollection.getAllPalettes())
		{
			mSortedPalettes.push_back(&pair.second);
		}

		std::sort(mSortedPalettes.begin(), mSortedPalettes.end(),
			[](const PaletteBase* a, const PaletteBase* b) { return a->getIdentifier() < b->getIdentifier(); });

		mPreviewPalette = nullptr;
	}

	// TODO: Cache filter results
	static ImGuiHelpers::FilterString filterString;
	filterString.draw();

	const PaletteBase* clickedPalette = nullptr;

	if (ImGui::BeginTable("Palette Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 200.0f * uiScale)))
	{
		ImGui::TableSetupColumn("Identifier");

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const PaletteBase* palette : mSortedPalettes)
		{
			if (!filterString.shouldInclude(palette->getIdentifier()))
				continue;

			const ImVec4 textColor = ImGuiHelpers::COLOR_WHITE;// (nullptr == palette->mSourceInfo.mMod) ? ImGuiHelpers::COLOR_WHITE : ImGuiHelpers::COLOR_LIGHT_CYAN;

			ImGui::PushID(palette);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextColored(textColor, "%s", palette->getIdentifier().c_str());

			ImGui::SameLine();
			if (ImGui::Selectable("", mPreviewPalette == palette, ImGuiSelectableFlags_SpanAllColumns))
			{
				clickedPalette = palette;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	// Preview
	if (nullptr != clickedPalette && mPreviewPalette != clickedPalette)
	{
		mPreviewPalette = clickedPalette;
	}

	if (nullptr != mPreviewPalette)
	{
		const ImVec2 colorEntrySize(roundToFloat(12.0f * uiScale), roundToFloat(12.0f * uiScale));

		ImGui::BeginChild("Palette", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(roundToFloat(2.0f * uiScale), roundToFloat(2.0f * uiScale)));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		for (int k = 0; k < (int)mPreviewPalette->getSize(); ++k)
		{
			Color color = Color::fromABGR32(mPreviewPalette->getRawColors()[k]);
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

#endif
