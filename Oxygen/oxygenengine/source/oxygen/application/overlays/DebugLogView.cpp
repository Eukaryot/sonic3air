/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/DebugLogView.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"


DebugLogView::DebugLogView()
{
}

DebugLogView::~DebugLogView()
{
}

void DebugLogView::initialize()
{
	// Debug output font
	mFont.setSize(15.0f);
	mFont.addFontProcessor(std::make_shared<ShadowFontProcessor>(Vec2i(1, 1), 1.0f));
}

void DebugLogView::deinitialize()
{
}

void DebugLogView::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);
}

void DebugLogView::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);
}

void DebugLogView::render()
{
	GuiBase::render();
	setRect(FTX::screenRect());

	Drawer& drawer = EngineMain::instance().getDrawer();
	DebugTracking& debugTracking = Application::instance().getSimulation().getCodeExec().getDebugTracking();

	// Part 1: Script log entries
	{
		const auto& entries = debugTracking.getScriptLogEntries();
		if (!entries.empty())
		{
			// Define rect
			Recti logRect;
			int maxKeyWidth = 80;
			int maxContentWidth = 80;
			{
				int height = 20;
				for (const auto& pair : entries)
				{
					const DebugTracking::ScriptLogEntry& entry = pair.second;
					const bool hasCurrent = (entry.mLastUpdate >= Application::instance().getSimulation().getFrameNumber() - 1);
					height += hasCurrent ? ((int)entry.mEntries.size() * 16 + 2) : 18;

					maxKeyWidth = std::max(maxKeyWidth, mFont.getWidth(pair.first.c_str()) + 20);
					for (const DebugTracking::ScriptLogSingleEntry& singleEntry : entry.mEntries)
					{
						maxContentWidth = std::max(maxContentWidth, mFont.getWidth(singleEntry.mValue.c_str()));
					}
				}
				logRect.set(5, (int)mRect.height - height - 25, 20 + maxKeyWidth + maxContentWidth, height);
			}

			drawer.drawRect(logRect, Color(0.0f, 0.0f, 0.0f, 0.5f));

			Recti rect = logRect;
			rect.y += 10;
			for (const auto& pair : entries)
			{
				const DebugTracking::ScriptLogEntry& entry = pair.second;
				const bool hasCurrent = (entry.mLastUpdate >= Application::instance().getSimulation().getFrameNumber() - 1);

				const float brightness = hasCurrent ? 1.0f : 0.6f;
				const Color color(brightness, brightness, brightness);

				rect.x = logRect.x + 10;
				drawer.printText(mFont, rect, (pair.first + ":"), 1, color);

				rect.x += maxKeyWidth;
				if (hasCurrent)
				{
					for (const DebugTracking::ScriptLogSingleEntry& singleEntry : entry.mEntries)
					{
						drawer.printText(mFont, rect, singleEntry.mValue, 1, color);
						rect.y += 16;
					}
					rect.y += 2;
				}
				else
				{
					drawer.printText(mFont, rect, entry.mEntries.back().mValue, 1, color);
					rect.y += 18;
				}
			}
		}
	}

	// Part 2: Color log entries
	{
		const DebugTracking::ColorLogEntryArray& entries = debugTracking.getColorLogEntries();
		if (!entries.empty())
		{
			// Define rect
			Recti logRect(550, 30, 192, 200);
			drawer.drawRect(logRect, Color(0.0f, 0.0f, 0.0f, 0.5f));

			Recti rect = logRect;
			rect.y += 10;
			for (const DebugTracking::ColorLogEntry& entry : entries)
			{
				drawer.printText(mFont, rect, entry.mName, 1, Color::WHITE);
				rect.y += 20;

				Recti colorRect(rect.x + 1, rect.y + 1, 10, 10);
				for (int k = 0; k < (int)entry.mColors.size(); ++k)
				{
					drawer.drawRect(colorRect, entry.mColors[k]);
					if ((k % 16) < 15)
					{
						colorRect.x += 12;
					}
					else
					{
						rect.y += 12;
						colorRect.x = rect.x + 1;
						colorRect.y = rect.y + 1;
					}
				}
				rect.y += 20;
			}
		}
	}

	drawer.performRendering();
}
