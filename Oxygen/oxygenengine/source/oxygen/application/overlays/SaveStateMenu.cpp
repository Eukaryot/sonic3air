/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/SaveStateMenu.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/Simulation.h"


SaveStateMenu::SaveStateMenu() :
	mIsActive(false),
	mForLoading(true),
	mHighlightedIndex(0)
{
	mFont.setSize(18.0f);
	mFont.setShadow(true, Vec2f(2,2), 0.5f);
}

SaveStateMenu::~SaveStateMenu()
{
}

void SaveStateMenu::init(bool forLoading)
{
	mSaveStateDirectory[0] = Configuration::instance().mSaveStatesDir;
	mSaveStateDirectory[1] = Configuration::instance().mSaveStatesDirLocal;

	mForLoading = forLoading;
	mHadFirstUpdate = false;
	mEntries.clear();

	// Gather save state lists
	bool addPadding = false;
	for (int type = 1; type >= 0; --type)
	{
		FileCrawler fc;
		fc.addFiles(mSaveStateDirectory[type] + L"/*.state");
		for (size_t i = 0; i < fc.size(); ++i)
		{
			std::wstring name = fc[i]->mFilename;
			name.erase(name.length() - 6);		// Remove ".state"
			addEntry(name, (Entry::Type)type, addPadding ? 12 : 0);
			addPadding = false;
		}
		if (type == 1)
			addPadding = (fc.size() > 0);
	}

	if (forLoading)
	{
		// Add reset options
		addEntry(L"Reset", Entry::Type::RESET, 12);
	}
	else
	{
		// Add "New Savegame" option
		WString name;
		for (uint32 number = 1; number <= 99; ++number)
		{
			name.format(L"state_%02d", number);
			bool found = false;
			for (const Entry& entry : mEntries)
			{
				found = (entry.mName == *name);
				if (found)
					break;
			}
			if (!found)
				break;
		}
		addEntry(*name, Entry::Type::NEWSAVE, 12);
	}

	// Set highlighted index depending on what was selected last time
	uint32 highlightedIndex = 0;
	if (!mHighlightedName.empty())
	{
		for (uint32 i = 0; i < (uint32)mEntries.size(); ++i)
		{
			if (mEntries[i].mName == mHighlightedName)
			{
				highlightedIndex = i;
				break;
			}
		}
	}
	setHighlightedIndex(highlightedIndex);
}

void SaveStateMenu::initialize()
{
	mIsActive = true;
	mEditing = false;
}

void SaveStateMenu::deinitialize()
{
	mHighlightedName = (mHighlightedIndex < mEntries.size()) ? mEntries[mHighlightedIndex].mName : L"";
	mIsActive = false;
}

void SaveStateMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	if (ev.state)
	{
		switch (ev.key)
		{
			case SDLK_RETURN:
			{
				onAccept(true, true);
				InputManager::instance().updateInput(0.0f);		// This clears the changed state of "Enter"
				ControlsIn::instance().setAllIgnores();	// Just to make sure any current key pressed (especially "Enter") won't have an effect in the simulation
				break;
			}

			case SDLK_ESCAPE:
			{
				((Application*)mParent)->childClosed(*this);
				break;
			}

			case SDLK_F5:
			{
				onAccept(false, true);
				break;
			}

			case SDLK_F8:
			{
				onAccept(true, false);
				break;
			}

			default:
			{
				if (mHighlightedIndex < mEntries.size())
				{
					Entry& entry = mEntries[mHighlightedIndex];
					if (ev.key == SDLK_F2)
					{
						mEditing = (entry.mType == Entry::Type::NEWSAVE);
					}
					else if (ev.key == SDLK_BACKSPACE && mEditing)
					{
						std::wstring& text = entry.mName;
						if (!text.empty())
						{
							text.erase(text.length() - 1);
						}
					}
				}
			}
		}
	}
}

void SaveStateMenu::textinput(const rmx::TextInputEvent& ev)
{
	if (mEditing && mHighlightedIndex < mEntries.size())
	{
		Entry& entry = mEntries[mHighlightedIndex];
		entry.mName += *ev.text;
	}
}

void SaveStateMenu::update(float timeElapsed)
{
	if (!mHadFirstUpdate)
	{
		mHadFirstUpdate = true;
		return;
	}

	const InputManager::ControllerScheme& controller = InputManager::instance().getController(0);

	if (controller.Up.justPressedOrRepeat())
	{
		changeHighlightedIndex(-1);
	}
	else if (controller.Down.justPressedOrRepeat())
	{
		changeHighlightedIndex(+1);
	}

	if (!mEditing)
	{
		if (controller.A.justPressed())
		{
			onAccept(true, true);
		}
		else if (controller.B.justPressed() || controller.Back.justPressed())
		{
			((Application*)mParent)->childClosed(*this);
		}
	}
}

