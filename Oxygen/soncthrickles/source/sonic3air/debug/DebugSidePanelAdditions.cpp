/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/debug/DebugSidePanelAdditions.h"
#include "sonic3air/audio/AudioOut.h"

#include "oxygen/application/Application.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/mainview/GameView.h"
#include "oxygen/application/overlays/DebugSidePanel.h"
#include "oxygen/application/overlays/DebugSidePanelCategory.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"


namespace s3air
{
	enum CategoryIdentifier
	{
		CATEGORY_MISSING_SFX	= 0x100,
		CATEGORY_GAME_OBJECTS	= 0x101
	};

	static void buildCategoryGameObjects(DebugSidePanelCategory& category, DebugSidePanel::Builder& builder, uint64 mouseOverKey)
	{
		Drawer& drawer = EngineMain::instance().getDrawer();

		const bool showUpdateRoutine = (category.mOpenKeys.count(1) > 0);
		const bool showPosition		 = (category.mOpenKeys.count(2) > 0);
		const bool showSubType		 = (category.mOpenKeys.count(3) > 0);

		builder.addLine(String(0, "[%c] Show update routine", showUpdateRoutine ? 'x' : ' '), Color::CYAN, 0, 1);
		builder.addLine(String(0, "[%c] Show position",		  showPosition ? 'x' : ' '), Color::CYAN, 0, 2);
		builder.addLine(String(0, "[%c] Show subtype",		  showSubType ? 'x' : ' '), Color::CYAN, 0, 3);
		builder.addSpacing(12);

		for (uint32 address = 0xffffb000; address < 0xffffcfcc; address += 0x4a)
		{
			const uint32 objectUpdateRoutine = EmulatorInterface::instance().readMemory32(address);
			if (objectUpdateRoutine == 0)
				continue;

			const uint16 positionX = EmulatorInterface::instance().readMemory16(address + 0x10);
			const uint16 positionY = EmulatorInterface::instance().readMemory16(address + 0x14);

			const uint64 key = address;
			builder.addLine(String(0, "0x%08x", address), Color::WHITE, 0, key);
			if (showUpdateRoutine)
				builder.addLine(String(0, "- Update routine: 0x%06x", objectUpdateRoutine), Color::WHITE, 8, key);
			if (showPosition)
				builder.addLine(String(0, "- Position: 0x%04x, 0x%04x", positionX, positionY), Color::WHITE, 8, key);
			if (showSubType)
				builder.addLine(String(0, "- Subtype: 0x%02x", EmulatorInterface::instance().readMemory8(address + 0x2c)), Color::WHITE, 8, key);

			if (mouseOverKey == key)
			{
				const Vec2i worldSpaceOffset = RenderParts::instance().getSpriteManager().getWorldSpaceOffset();
				Recti objectRect(positionX - 15, positionY - 15, 31, 31);
				objectRect.x -= worldSpaceOffset.x;
				objectRect.y -= worldSpaceOffset.y;

				Rectf translatedRect;
				Application::instance().getGameView().translateRectIntoScreenCoords(translatedRect, objectRect);
				drawer.drawRect(translatedRect, Color(0.0f, 0.5f, 1.0f, 0.75f));
			}
			builder.addSpacing(4);
		}
	}

	void registerDebugSidePanelAdditions(DebugSidePanel& debugSidePanel)
	{
		debugSidePanel.createGameCategory(CATEGORY_GAME_OBJECTS, "GAME OBJECTS", 'O', buildCategoryGameObjects).mOpenKeys.insert(1);
	}
}
