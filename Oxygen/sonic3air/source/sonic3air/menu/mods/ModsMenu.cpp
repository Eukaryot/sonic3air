/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/mods/ModsMenu.h"
#include "sonic3air/menu/GameApp.h"
#include "sonic3air/menu/MenuBackground.h"
#include "sonic3air/menu/entries/GeneralMenuEntries.h"
#include "sonic3air/menu/mods/ModsMenuEntries.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/helper/DrawingUtils.h"
#include "sonic3air/ConfigurationImpl.h"
#include "sonic3air/Game.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/DrawerHelper.h"
#include "oxygen/helper/FileHelper.h"


namespace
{
	static constexpr int BACK = 0xffff;

	void moveFloatTowards(float& value, float target, float maxStep)
	{
		if (value != target)
		{
			if (value < target)
				value = std::min(value + maxStep, target);
			else
				value = std::max(value - maxStep, target);
		}
	}
}


ModsMenu::ModsMenu(MenuBackground& menuBackground) :
	mMenuBackground(&menuBackground)
{
}

ModsMenu::~ModsMenu()
{
}

GameMenuBase::BaseState ModsMenu::getBaseState() const
{
	switch (mState)
	{
		case State::APPEAR:			  return BaseState::FADE_IN;
		case State::SHOW:			  return BaseState::SHOW;
		case State::APPLYING_CHANGES: return BaseState::SHOW;
		case State::FADE_TO_MENU:	  return BaseState::FADE_OUT;
		default:					  return BaseState::INACTIVE;
	}
}

void ModsMenu::setBaseState(BaseState baseState)
{
	switch (baseState)
	{
		case BaseState::INACTIVE: mState = State::INACTIVE;  break;
		case BaseState::FADE_IN:  mState = State::APPEAR;  break;
		case BaseState::SHOW:	  mState = State::SHOW;  break;
		case BaseState::FADE_OUT: mState = State::FADE_TO_MENU;  break;
	}
}

void ModsMenu::onFadeIn()
{
	mState = State::APPEAR;
	mFadeInDelay = 0.2f;

	mMenuBackground->showPreview(false, false);
	mMenuBackground->startTransition(MenuBackground::Target::ALTER);

	AudioOut::instance().setMenuMusic(0x2f);

	refreshControlsDisplay();
}

bool ModsMenu::canBeRemoved()
{
	return (mState == State::INACTIVE && mVisibility <= 0.0f);
}

