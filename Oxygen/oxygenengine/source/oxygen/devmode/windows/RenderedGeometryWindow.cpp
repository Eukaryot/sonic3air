/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/RenderedGeometryWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"


RenderedGeometryWindow::RenderedGeometryWindow() :
	DevModeWindowBase("Rendered Geometry", Category::DEBUGGING, 0)
{
}

void RenderedGeometryWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(350.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = ImGui::GetIO().FontGlobalScale;

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	Drawer& drawer = EngineMain::instance().getDrawer();

	if (ImGui::BeginTable("Rendered Geometry Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupColumn("Render Queue", 0, 120);
		ImGui::TableSetupColumn("Geometry Type", 0, 200);
		ImGui::TableSetupColumn("Position", 0, 100);
		ImGui::TableSetupColumn("Name", 0, 180);

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();
		
		const auto& geometries = VideoOut::instance().getGeometries();
		int index = 0;
		for (const Geometry* geometry : geometries)
		{
			ImGui::PushID(index);
			ImGui::TableNextRow();

			String typeString;
			String positionString;
			String nameString;
			Color color = Color::WHITE;
			Recti highlightRect;

			switch (geometry->getType())
			{
				case Geometry::Type::PLANE:
				{
					const PlaneGeometry& pg = *static_cast<const PlaneGeometry*>(geometry);
					typeString.formatString("Plane %d%s", pg.mPlaneIndex, pg.mPriorityFlag ? " (Prio)" : "");
					break;
				}

				case Geometry::Type::SPRITE:
				{
					const renderitems::SpriteInfo& info = static_cast<const SpriteGeometry*>(geometry)->mSpriteInfo;
					const char* spriteType = nullptr;
					color = Color::fromABGR32(0xffa0a0a0);
					highlightRect.set(info.mPosition.x, info.mPosition.y, 32, 32);
					switch (info.getType())
					{
						case RenderItem::Type::VDP_SPRITE:
						{
							spriteType = "VDP sprite";
							color.setABGR32(0xffffffc0);
							renderitems::VdpSpriteInfo& vsi = (renderitems::VdpSpriteInfo&)info;
							highlightRect.width = vsi.mSize.x * 8;
							highlightRect.height = vsi.mSize.y * 8;
							break;
						}

						case RenderItem::Type::PALETTE_SPRITE:
						{
							spriteType = "Palette sprite";
							color.setABGR32(0xffffc0ff);
							renderitems::PaletteSpriteInfo& psi = (renderitems::PaletteSpriteInfo&)info;
							highlightRect.setPos(highlightRect.getPos() + psi.mPivotOffset);
							highlightRect.width = psi.mSize.x;
							highlightRect.height = psi.mSize.y;
							const lemon::FlyweightString* str = codeExec.getLemonScriptRuntime().getInternalLemonRuntime().resolveStringByKey(psi.mKey);
							if (nullptr != str)
								nameString = str->getString();
							break;
						}

						case RenderItem::Type::COMPONENT_SPRITE:
						{
							spriteType = "Component sprite";
							color.setABGR32(0xffffc0e0);
							renderitems::ComponentSpriteInfo& csi = (renderitems::ComponentSpriteInfo&)info;
							highlightRect.setPos(highlightRect.getPos() + csi.mPivotOffset);
							highlightRect.width = csi.mSize.x;
							highlightRect.height = csi.mSize.y;
							const lemon::FlyweightString* str = codeExec.getLemonScriptRuntime().getInternalLemonRuntime().resolveStringByKey(csi.mKey);
							if (nullptr != str)
								nameString = str->getString();
							break;
						}

						case RenderItem::Type::SPRITE_MASK:
						{
							spriteType = "Sprite mask";
							color.setABGR32(0xffc0ffff);
							renderitems::SpriteMaskInfo& smi = (renderitems::SpriteMaskInfo&)info;
							highlightRect.width = smi.mSize.x;
							highlightRect.height = smi.mSize.y;
							break;
						}

						default:
							break;
					}

					if (nullptr != spriteType)
					{
						typeString.formatString("%s%s", spriteType, info.mPriorityFlag ? " (Prio)" : "");
						positionString.formatString("%d, %d", info.mPosition.x, info.mPosition.y);
					}

					break;
				}

				case Geometry::Type::RECT:
				{
					typeString = "Rect";
					color.setABGR32(0xffc0ffff);
					break;
				}

				case Geometry::Type::TEXTURED_RECT:
				{
					typeString = "Textured rect / Text";
					color.setABGR32(0xffc0ffff);
					break;
				}

				case Geometry::Type::EFFECT_BLUR:
				{
					typeString = "Blur effect";
					color.setABGR32(0xffc0ffff);
					break;
				}

				case Geometry::Type::VIEWPORT:
				{
					const ViewportGeometry& vg = *static_cast<const ViewportGeometry*>(geometry);
					typeString = "Viewport";
					positionString.formatString("%d, %d   (%d x %d)", vg.mRect.x, vg.mRect.y, vg.mRect.width, vg.mRect.height);
					color.setABGR32(0xffc0ffff);
					break;
				}

				default:
				{
					typeString = "Unknown geometry type";
					color.setABGR32(0xffa0a0a0);
					break;
				}
			}

			const ImVec4 textColor(color.r, color.g, color.b, color.a);

			ImGui::TableSetColumnIndex(0);
			ImGui::TextColored(textColor, "0x%04x", geometry->mRenderQueue);

			ImGui::TableSetColumnIndex(1);
			ImGui::TextColored(textColor, "%s", *typeString);

			ImGui::TableSetColumnIndex(2);
			ImGui::TextColored(textColor, "%s", *positionString);

			ImGui::TableSetColumnIndex(3);
			ImGui::TextColored(textColor, "%s", *nameString);

			ImGui::SameLine();
			ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
			if (ImGui::IsItemHovered())
			{
				if (highlightRect.nonEmpty())
				{
					Application::instance().getGameView().addScreenHighlightRect(highlightRect, Color(0.0f, 1.0f, 0.5f, 0.75f));
				}
			}

			ImGui::PopID();
			++index;
		}

		ImGui::EndTable();
	}
}

#endif
