/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/context/ApplicationContextMenu.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/base/PlatformFunctions.h"


ApplicationContextMenu::ApplicationContextMenu()
{
}

ApplicationContextMenu::~ApplicationContextMenu()
{
}

void ApplicationContextMenu::initialize()
{
	GuiBase::initialize();

	if (mItems.empty())
	{
		#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC)
			#define DIRECTORY_STRING "folder"
		#else
			#define DIRECTORY_STRING "directory"
		#endif

		mItems.emplace_back(Item { "Open saved data " DIRECTORY_STRING, Item::Function::OPEN_SAVED_DATA_DIRECTORY });
		mItems.emplace_back(Item { "Open mods " DIRECTORY_STRING,		Item::Function::OPEN_MODS_DIRECTORY });
	#if 0
		// TODO: This does not work well on Windows
		mItems.emplace_back(Item { "Open log file",						Item::Function::OPEN_LOGFILE });
	#endif

		mBaseInnerRect.set(2, 24, 210, 24);
		mBaseOuterSize.set(mBaseInnerRect.width + 4, (int)(mItems.size()) * 24 + 28);
	}
}

void ApplicationContextMenu::mouse(const rmx::MouseEvent& ev)
{
	if (ev.state)
	{
		if (ev.button == rmx::MouseButton::Right && !FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
		{
			mActive = !mActive;
			if (mActive)
			{
				// Determine a good position where to show the context menu, so it stays inside the game window
				mContextMenuPosition = ev.position;
				if (mContextMenuPosition.x + mBaseOuterSize.x > FTX::screenWidth())
				{
					mContextMenuPosition.x = std::max(mContextMenuPosition.x - mBaseOuterSize.x, 0);
				}
				if (mContextMenuPosition.y + mBaseOuterSize.y > FTX::screenHeight())
				{
					mContextMenuPosition.y = std::max(mContextMenuPosition.y - mBaseOuterSize.y, 0);
				}
			}
			mContextMenuClick.set(-1, -1);
			return;
		}
		else if (ev.button == rmx::MouseButton::Left)
		{
			if (mActive)
			{
				mContextMenuClick = ev.position;
				return;
			}
		}
	}

	GuiBase::mouse(ev);
}

void ApplicationContextMenu::update(float timeElapsed)
{
	GuiBase::update(timeElapsed);
}

void ApplicationContextMenu::render()
{
	GuiBase::render();

	if (mActive)
	{
		Drawer& drawer = EngineMain::instance().getDrawer();

		Recti rect(mContextMenuPosition.x, mContextMenuPosition.y, mBaseOuterSize.x, mBaseOuterSize.y);
		drawer.drawRect(rect, Color(0.3f, 0.3f, 0.3f));

		rect.addPos(mBaseInnerRect.getPos());
		rect.setSize(mBaseInnerRect.getSize());

		{
			const Recti textRect(rect.x + 5, rect.y - 21, 0, rect.height);
			drawer.printText(global::mFont10, textRect, "Sonic 3 A.I.R.", 4);
		}

		Item* clickedItem = nullptr;
		for (Item& item : mItems)
		{
			rect.y += 2;
			const bool selected = rect.contains(FTX::mousePos());
			if (mContextMenuClick.x >= 0 && rect.contains(mContextMenuClick))
			{
				clickedItem = &item;
			}

			drawer.drawRect(rect, selected ? Color(0.4f, 0.4f, 0.1f) : Color(0.1f, 0.1f, 0.1f));

			const Recti textRect(rect.x + 10, rect.y + 1, 0, rect.height);
			drawer.printText(global::mFont10, textRect, item.mText, 4);
			rect.y += 22;
		}

		if (mContextMenuClick.x >= 0)
		{
			if (nullptr != clickedItem)
			{
				switch (clickedItem->mFunction)
				{
					case Item::Function::OPEN_SAVED_DATA_DIRECTORY:
						PlatformFunctions::openDirectoryExternal(Configuration::instance().mAppDataPath);
						break;

					case Item::Function::OPEN_MODS_DIRECTORY:
						PlatformFunctions::openDirectoryExternal(Configuration::instance().mAppDataPath + L"mods/");
						break;

					case Item::Function::OPEN_LOGFILE:
						PlatformFunctions::openFileExternal(Configuration::instance().mAppDataPath + L"logfile.txt");
						break;
				}
			}

			// Close context menu again in any case
			mActive = false;
		}

		drawer.performRendering();
	}
}