void ModsMenu::initialize()
{
	// Rescan for new / removed mods
	ModManager& modManager = ModManager::instance();
	const bool anyChange = modManager.rescanMods();

	std::vector<Mod*> allMods = modManager.getAllMods();
	std::sort(allMods.begin(), allMods.end(), [](const Mod* a, const Mod* b) { return (a->mDisplayName < b->mDisplayName); } );
	const std::vector<Mod*>& activeMods = modManager.getActiveMods();

	// Rebuild list of mods
	if (anyChange || (mModEntries.empty() && !allMods.empty()))
	{
		mModEntries.clear();
		mModEntries.reserve(allMods.size());

		// Pass 1: Add active mods, they should go first in our list
		//  -> But reverse the order, as highest priority are the last (not first!) mods in the active list
		for (auto it = activeMods.rbegin(); it != activeMods.rend(); ++it)
		{
			Mod* mod = *it;
			RMX_ASSERT(mod->mState == Mod::State::ACTIVE, "Active mod is not really active");

			ModEntry& entry = vectorAdd(mModEntries);
			entry.mMod = mod;
			entry.mMakeActive = true;
		}

		// Pass 2: Add inactive mods, incl. failed ones
		for (Mod* mod : allMods)
		{
			if (mod->mState != Mod::State::ACTIVE)
			{
				ModEntry& entry = vectorAdd(mModEntries);
				entry.mMod = mod;
				entry.mMakeActive = false;
			}
		}
	}

	for (int i = 0; i < 2; ++i)
	{
		mTabs[i].mMenuEntries.clear();
	}

	mHasAnyMods = !mModEntries.empty();
	if (mHasAnyMods)
	{
		// Build or re-use mod resources
		for (ModEntry& modEntry : mModEntries)
		{
			const auto it = mModResources.find(modEntry.mMod);
			if (it != mModResources.end())
			{
				modEntry.mModResources = &it->second;
			}
			else
			{
				ModResources& res = mModResources[modEntry.mMod];
				modEntry.mModResources = &res;

				// Load icons
				Bitmap icon16px;
				Bitmap icon64px;
				{
					FileHelper::loadBitmap(icon16px, modEntry.mMod->mFullPath + L"icon-16px.png", false);
					FileHelper::loadBitmap(icon64px, modEntry.mMod->mFullPath + L"icon-64px.png", false);
					if (icon64px.empty())
						FileHelper::loadBitmap(icon64px, modEntry.mMod->mFullPath + L"icon.png", false);
				}

				Bitmap* sourceLarge = !icon64px.empty() ? &icon64px : !icon16px.empty() ? &icon16px : nullptr;
				if (nullptr != sourceLarge)
				{
					EngineMain::instance().getDrawer().createTexture(res.mLargeIcon);
					res.mLargeIcon.accessBitmap().rescale(*sourceLarge, 64, 64);
					res.mLargeIcon.bitmapUpdated();
				}

				Bitmap* sourceSmall = !icon16px.empty() ? &icon16px : !icon64px.empty() ? &icon64px : nullptr;
				if (nullptr != sourceSmall)
				{
					EngineMain::instance().getDrawer().createTexture(res.mSmallIcon);
					res.mSmallIcon.accessBitmap().rescale(*sourceSmall, 16, 16);
					res.mSmallIcon.bitmapUpdated();

					EngineMain::instance().getDrawer().createTexture(res.mSmallIconGray);
					{
						Bitmap& bitmap = res.mSmallIconGray.accessBitmap();
						bitmap = res.mSmallIcon.accessBitmap();
						uint32* data = bitmap.getData();

						// Convert to grayscale
						const int pixels = bitmap.getPixelCount();
						for (int i = 0; i < pixels; ++i)
						{
							data[i] = (data[i] & 0xff000000) + (roundToInt(Color::fromABGR32(data[i]).getGray() * 255.0f) * 0x10101);
						}
						res.mSmallIconGray.bitmapUpdated();
					}
				}
			}
		}

		// Build menu entries
		{
			GameMenuEntries& entries = mTabs[0].mMenuEntries;
			entries.clear();
			entries.reserve(activeMods.size());

			// Add active mods
			//  -> In reverse order, as highest priority are the last (not first!) mods in the active list
			for (auto it = activeMods.rbegin(); it != activeMods.rend(); ++it)
			{
				Mod* mod = *it;
				for (size_t index = 0; index < mModEntries.size(); ++index)
				{
					if (mModEntries[index].mMod == mod)
					{
						entries.addEntry<ModMenuEntry>().initEntry(*mod, *mModEntries[index].mModResources, (uint32)index);
						break;
					}
				}
			}
		}

		{
			GameMenuEntries& entries = mTabs[1].mMenuEntries;
			entries.clear();
			entries.reserve(allMods.size() - activeMods.size());

		#if 0
			// Filter
			entries.addEntry<InputFieldMenuEntry>().initEntry(Vec2i(200, 15), L"", L"Filter mods...");
			entries.mSelectedEntryIndex = 1;
		#endif

			// Add inactive mods (in no special order)
			for (size_t index = 0; index < mModEntries.size(); ++index)
			{
				const ModEntry& modEntry = mModEntries[index];
				if (!modEntry.mMakeActive)
				{
					entries.addEntry<ModMenuEntry>().initEntry(*modEntry.mMod, *mModEntries[index].mModResources, (uint32)index);
				}
			}
		}

		mActiveTab = (mTabs[0].mMenuEntries.size() != 0) ? 0 : 1;
	}
	else
	{
		mModsStartPage.initialize();
		mActiveTab = 0;
	}

	refreshAllDependencies();

	mInfoOverlay.mShouldBeVisible = false;
	mInfoOverlay.mVisibility = 0.0f;
}

void ModsMenu::deinitialize()
{
}

void ModsMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	GameMenuEntry* entry = getSelectedGameMenuEntry();
	if (nullptr != entry)
		entry->keyboard(ev);
}

void ModsMenu::textinput(const rmx::TextInputEvent& ev)
{
	GameMenuEntry* entry = getSelectedGameMenuEntry();
	if (nullptr != entry)
		entry->textinput(ev);
}

