/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/ProfilingView.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/audio/AudioPlayer.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/helper/Profiling.h"


namespace
{
	void addRectToMesh(std::vector<DrawerMeshVertex_P2_C4>& mesh, Rectf rect, Color color)
	{
		const size_t offset = mesh.size();
		mesh.resize(offset + 6);

		mesh[offset+0].mPosition.set(rect.x, rect.y);
		mesh[offset+1].mPosition.set(rect.x, rect.y + rect.height);
		mesh[offset+2].mPosition.set(rect.x + rect.width, rect.y + rect.height);
		mesh[offset+3].mPosition.set(rect.x + rect.width, rect.y + rect.height);
		mesh[offset+4].mPosition.set(rect.x + rect.width, rect.y);
		mesh[offset+5].mPosition.set(rect.x, rect.y);

		mesh[offset+0].mColor = color;
		mesh[offset+1].mColor = color;
		mesh[offset+2].mColor = color;
		mesh[offset+3].mColor = color;
		mesh[offset+4].mColor = color;
		mesh[offset+5].mColor = color;
	}
}


ProfilingView::ProfilingView()
{
}

ProfilingView::~ProfilingView()
{
}

void ProfilingView::initialize()
{
}

void ProfilingView::deinitialize()
{
}

void ProfilingView::update(float timeElapsed)
{
}

void ProfilingView::render()
{
	GuiBase::render();

	Configuration& config = Configuration::instance();
	if (config.mPerformanceDisplay != 2)
		return;

	Drawer& drawer = EngineMain::instance().getDrawer();
	Profiling::Region& rootRegion = Profiling::getRootRegion();
	Font& font = EngineMain::getDelegate().getDebugFont(10);

	static std::vector<std::pair<Profiling::Region*, int>> regions;
	Profiling::listRegionsRecursive(regions);
	Profiling::AdditionalData& additionalData = Profiling::getAdditionalData();
	const int numFrames = (int)additionalData.mFrames.size();

	// Show graph -- except if playback speed is extremely high for script performance measurements
	if (additionalData.mAverageSimulationsPerSecond < 800.0f)
	{
		static std::vector<DrawerMeshVertex_P2_C4> mesh;
		mesh.clear();

		Recti box(FTX::screenWidth() - numFrames * 3 - 40, FTX::screenHeight() - 117, numFrames * 3 + 20, 107);
		addRectToMesh(mesh, box, Color(0.0f, 0.0f, 0.0f, 0.7f));

		struct Bar
		{
			float mHeight = 0.0f;
			Color mColor;
		};
		static std::vector<Bar> bars;

		for (int i = 0; i < numFrames; ++i)
		{
			// Build bars
			// TODO: Support hierarchy of regions here (as soon as this gets actually used)
			bars.clear();
			for (const std::pair<Profiling::Region*,int>& pair : regions)
			{
				const Profiling::Region& region = *pair.first;
				RMX_ASSERT((int)region.mFrameTimes.size() == numFrames, "Wrong number of frames in region " << region.mName);
				Bar& bar = vectorAdd(bars);
				bar.mHeight = (float)region.mFrameTimes[i].mExclusiveTime * 4000.0f;	// Multiplier = 4 pixels per millisecond
				bar.mColor = region.mColor;
			}

			const Recti barBox(box.x + 10 + i*3, box.y, 2, box.height);
			float accumulatedHeight = 0.0f;
			int startY = 0;
			for (Bar& bar : bars)
			{
				accumulatedHeight += bar.mHeight;
				const int endY = roundToInt(accumulatedHeight);
				Recti rect(barBox.x, barBox.y + barBox.height - 28 - endY, barBox.width, endY - startY);
				addRectToMesh(mesh, rect, bar.mColor);
				startY = endY;
			}

			// Number of simulated frames
			Recti rect(barBox.x, barBox.y + barBox.height - 16, barBox.width, additionalData.mFrames[i].mNumSimulationFrames * 3);
			addRectToMesh(mesh, rect, Color(0.5f, 0.5f, 0.5f));
		}

		drawer.drawMesh(mesh);
	}

	// Frame rate
	int py = FTX::screenHeight() - 210 - (int)regions.size() * 18;
	if (rootRegion.mAverageTime > 0.0)
	{
		if (additionalData.mAverageSimulationsPerSecond >= 100.0f)
		{
			drawer.printText(font, Recti(FTX::screenWidth() - 100, py - 14, 0, 0), String(0, "%.1f sim./s", additionalData.mAverageSimulationsPerSecond), 3);
			drawer.printText(font, Recti(FTX::screenWidth() - 100, py, 0, 0), String(0, "(smoothed) %.1f sim./s", additionalData.mSmoothedSimulationsPerSecond), 3);
		}
		py += 18;
		drawer.printText(font, Recti(FTX::screenWidth() - 100, py, 0, 0), String(0, "%.1f fps", (float)(1.0 / rootRegion.mAverageTime)), 3);
	}
	py += 36;

	for (const std::pair<Profiling::Region*,int>& pair : regions)
	{
		const Profiling::Region& region = *pair.first;
		const int px = FTX::screenWidth() - 320;

		Color color = region.mColor;
		color.a = 0.333f;
		color = color.blendOver(Color::WHITE);
		drawer.printText(font, Recti(px + pair.second * 15, py, 0, 0), region.mName + ":", 1, color);
		drawer.printText(font, Recti(px + 220, py, 0, 0), String(0, "%0.2f ms", (float)region.mAverageTime * 1000.0f), 3, color);
		py += 18;
	}

	// Memory usage data
	drawer.printText(font, Recti(FTX::screenWidth() - 200, 10, 0, 0), String(0, "Audio Memory: %.2f MB", (float)EngineMain::instance().getAudioOut().getAudioPlayer().getMemoryUsage() / 1048576.0f));
	drawer.printText(font, Recti(FTX::screenWidth() - 200, 25, 0, 0), String(0, "%d sounds playing", EngineMain::instance().getAudioOut().getAudioPlayer().getNumPlayingSounds()));

	drawer.performRendering();
}
