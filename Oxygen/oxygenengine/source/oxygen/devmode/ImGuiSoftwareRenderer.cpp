/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/ImGuiSoftwareRenderer.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/drawing/software/SoftwareDrawer.h"
#include "oxygen/drawing/software/SoftwareRasterizer.h"
#include "imgui.h"


void ImGuiSoftwareRenderer::newFrame()
{
	// This is needed not only to cache the fonts texture data, but also to let ImGui know we actually processed the font texture data
	uint8* data = nullptr;
	ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&data, &mFontsTextureSize.x, &mFontsTextureSize.y);
	mFontsTextureData = reinterpret_cast<uint32*>(data);
}

void ImGuiSoftwareRenderer::renderDrawData()
{
	// TODO:
	//  - Extend software rasterizer to respect the "mSwapRedBlueChannels" setting when sampling a texture
	//  - Support different textures; this requires actual usage of drawer textures (e.g. in SpriteBrowserWindow) and some system extensions to have unique texture IDs
	//  - Performance optimization: Support rendering textured rectangles as well
	//  - Fix the minor glitches; though they might in part be related to missing anti-aliasing

	DrawerInterface* drawer = EngineMain::instance().getDrawer().getActiveDrawer();
	if (nullptr == drawer || drawer->getType() != Drawer::Type::SOFTWARE)
		return;

	SoftwareDrawer& softwareDrawer = *static_cast<SoftwareDrawer*>(drawer);
	const BitmapViewMutable<uint32>& output = softwareDrawer.getRenderTarget();

	Blitter::Options blitterOptions;
	blitterOptions.mBlendMode = BlendMode::ALPHA;
	blitterOptions.mSamplingMode = SamplingMode::BILINEAR;	// Looks a bit better, but is expensive
	blitterOptions.mSwapRedBlueChannels = softwareDrawer.needSwapRedBlueChannels();
	SoftwareRasterizer rasterizer = SoftwareRasterizer(output, blitterOptions);
	Blitter blitter;

	SoftwareRasterizer::Vertex vertices[3];

	const ImDrawData* data = ImGui::GetDrawData();
	for (const ImDrawList* drawList : data->CmdLists)
	{
		for (const ImDrawCmd& drawCmd : drawList->CmdBuffer)
		{
			const ImDrawIdx* indexPtr = &drawList->IdxBuffer[drawCmd.IdxOffset];
			const uint32 indexOffset = drawCmd.VtxOffset;

			const Recti clipRect((int)drawCmd.ClipRect.x, (int)drawCmd.ClipRect.y, (int)(drawCmd.ClipRect.z - drawCmd.ClipRect.x), (int)(drawCmd.ClipRect.w - drawCmd.ClipRect.y));
			BitmapViewMutable<uint32> outputView(output, clipRect);
			rasterizer.setOutput(outputView);

			const uint32* textureData = nullptr;
			Vec2i textureSize;
			if (drawCmd.TextureId == ImGui::GetIO().Fonts->TexID)
			{
				textureData = mFontsTextureData;
				textureSize = mFontsTextureSize;
			}
			else
			{
				// TODO: Support other textures than the default one (depending on "drawCmd.TextureId")
			}

			if (nullptr != textureData)
			{
				BitmapView<uint32> inputView(textureData, textureSize);

				for (uint32 k = 0; k + 3 <= drawCmd.ElemCount; k += 3)
				{
					// Check if the next two triangles form a rectangle
					if (k + 6 <= drawCmd.ElemCount)
					{
						if (indexPtr[k + 0] == indexPtr[k + 3] && indexPtr[k + 2] == indexPtr[k + 4])
						{
							const ImDrawVert& v0 = drawList->VtxBuffer[indexOffset + indexPtr[k + 0]];
							const ImDrawVert& v1 = drawList->VtxBuffer[indexOffset + indexPtr[k + 1]];
							const ImDrawVert& v2 = drawList->VtxBuffer[indexOffset + indexPtr[k + 2]];
							const ImDrawVert& v3 = drawList->VtxBuffer[indexOffset + indexPtr[k + 5]];

							if (v0.pos.x == v3.pos.x && v1.pos.x == v2.pos.x && v0.pos.y == v1.pos.y && v2.pos.y == v3.pos.y &&
								v0.uv.x == v3.uv.x && v1.uv.x == v2.uv.x && v0.uv.y == v1.uv.y && v2.uv.y == v3.uv.y &&
								v0.col == v1.col && v1.col == v2.col && v2.col == v3.col)
							{
								Recti destRect((int)v0.pos.x, (int)v0.pos.y, (int)(v2.pos.x - v0.pos.x), (int)(v2.pos.y - v0.pos.y));
								destRect.intersect(clipRect);
								Color color = Color::fromABGR32(v0.col);
								const bool isUntextured = (v0.uv.x == v2.uv.x && v0.uv.y == v2.uv.y);

								if (isUntextured)
								{
									if (blitterOptions.mSwapRedBlueChannels)
										color.swapRedBlue();

									blitter.blitColor(Blitter::OutputWrapper(output, destRect), color, BlendMode::ALPHA);

									// Skip the next triangle, as we already handled it
									k += 3;
									continue;
								}
								else
								{
									// TODO: Support textured rects as well

								}
							}
						}
					}

					// Build vertices
					bool isUntextured = false;
					bool isUntinted = false;
					{
						const ImDrawVert* tri[3];
						for (int j = 0; j < 3; ++j)
						{
							tri[j] = &drawList->VtxBuffer[indexOffset + indexPtr[k + j]];
						}

						isUntextured = (tri[0]->uv.x == tri[1]->uv.x && tri[0]->uv.x == tri[2]->uv.x &&
										tri[0]->uv.y == tri[1]->uv.y && tri[0]->uv.y == tri[2]->uv.y);
						isUntinted = (tri[0]->col == 0xffffffff && tri[1]->col == 0xffffffff && tri[2]->col == 0xffffffff);

						for (int j = 0; j < 3; ++j)
						{
							const ImDrawVert& vert = *tri[j];
							vertices[j].mPosition.set(vert.pos.x - drawCmd.ClipRect.x, vert.pos.y - drawCmd.ClipRect.y);
							vertices[j].mColor = Color::fromABGR32(vert.col);
							vertices[j].mUV.set(vert.uv.x, vert.uv.y);

							// TODO: This doesn't swap red and blue channels for the sampled texture - that would need to be implemented separately in teh rasterizer itself
							if (blitterOptions.mSwapRedBlueChannels)
								vertices[j].mColor.swapRedBlue();
						}
					}

					if (isUntextured && isUntinted)
					{
						rasterizer.drawTriangle(vertices);
					}
					else
					{
						rasterizer.drawTriangle(vertices, BitmapView<uint32>(textureData, textureSize), !isUntinted);
					}
				}
			}
		}
	}
}

#endif