void ModsMenu::update(float timeElapsed)
{
	GameMenuBase::update(timeElapsed);

	mActiveTabAnimated += clamp((float)mActiveTab - mActiveTabAnimated, -timeElapsed * 6.0f, timeElapsed * 6.0f);
	bool preventScrolling = false;

	// Don't react to input during transitions
	if (mState == State::SHOW)
	{
		const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);

		if (mHasAnyMods)
		{
			GameMenuEntries& menuEntries = mTabs[mActiveTab].mMenuEntries;

			bool processInput = true;
			if (menuEntries.hasSelected() && menuEntries.selected().is<InputFieldMenuEntry>())
			{
				processInput = keys.Up.justPressed() || keys.Down.justPressed();
			}

			if (processInput)
			{
				enum class ButtonEffect
				{
					NONE,
					SWITCH_TAB,
					TOGGLE_INFO,
					BACK
				};
				const ButtonEffect buttonEffect = (keys.Right.justPressed() && mActiveTab == 0) ? ButtonEffect::SWITCH_TAB :
												  (keys.Left.justPressed() && mActiveTab == 1) ? ButtonEffect::SWITCH_TAB :
												  (keys.Y.justPressed()) ? ButtonEffect::TOGGLE_INFO :
												  (keys.Start.justPressed() || keys.Back.justPressed() || keys.B.justPressed()) ? ButtonEffect::BACK : ButtonEffect::NONE;

				// Update menu entries
				const int previousSelectedEntryIndex = menuEntries.mSelectedEntryIndex;
				GameMenuEntries::UpdateResult result = GameMenuEntries::UpdateResult::NONE;

				if (keys.X.isPressed() && (keys.Up.justPressedOrRepeat() || keys.Down.justPressedOrRepeat()))
				{
					// Quick navigation while holding X (can be combined with mod movement)
					const int entryChange = menuEntries.getEntryChangeByInput();
					if (entryChange != 0)
					{
						const constexpr int NUM_QUICK_NAV_STEPS = 3;
						for (int k = 0; k < NUM_QUICK_NAV_STEPS; ++k)
						{
							if (!menuEntries.changeSelectedIndex(entryChange, false))
								break;
						}
						result = GameMenuEntries::UpdateResult::ENTRY_CHANGED;
					}
				}
				else
				{
					result = menuEntries.update();
				}

				if (result != GameMenuEntries::UpdateResult::NONE)
				{
					bool playSound = true;
					if (result == GameMenuEntries::UpdateResult::ENTRY_CHANGED)
					{
						if (keys.A.isPressed())
						{
							int diff = menuEntries.mSelectedEntryIndex - previousSelectedEntryIndex;
							if (!menuEntries.hasSelected() || !menuEntries.selected().is<ModMenuEntry>() || !menuEntries.isValidIndex(previousSelectedEntryIndex) || !menuEntries[previousSelectedEntryIndex].is<ModMenuEntry>())
								diff = 0;

							if (diff < 0 && keys.Up.isPressed())
							{
								const int indexA = previousSelectedEntryIndex;
								const int indexB = menuEntries.mSelectedEntryIndex;

								// Exchange entry with previous one, effectively moving that one
								for (int i = indexA; i > indexB; --i)
									menuEntries.swapEntries(i, i - 1);

								if (mActiveTab == 0)
								{
									for (int i = indexA; i >= indexB; --i)
										refreshDependencies(menuEntries[i].as<ModMenuEntry>(), i);
								}

								// For animation
								for (int i = indexA; i > indexB; --i)
									menuEntries[i].mAnimation.mOffset.y = -1.0f;
								menuEntries[indexB].mAnimation.mOffset.y = -(float)diff;
							}
							else if (diff > 0 && keys.Down.isPressed())
							{
								const int indexA = previousSelectedEntryIndex;
								const int indexB = menuEntries.mSelectedEntryIndex;

								// Exchange entry with previous one, effectively moving that one
								for (int i = indexA; i < indexB; ++i)
									menuEntries.swapEntries(i, i + 1);

								if (mActiveTab == 0)
								{
									for (int i = indexA; i <= indexB; ++i)
										refreshDependencies(menuEntries[i].as<ModMenuEntry>(), i);
								}

								// For animation
								for (int i = indexA; i < indexB; ++i)
									menuEntries[i].mAnimation.mOffset.y = 1.0f;
								menuEntries[indexB].mAnimation.mOffset.y = -(float)diff;
							}
							else
							{
								// Reset change
								menuEntries.mSelectedEntryIndex = previousSelectedEntryIndex;
								playSound = false;
							}
						}
					}

					if (playSound)
					{
						playMenuSound(0x5b);
					}
				}

				switch (buttonEffect)
				{
					case ButtonEffect::SWITCH_TAB:
					{
						if (keys.A.isPressed())
						{
							// Make entry active or inactive
							//  -> Note that mActiveTab is still the old tab here
							if (menuEntries.empty())
								break;

							const bool makeActive = (mActiveTab != 0);
							Tab& newTab = mTabs[1 - mActiveTab];

							GameMenuEntry& entry = menuEntries.selected();
							ModEntry& modEntry = mModEntries[entry.mData];

							if (makeActive && modEntry.mMod->mState == Mod::State::FAILED)
							{
								playMenuSound(0xb2);
								break;
							}

							modEntry.mMakeActive = makeActive;

							if (newTab.mMenuEntries.mSelectedEntryIndex >= (int)newTab.mMenuEntries.size())
								newTab.mMenuEntries.mSelectedEntryIndex = std::max<int>(1, (int)newTab.mMenuEntries.size()) - 1;
							newTab.mMenuEntries.insertByReference(entry, newTab.mMenuEntries.mSelectedEntryIndex);
							menuEntries.erase(menuEntries.mSelectedEntryIndex);

							if (!makeActive)
							{
								clearDependencies(entry.as<ModMenuEntry>());
							}

							// For animation
							entry.mAnimation.mOffset.x = makeActive ? 1.0f : -1.0f;
							for (size_t k = newTab.mMenuEntries.mSelectedEntryIndex + 1; k < newTab.mMenuEntries.size(); ++k)
								newTab.mMenuEntries[k].mAnimation.mOffset.y = -1.0f;
							for (size_t k = menuEntries.mSelectedEntryIndex; k < menuEntries.size(); ++k)
								menuEntries[k].mAnimation.mOffset.y = 1.0f;

							refreshAllDependencies();
						}

						playMenuSound(0x5b);

						mActiveTab = 1 - mActiveTab;
						preventScrolling = true;
						refreshControlsDisplay();
						break;
					}

					case ButtonEffect::TOGGLE_INFO:
					{
						mInfoOverlay.mShouldBeVisible = !mInfoOverlay.mShouldBeVisible;
						break;
					}

					case ButtonEffect::BACK:
					{
						if (mInfoOverlay.mShouldBeVisible)
						{
							mInfoOverlay.mShouldBeVisible = false;
						}
						else
						{
							goBack();
						}
						break;
					}

					default:
						break;
				}

				const bool inMovementMode = (keys.A.isPressed() && !mTabs[mActiveTab].mMenuEntries.empty());
				if (mInMovementMode != inMovementMode)
				{
					mInMovementMode = inMovementMode;
					refreshControlsDisplay();
				}
				else if (keys.X.hasChanged())
				{
					refreshControlsDisplay();
				}
			}
		}
		else
		{
			const bool result = mModsStartPage.update(timeElapsed);
			if (!result)
			{
				goBack();
			}
		}
	}

	// Animation
	for (int tabIndex = 0; tabIndex < 2; ++tabIndex)
	{
		for (size_t i = 0; i < mTabs[tabIndex].mMenuEntries.size(); ++i)
		{
			GameMenuEntry& entry = mTabs[tabIndex].mMenuEntries[i];
			if (entry.mAnimation.mOffset.x != 0.0f || entry.mAnimation.mOffset.y != 0.0f)
			{
				moveFloatTowards(entry.mAnimation.mOffset.x, 0.0f, timeElapsed * 6.0f);
				moveFloatTowards(entry.mAnimation.mOffset.y, 0.0f, timeElapsed * ((std::abs(entry.mAnimation.mOffset.y) <= 1.0f) ? 12.0f : 30.0f));
			}
		}
	}

	// Info overlay animation
	moveFloatTowards(mInfoOverlay.mVisibility, mInfoOverlay.mShouldBeVisible ? 1.0f : 0.0f, timeElapsed * 8.0f);
	if (mInfoOverlay.mVisibility > 0.0f)
	{
		GameMenuEntries& menuEntries = mTabs[mActiveTab].mMenuEntries;
		if (!menuEntries.hasSelected())
		{
			if (menuEntries.empty())
			{
				mInfoOverlay.mShouldBeVisible = false;
			}
			else
			{
				menuEntries.mSelectedEntryIndex = (int)menuEntries.size() - 1;
			}
		}
		else
		{
			ModEntry& modEntry = mModEntries[menuEntries.selected().mData];
			if (mInfoOverlay.mShownMod != modEntry.mMod)
			{
				global::mOxyfontTinySimple.wordWrapText(mInfoOverlay.mDescriptionLines, 290, modEntry.mMod->mDescription);
				mInfoOverlay.mShownMod = modEntry.mMod;
			}
		}
	}

	// Scrolling
	if (!preventScrolling)
	{
		for (int i = 0; i < 2; ++i)
		{
			mTabs[i].mScrolling.setVisibleAreaHeight((float)(224 - std::max(15, getInfoOverlayHeight())));
			mTabs[i].mScrolling.update(timeElapsed);
		}
	}

	// Fading in/out
	if (mState == State::APPEAR)
	{
		if (mFadeInDelay > 0.0f)
		{
			mFadeInDelay = std::max(mFadeInDelay - timeElapsed, 0.0f);
		}
		else
		{
			if (updateFadeIn(timeElapsed * 3.0f))
			{
				mState = State::SHOW;
			}
		}
	}
	else if (mState == State::APPLYING_CHANGES)
	{
		if (mApplyingChangesFrameCounter > 0)
			--mApplyingChangesFrameCounter;

		if (mApplyingChangesFrameCounter == 2)
		{
			applyModChanges();
		}
		else if (mApplyingChangesFrameCounter == 0)
		{
			playMenuSound(0xad);
			mMenuBackground->openMainMenu();
			mState = State::FADE_TO_MENU;
		}
	}
	else if (mState > State::APPLYING_CHANGES)
	{
		if (updateFadeOut(timeElapsed * 3.0f))
		{
			mState = State::INACTIVE;
		}
	}
}

void ModsMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();

	if (!mHasAnyMods)
	{
		mModsStartPage.render(drawer, mVisibility);
		return;
	}

	ModsMenuRenderContext renderContext;
	renderContext.mDrawer = &drawer;

	const int globalOffsetX = -roundToInt(saturate(1.0f - mVisibility) * 300.0f);

	int infoOverlayPosition = 224;
	if (mInfoOverlay.mVisibility > 0.0f)
	{
		infoOverlayPosition -= getInfoOverlayHeight();
		drawer.pushScissor(Recti(0, 0, 400, infoOverlayPosition));
	}

	const int minTabIndex = 0;
	const int maxTabIndex = 1;
	const int rightTabStart = roundToInt(interpolate(260.0f, 140.0f, mActiveTabAnimated));

	for (size_t tabIndex = minTabIndex; tabIndex <= maxTabIndex; ++tabIndex)
	{
		Tab& tab = mTabs[tabIndex];
		GameMenuEntries& menuEntries = tab.mMenuEntries;
		const int startY = 10 - tab.mScrolling.getScrollOffsetYInt();

		Recti rect((tabIndex == 0) ? 0 : rightTabStart, startY, 300, 18);
		rect.x += globalOffsetX + 20;
		float alpha = mVisibility;
		if (tabIndex != mActiveTab)
			alpha *= 0.75f;

		if (tabIndex == 0)
		{
			drawer.pushScissor(Recti(0, 0, rightTabStart + globalOffsetX, 224));
		}
		else
		{
			drawer.pushScissor(Recti(rightTabStart + globalOffsetX, 0, 400, 224));
		}

		{
			Recti visualRect = rect;
			visualRect.x += 24;
			std::string text = (tabIndex == 0) ? "Active Mods" : "Inactive Mods";
			if (tabIndex == mActiveTab)
			{
				text = "* " + text + " *";
				visualRect.x -= 12;
			}
			drawer.printText(global::mSonicFontB, visualRect, text, 1, Color(0.6f, 0.8f, 1.0f, alpha));
			rect.y += rect.height + 7;
		}

		const bool showPriorityHints = (tabIndex == 0 && mActiveTab == 0 && mInMovementMode && menuEntries.size() >= 2);
		if (showPriorityHints)
		{
			drawer.printText(global::mSmallfont, rect + Vec2i(-12, -12), "HIGH PRIORITY", 1, Color(0.4f, 0.5f, 0.6f, alpha * 0.25f));
		}

		if (menuEntries.empty())
		{
			Recti visualRect = rect;
			visualRect.x -= roundToInt(saturate(1.0f - mVisibility) * 300.0f);
			Color color = (tabIndex == mActiveTab) ? Color(1.0f, 1.0f, 1.0f, alpha) : Color(0.7f, 0.7f, 0.7f, alpha);
			drawer.printText(global::mOxyfontSmall, visualRect + Vec2i(24, 0), (tabIndex == 0) ? "- None -" : "- None -", 1, color);
		}

		for (size_t index = 0; index < menuEntries.size(); ++index)
		{
			GameMenuEntry& entry = menuEntries[index];
			const bool isSelected = ((int)index == menuEntries.mSelectedEntryIndex && tabIndex == mActiveTab);
			renderContext.mIsSelected = isSelected;

			const float lineOffset = (mState < State::SHOW) ? (224.0f - (float)rect.y - startY) : 0.0f;

			Recti visualRect = rect;
			visualRect.x -= roundToInt(saturate(1.0f - mVisibility - lineOffset / 500.0f) * 300.0f);
			visualRect.x += roundToInt(entry.mAnimation.mOffset.x * 200.0f);
			visualRect.y += roundToInt(entry.mAnimation.mOffset.y * rect.height);

			renderContext.mVisualRect = visualRect;
			renderContext.mCurrentPosition.set(visualRect.x, rect.y);

			if (entry.is<ModMenuEntry>())
			{
				const ModEntry* modEntry = (entry.mData < 0xfff0) ? &mModEntries[entry.mData] : nullptr;

				Color color = (tabIndex == mActiveTab) ? (isSelected ? (mInMovementMode ? Color(0.25f, 0.75f, 1.0f) : Color::YELLOW) : Color::WHITE) : Color(0.7f, 0.7f, 0.7f);
				color.a *= alpha;

				renderContext.mBaseColor = color;
				renderContext.mIsActiveModsTab = (tabIndex == 0);
				renderContext.mInMovementMode = mInMovementMode;
				renderContext.mNumModsInTab = menuEntries.size();

				if (nullptr != modEntry)
				{
					// Render this game menu entry
					entry.performRenderEntry(renderContext);
				}
				else
				{
					drawer.printText(global::mOxyfontSmall, visualRect + Vec2i(24, 0), entry.mText, 1, color);
				}

				if (mInMovementMode && tabIndex != mActiveTab && (int)index == menuEntries.mSelectedEntryIndex)
				{
					// Draw a line to show where the mod would be inserted
					drawer.drawRect(Recti(visualRect.x + 20, visualRect.y - 6, 100, 1), Color(0.25f, 0.75f, 1.0f));
					drawer.drawRect(Recti(visualRect.x + 21, visualRect.y - 5, 100, 1), Color(0.2f, 0.2f, 0.2f, 0.9f));
				}
			}
			else
			{
				entry.performRenderEntry(renderContext);
				rect.y = renderContext.mCurrentPosition.y;
			}

			const int currentAbsoluteY1 = (index == 0) ? 0 : (rect.y - startY);
			rect.y += rect.height;

			if (isSelected)
			{
				const int currentAbsoluteY2 = rect.y - startY;
				tab.mScrolling.setCurrentSelection(currentAbsoluteY1 - 20, currentAbsoluteY2 + 25);
			}
		}

		if (showPriorityHints)
		{
			drawer.printText(global::mSmallfont, rect + Vec2i(-12, -3), "LOW PRIORITY", 1, Color(0.4f, 0.5f, 0.6f, alpha * 0.25f));
		}

		drawer.popScissor();
	}

	// Draw separator line
	drawer.drawRect(Recti(rightTabStart + globalOffsetX, 8, 1, 208), Color(0.0f, 0.0f, 0.0f, 0.2f));

	if (mInfoOverlay.mVisibility > 0.0f)
	{
		drawer.popScissor();

		Recti rect(globalOffsetX, infoOverlayPosition, 400, 224 - infoOverlayPosition);
		drawer.drawRect(rect, Color(0.0f, 0.0f, 0.0f, mVisibility * 0.9f));
		drawer.drawRect(Recti(rect.x, rect.y - 1, rect.width, 1), Color(1.0f, 1.0f, 1.0f, mVisibility));

		if (mTabs[mActiveTab].mMenuEntries.hasSelected())
		{
			const auto& entry = mTabs[mActiveTab].mMenuEntries.selected();
			const ModEntry* modEntry = (entry.mData < 0xfff0) ? &mModEntries[entry.mData] : nullptr;
			if (nullptr != modEntry)
			{
				const Color colorW(1.0f, 1.0f, 1.0f, mVisibility);
				const Color colorY(1.0f, 1.0f, 0.75f, mVisibility);
				const Color colorY2(1.0f, 1.0f, 0.0f, mVisibility);
				const Color colorDark(0.4f, 0.4f, 0.4f, mVisibility * 0.75f);

				// Icon
				DrawerTexture& texture = modEntry->mModResources->mLargeIcon;
				if (texture.isValid())
				{
					const Recti iconRect(rect.x + 12, rect.y + 10, 64, 64);
					drawer.drawRect(iconRect, texture, colorW);
				}

				// Text
				rect.addPos(90, 14);
				rect.width -= 100;
				{
					WString path(L"mods/" + modEntry->mMod->mLocalDirectory);
					path.upperCase();
					drawer.printText(global::mSmallfont, rect + Vec2i(3, -10), path, 3, colorDark);
				}
				{
					drawer.printText(global::mOxyfontRegular, rect, modEntry->mMod->mDisplayName, 1, colorY2);
					rect.y += 19;
				}
				if (!modEntry->mMod->mAuthor.empty())
				{
					drawer.printText(global::mOxyfontSmallNoOutline, rect + Vec2i(8, 0), "by " + modEntry->mMod->mAuthor, 1, colorY);
					rect.y += 15;
				}
				if (!modEntry->mMod->mModVersion.empty())
				{
					drawer.printText(global::mOxyfontSmallNoOutline, rect + Vec2i(8, 0), "Version " + modEntry->mMod->mModVersion, 1, colorY);
					rect.y += 15;
				}
				rect.y += 4;
				if (!mInfoOverlay.mDescriptionLines.empty())
				{
					for (const std::wstring& line : mInfoOverlay.mDescriptionLines)
					{
						drawer.printText(global::mOxyfontTinySimple, rect, line, 1, colorW);
						rect.y += 11;
					}
					rect.y += 6;
				}
				if (!modEntry->mMod->mURL.empty())
				{
					drawer.printText(global::mOxyfontTinySimple, rect, "More info:   " + modEntry->mMod->mURL, 1, colorW);
					rect.y += 16;
				}
			}
		}
	}

	if (!renderContext.mSpeechBalloon.mText.empty() && mVisibility == 1.0f)
	{
		const Recti attachmentRect(renderContext.mSpeechBalloon.mBasePosition.x, renderContext.mSpeechBalloon.mBasePosition.y, 0, 13);
		DrawingUtils::drawSpeechBalloon(drawer, global::mOxyfontTinySimple, renderContext.mSpeechBalloon.mText, attachmentRect, Recti(15, 10, 370, 204), Color(1.0f, 1.0f, 1.0f, 0.9f));
	}

	// "Applying changes" box
	if (mApplyingChangesFrameCounter > 0)
	{
		Recti rect(100, 94, 200, 35);
		DrawerHelper::drawBorderedRect(drawer, rect, 1, Color(1.0f, 1.0f, 1.0f, 0.95f), Color(0.2f, 0.2f, 0.2f, 0.9f));
		drawer.printText(global::mOxyfontRegular, rect, "Applying changes...", 5);
	}

	if (ConfigurationImpl::instance().mShowControlsDisplay)
	{
		mGameMenuControlsDisplay.render(drawer, mVisibility * 3.0f - 2.0f);
	}

	drawer.performRendering();
}

