/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/windows/RenderedGeometryWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/ImGuiHelpers.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/gameview/GameView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"


RenderedGeometryWindow::RenderedGeometryWindow() :
	DevModeWindowBase("Rendered Geometry", Category::GRAPHICS, 0)
{
}

void RenderedGeometryWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(300.0f, 180.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	Drawer& drawer = EngineMain::instance().getDrawer();

	if (ImGui::BeginTable("Rendered Geometry Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY))
	{
		ImGui::TableSetupColumn("Render Queue", 0, 120);
		ImGui::TableSetupColumn("Geometry Type", 0, 200);
		ImGui::TableSetupColumn("Position", 0, 100);
		ImGui::TableSetupColumn("Size", 0, 100);
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
			String nameString;
			Color color = Color::WHITE;
			Vec2i position;
			Vec2i size;
			Recti highlightRect;

			switch (geometry->getType())
			{
				case Geometry::Type::PLANE:
				{
					const PlaneGeometry& pg = *static_cast<const PlaneGeometry*>(geometry);
					typeString.formatString("Plane %d%s", pg.mPlaneIndex, pg.mPriorityFlag ? " (Prio)" : "");
					position = pg.mActiveRect.getPos();
					size = pg.mActiveRect.getSize();
					highlightRect = pg.mActiveRect;
					break;
				}

				case Geometry::Type::SPRITE:
				{
					const renderitems::SpriteInfo& info = static_cast<const SpriteGeometry*>(geometry)->mSpriteInfo;
					const char* spriteType = nullptr;
					color = Color::fromABGR32(0xffa0a0a0);
					position = info.mPosition;
					highlightRect.set(info.mPosition.x, info.mPosition.y, 32, 32);
					switch (info.getType())
					{
						case RenderItem::Type::VDP_SPRITE:
						{
							spriteType = "VDP sprite";
							color.setABGR32(0xffffffc0);
							renderitems::VdpSpriteInfo& vsi = (renderitems::VdpSpriteInfo&)info;
							size = vsi.mSize * 8;
							highlightRect.setSize(size);
							break;
						}

						case RenderItem::Type::PALETTE_SPRITE:
						{
							spriteType = "Palette sprite";
							color.setABGR32(0xffffc0ff);
							renderitems::PaletteSpriteInfo& psi = (renderitems::PaletteSpriteInfo&)info;
							size = psi.mSize;
							highlightRect.setPos(highlightRect.getPos() + psi.mPivotOffset);
							highlightRect.setSize(size);
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
							size = csi.mSize;
							highlightRect.setPos(highlightRect.getPos() + csi.mPivotOffset);
							highlightRect.setSize(size);
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
							size = smi.mSize;
							highlightRect.setSize(size);
							break;
						}

						default:
							break;
					}

					if (nullptr != spriteType)
					{
						typeString.formatString("%s%s", spriteType, info.mPriorityFlag ? " (Prio)" : "");
					}

					break;
				}

				case Geometry::Type::RECT:
				{
					const RectGeometry& rg = static_cast<const RectGeometry&>(*geometry);
					typeString = "Rect";
					color.setABGR32(0xffc0ffff);
					position = rg.mRect.getPos();
					size = rg.mRect.getSize();
					highlightRect = rg.mRect;
					break;
				}

				case Geometry::Type::TEXTURED_RECT:
				{
					const TexturedRectGeometry& tg = static_cast<const TexturedRectGeometry&>(*geometry);
					typeString = "Textured rect / Text";
					color.setABGR32(0xffc0ffff);
					position = tg.mRect.getPos();
					size = tg.mRect.getSize();
					highlightRect = tg.mRect;
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
					color.setABGR32(0xffc0ffff);
					position = vg.mRect.getPos();
					size = vg.mRect.getSize();
					highlightRect = vg.mRect;
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
			ImGui::TextColored(textColor, "%d, %d", position.x, position.y);

			ImGui::TableSetColumnIndex(3);
			ImGui::TextColored(textColor, "%d, %d", size.x, size.y);

			ImGui::TableSetColumnIndex(4);
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
