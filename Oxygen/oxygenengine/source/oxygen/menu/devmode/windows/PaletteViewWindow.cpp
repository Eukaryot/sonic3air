/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/PaletteViewWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/imgui/ImGuiHelpers.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/PaletteCollection.h"
#include "oxygen/simulation/LogDisplay.h"


PaletteViewWindow::PaletteViewWindow() :
	DevModeWindowBase("Palette View", Category::GRAPHICS, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void PaletteViewWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(5.0f, 180.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const PaletteManager& paletteManager = RenderParts::instance().getPaletteManager();

	if (ImGui::CollapsingHeader("Options"))
	{
		ImGui::Checkbox("Show secondary", &mShowSecondary);

		if (ImGui::Button("Save palette as BMP"))
		{
			const std::wstring filepath = Configuration::instance().mAppDataPath + L"devmode/output/main_palette.bmp";
			savePaletteTo(paletteManager.getMainPalette(0), filepath);
		}
		if (mShowSecondary)
		{
			ImGui::SameLine();
			if (ImGui::Button("Save secondary as BMP"))
			{
				const std::wstring filepath = Configuration::instance().mAppDataPath + L"devmode/output/main_palette_secondary.bmp";
				savePaletteTo(paletteManager.getMainPalette(1), filepath);
			}
		}
	}

	const int numPalettes = mShowSecondary ? 2 : 1;

	ImGui::BeginTable("Palette Table", numPalettes, 0);
	ImGui::TableNextRow();
	for (int k = 0; k < numPalettes; ++k)
	{
		ImGui::PushID(k);
		ImGui::TableSetColumnIndex(k);
		const PaletteBase& palette = paletteManager.getMainPalette(k);
		drawPalette(palette);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void PaletteViewWindow::drawPalette(const PaletteBase& palette)
{
	const float uiScale = getUIScale();
	const ImVec2 colorEntrySize(roundToFloat(14.0f * uiScale), roundToFloat(14.0f * uiScale));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(roundToFloat(2.0f * uiScale), roundToFloat(2.0f * uiScale)));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

	for (int k = 0; k < std::min((int)palette.getSize(), 256); ++k)
	{
		Color color = Color::fromABGR32(palette.getRawColors()[k]);
		if (k & 15)
			ImGui::SameLine();
		ImGui::PushID(k);
		ImGui::ColorButton(*String(0, "Palette color #%d", k), ImVec4(color.r, color.g, color.b, color.a), ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoLabel, colorEntrySize);
		ImGui::PopID();
	}
	ImGui::PopStyleVar(2);
}

void PaletteViewWindow::savePaletteTo(const PaletteBase& palette, const std::wstring& filepath)
{
	PaletteBitmap bmp;
	bmp.create(16, 16);
	for (int colorIndex = 0; colorIndex < 0x100; ++colorIndex)
	{
		bmp.getData()[colorIndex] = (uint8)colorIndex;
	}

	bool success = false;
	std::vector<uint8> fileContent;
	if (bmp.saveBMP(fileContent, palette.getRawColors()))
	{
		success = FTX::FileSystem->saveFile(filepath, fileContent);
	}

	if (success)
		LogDisplay::instance().setLogDisplay("Saved palette to \"" + WString(filepath).toStdString() + "\"");
	else
		LogDisplay::instance().setLogDisplay("Error saving palette to \"" + WString(filepath).toStdString() + "\"");
}

#endif