void ModsMenu::refreshAllDependencies()
{
	for (size_t i = 0; i < mTabs[0].mMenuEntries.size(); ++i)
	{
		GameMenuEntry& gameMenuEntry = *mTabs[0].mMenuEntries.getEntries()[i];
		if (gameMenuEntry.is<ModMenuEntry>())
		{
			refreshDependencies(gameMenuEntry.as<ModMenuEntry>(), i);
		}
	}
}

void ModsMenu::clearDependencies(ModMenuEntry& modMenuEntry)
{
	modMenuEntry.mRemarks.clear();
	modMenuEntry.refreshAfterRemarksChange();
}

void ModsMenu::refreshDependencies(ModMenuEntry& modMenuEntry, size_t modIndex)
{
	modMenuEntry.mRemarks.clear();

	ModManager& modManager = ModManager::instance();
	for (const Mod::OtherModInfo& otherModInfo : modMenuEntry.getMod().mOtherModInfos)
	{
		const Mod* otherMod = modManager.findModByIDHash(otherModInfo.mModIDHash);
		ModEntry* foundOtherMod = nullptr;
		size_t foundIndex = ~0;
		if (nullptr != otherMod)
		{
			// Search other mod entry
			for (size_t i = 0; i < mTabs[0].mMenuEntries.size(); ++i)
			{
				GameMenuEntry& gameMenuEntry = *mTabs[0].mMenuEntries.getEntries()[i];
				if (gameMenuEntry.is<ModMenuEntry>())
				{
					if (&gameMenuEntry.as<ModMenuEntry>().getMod() == otherMod)
					{
						foundIndex = i;
						break;
					}
				}
			}
		}

		const bool otherShouldBeHigherPrio = (otherModInfo.mRelativePriority > 0);
		if (foundIndex != ~0)
		{
			// Check relative priority
			const bool otherIsHigherPrio = (foundIndex < modIndex);
			if (otherIsHigherPrio != otherShouldBeHigherPrio)
			{
				// Show warning
				ModMenuEntry::Remark& remark = vectorAdd(modMenuEntry.mRemarks);
				remark.mIsError = otherModInfo.mIsRequired;
				remark.mText = std::string("This mod needs to be placed ") + (otherShouldBeHigherPrio ? "below" : "above") + " \"" + otherMod->mDisplayName + "\"";
			}
		}
		else
		{
			if (otherModInfo.mIsRequired)
			{
				// Show error
				ModMenuEntry::Remark& remark = vectorAdd(modMenuEntry.mRemarks);
				remark.mIsError = true;
				if (nullptr != otherMod)
				{
					remark.mText = std::string("This mod requires \"") + otherMod->mDisplayName + "\" to be activated as well, and placed " + (otherShouldBeHigherPrio ? "above" : "below") + " this one";
				}
				else
				{
					remark.mText = std::string("This mod requires \"") + otherModInfo.mDisplayName + "\" in order to work (you might need to search for that mod online)";
				}
			}
		}
	}

	modMenuEntry.refreshAfterRemarksChange();
}