void SaveStateMenu::render()
{
	Drawer& drawer = EngineMain::instance().getDrawer();

	drawer.drawRect(FTX::screenRect(), Color::fromABGR32(0xe0000000));

	Rectf rect((float)(FTX::screenWidth() / 2 - 280), 30, 0, 0);
	drawer.printText(mFont, rect, mForLoading ? "LOAD STATE" : "SAVE STATE");
	rect.addPos(16, 40);

	int highlightedPositionY = 0;
	for (uint32 index = 0; index < (uint32)mEntries.size(); ++index)
	{
		const Entry& entry = mEntries[index];

		rect.y += entry.mPaddingBefore;

		WString text = entry.mName.c_str();
		if (entry.mType == Entry::Type::NEWSAVE)
			text.insert(L"+ ", 0);
		else if (entry.mType == Entry::Type::RESET)
			text.insert(L"- ", 0);

		Color color = Color::fromABGR32(0xe0c0c0c0);
		if (mHighlightedIndex == index)
		{
			color = Color::YELLOW;
			highlightedPositionY = roundToInt(rect.y) + 12;
			if (mEditing && std::fmod(FTX::System->getTime(), 1.0f) < 0.5f)
				text.add('_');
			drawer.printText(mFont, rect, "> ", 3, color);
		}
		else if (entry.mType == Entry::Type::SAVESTATE_LOCAL)
		{
			color.setABGR32(0xe0e0e080);
		}
		else if (entry.mType == Entry::Type::SAVESTATE_EMULATOR)
		{
			color.setABGR32(0xe0e08080);
		}
		else if (entry.mType == Entry::Type::NEWSAVE || entry.mType == Entry::Type::RESET)
		{
			color.setABGR32(0xe0c0c0c0);
		}

		drawer.printText(mFont, rect, text.toString(), 1, color);
		rect.y += 28;
	}

	if (mHasPreview && mPreview.isValid())
	{
		Recti rct((FTX::screenWidth() - mPreview.getWidth()) / 2 + 210, highlightedPositionY - mPreview.getHeight() / 2, mPreview.getWidth(), mPreview.getHeight());
		rct.y = clamp(rct.y, 20, FTX::screenHeight() - mPreview.getHeight() - 20);
		drawer.drawRect(rct, mPreview);
	}

	drawer.performRendering();
}

void SaveStateMenu::addEntry(const std::wstring& name, Entry::Type type, int padding)
{
	Entry& entry = vectorAdd(mEntries);
	entry.mName = name;
	entry.mType = type;
	entry.mPaddingBefore = padding;
}

const std::wstring& SaveStateMenu::getSaveStatesDirByType(Entry::Type type)
{
	if (type == Entry::Type::SAVESTATE_EMULATOR)
	{
		return mSaveStateDirectory[0];
	}
	else
	{
		return mSaveStateDirectory[1];
	}
}

void SaveStateMenu::setHighlightedIndex(uint32 highlightedIndex)
{
	mHighlightedIndex = highlightedIndex;
	mHasPreview = false;

	// Try to load image
	if (mHighlightedIndex < mEntries.size())
	{
		const Entry& entry = mEntries[mHighlightedIndex];
		if (entry.mType <= Entry::Type::SAVESTATE_LOCAL)
		{
			Bitmap bmp;
			if (bmp.load(mSaveStateDirectory[(size_t)entry.mType] + L"/" + entry.mName + L".state.bmp"))
			{
				if (!mPreview.isValid())
				{
					EngineMain::instance().getDrawer().createTexture(mPreview);
				}
				mPreview.accessBitmap() = bmp;
				mPreview.bitmapUpdated();
				mHasPreview = true;
			}
		}
	}
}

void SaveStateMenu::changeHighlightedIndex(int difference)
{
	if (!mEntries.empty())
	{
		setHighlightedIndex((mHighlightedIndex + difference + (uint32)mEntries.size()) % mEntries.size());
	}
	mEditing = false;
}

void SaveStateMenu::onAccept(bool loadingAllowed, bool savingAllowed)
{
	Simulation& simulation = Application::instance().getSimulation();
	if (mForLoading)
	{
		if (loadingAllowed && mHighlightedIndex < mEntries.size())
		{
			// Disable game mode checks
			EngineMain::getDelegate().onPreSaveStateLoad();

			const Entry& entry = mEntries[mHighlightedIndex];
			if (entry.mType <= Entry::Type::SAVESTATE_LOCAL)
			{
				const WString filename = WString(mSaveStateDirectory[(size_t)entry.mType]) + entry.mName + L".state";
				simulation.loadState(*filename);
			}
			else if (entry.mType == Entry::Type::RESET)
			{
				simulation.resetState();
			}
		}
	}
	else
	{
		if (savingAllowed && mHighlightedIndex < mEntries.size())
		{
			const Entry& entry = mEntries[mHighlightedIndex];
			if (entry.mType != Entry::Type::RESET && !entry.mName.empty())
			{
				const WString filename = WString(mSaveStateDirectory[1]) + entry.mName + L".state";
				simulation.saveState(*filename);
			}
		}
	}

	((Application*)mParent)->childClosed(*this);
}