void ModsMenu::refreshControlsDisplay()
{
	mGameMenuControlsDisplay.clear();

	const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);
	if (mInMovementMode)
	{
		if (mActiveTab == 0)
		{
			mGameMenuControlsDisplay.addControl("Deactivate", false, "@input_icon_button_right");
			mGameMenuControlsDisplay.addControl("Change priority", false, "@input_icon_button_up", "@input_icon_button_down");
		}
		else
		{
			mGameMenuControlsDisplay.addControl("Activate", false, "@input_icon_button_left");
		}
	}
	else if (keys.X.isPressed())
	{
		mGameMenuControlsDisplay.addControl("Move quickly", false, "@input_icon_button_up", "@input_icon_button_down");
	}
	else if (!mTabs[mActiveTab].mMenuEntries.empty())
	{
		mGameMenuControlsDisplay.addControl("Hold: Move mod", false, "@input_icon_button_A");
		mGameMenuControlsDisplay.addControl("Details", false, "@input_icon_button_Y");
		mGameMenuControlsDisplay.addControl("Hold: Quick Nav", true, "@input_icon_button_X");
	}

	mGameMenuControlsDisplay.addControl("Back", true, "@input_icon_button_B");
}

int ModsMenu::getInfoOverlayHeight() const
{
	return roundToInt(mInfoOverlay.mVisibility * 155.0f);
}

bool ModsMenu::applyModChanges(bool dryRun)
{
	if (!mHasAnyMods)
		return false;

	ModManager& modManager = ModManager::instance();
	const std::vector<GameMenuEntry*>& menuEntries = mTabs[0].mMenuEntries.getEntries();

	// Build new active mods list
	//  -> Here again, reverse order
	std::vector<Mod*> activeMods;
	activeMods.reserve(menuEntries.size());

	for (int index = (int)menuEntries.size()-1; index >= 0; --index)
	{
		const GameMenuEntry& entry = *menuEntries[index];
		if (entry.is<ModMenuEntry>())
		{
			const ModEntry& modEntry = mModEntries[entry.mData];
			if (modEntry.mMakeActive)
			{
				activeMods.push_back(modEntry.mMod);
			}
		}
	}

	// Any change?
	const bool anyChange = (activeMods != modManager.getActiveMods());
	if (anyChange && !dryRun)
	{
		modManager.setActiveMods(activeMods);
		modManager.saveActiveMods();
		Game::instance().resetCurrentMode();		// Only used to signal MenuBackground that it needs to reload the animated background
	}
	return anyChange;
}

void ModsMenu::goBack()
{
	mInfoOverlay.mShouldBeVisible = false;

	if (applyModChanges(true))	// Dry run only to see if there's any change at all
	{
		mApplyingChangesFrameCounter = 4;
		mState = State::APPLYING_CHANGES;
	}
	else
	{
		playMenuSound(0xad);
		mMenuBackground->openMainMenu();
		mState = State::FADE_TO_MENU;
	}
}

GameMenuEntry* ModsMenu::getSelectedGameMenuEntry()
{
	GameMenuEntries& menuEntries = mTabs[mActiveTab].mMenuEntries;
	if (menuEntries.mSelectedEntryIndex >= 0 && menuEntries.mSelectedEntryIndex < (int)menuEntries.size())
	{
		return &menuEntries[menuEntries.mSelectedEntryIndex];
	}
	return nullptr;
}
