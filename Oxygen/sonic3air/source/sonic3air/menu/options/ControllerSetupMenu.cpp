/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/options/ControllerSetupMenu.h"
#include "sonic3air/menu/options/OptionsMenu.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/resources/DynamicSprites.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen/application/Application.h"
#include "oxygen/helper/DrawerHelper.h"
#include "oxygen/helper/Utils.h"


namespace
{
	enum Entry
	{
		CONTROLLER_SELECT,
		ASSIGN_ALL,
		VISUAL_STYLE,
		BUTTON_UP		= 0x10,
		BUTTON_DOWN		= 0x11,
		BUTTON_LEFT		= 0x12,
		BUTTON_RIGHT	= 0x13,
		BUTTON_A		= 0x14,
		BUTTON_B		= 0x15,
		BUTTON_X		= 0x16,
		BUTTON_Y		= 0x17,
		BUTTON_START	= 0x18,
		BUTTON_BACK		= 0x19,
		BUTTON_L		= 0x1a,
		BUTTON_R		= 0x1b,
		_BACK			= 0xff
	};

	enum class AssignmentType
	{
		ASSIGN = 0,
		APPEND = 1,
		REMOVE = 2
	};
}


ControllerSetupMenu::ControllerSetupMenu(OptionsMenu& optionsMenu) :
	mOptionsMenu(optionsMenu)
{
	mAssignmentType.addEntry("")
					.addOption("Assign", (uint32)AssignmentType::ASSIGN)
					.addOption("Append", (uint32)AssignmentType::APPEND)
					.addOption("Remove", (uint32)AssignmentType::REMOVE);
}

ControllerSetupMenu::~ControllerSetupMenu()
{
}

void ControllerSetupMenu::fadeIn()
{
	mState = State::APPEAR;
	refreshGamepadList(true);

	mUsingControlsLR = InputManager::instance().isUsingControlsLR();
	mMenuEntries.getEntryByData(::BUTTON_L)->setVisible(mUsingControlsLR);
	mMenuEntries.getEntryByData(::BUTTON_R)->setVisible(mUsingControlsLR);

	refreshControlsDisplay();
}

void ControllerSetupMenu::initialize()
{
	mMenuEntries.clear();
	mMenuEntries.reserve(20);
	{
		GameMenuEntry& entry = mMenuEntries.addEntry("", ::CONTROLLER_SELECT);
		mControllerSelectEntry = &entry;
		refreshGamepadList(true);
	}

	mMenuEntries.addEntry("Assign all buttons", ::ASSIGN_ALL);

	for (size_t i = 0; i < InputConfig::DeviceDefinition::NUM_BUTTONS; ++i)
	{
		mMenuEntries.addEntry(InputConfig::DeviceDefinition::BUTTON_NAME[i], ::BUTTON_UP + (uint32)i);
	}

	{
		GameMenuEntry& entry = mMenuEntries.addEntry("Visual Style", ::VISUAL_STYLE);
		entry.addOption("XBox", 0);
		entry.addOption("PS", 1);
		entry.addOption("Switch", 2);
		entry.mMarginAbove += 5;
		entry.setSelectedIndexByValue(ConfigurationImpl::instance().mGamepadVisualStyle);

		const bool isGamepad = (nullptr != getSelectedDevice() && getSelectedDevice()->mType == InputConfig::DeviceType::GAMEPAD);
		entry.setVisible(isGamepad);
	}

	// Back button
	mMenuEntries.addEntry("Back to Options", ::_BACK);
}

void ControllerSetupMenu::deinitialize()
{
}

void ControllerSetupMenu::keyboard(const rmx::KeyboardEvent& ev)
{
	if (ev.state && mCurrentlyAssigningButtonIndex != -1 && !mControlsBlocked)
	{
		const InputManager::RealDevice* device = getSelectedDevice();
		if (nullptr == device)
		{
			// Abort
			mCurrentlyAssigningButtonIndex = -1;
			mControlsBlocked = true;
			refreshControlsDisplay();
			return;
		}

		if (ev.key == SDLK_ESCAPE)
		{
			abortButtonBinding(*device);
			return;
		}

		if (device->mType == InputConfig::DeviceType::KEYBOARD)
		{
			assignButtonBinding(*device, InputConfig::Assignment(InputConfig::Assignment::Type::BUTTON, ev.key));
		}
	}
}

void ControllerSetupMenu::update(float timeElapsed)
{
	if (isVisible())
	{
		const InputManager::ControllerScheme& keys = InputManager::instance().getController(0);
		const bool anyTouch = !InputManager::instance().getActiveTouches().empty();

		if (mControlsBlocked)
		{
			// Anything pressed?
			if (anyTouch || keys.Start.justPressed() || keys.Back.justPressed() ||
				keys.A.justPressed() || keys.B.justPressed() || keys.X.justPressed() || keys.Y.justPressed() ||
				keys.Up.justPressed() || keys.Down.justPressed() || keys.Left.justPressed() || keys.Right.justPressed())
			{
				return;
			}
			mControlsBlocked = false;
		}

		if (mCurrentlyAssigningButtonIndex != -1)
		{
			// Check gamepad
			const InputManager::RealDevice* device = getSelectedDevice();
			if (nullptr == device)
			{
				// Abort
				mCurrentlyAssigningButtonIndex = -1;
				mControlsBlocked = true;
				refreshControlsDisplay();
			}
			else if (device->mType == InputConfig::DeviceType::GAMEPAD)
			{
				std::vector<InputConfig::Assignment> pressedInputs;
				InputManager::instance().getPressedGamepadInputs(pressedInputs, *device);

				// Assign first pressed input that is not blocked
				for (const InputConfig::Assignment& input : pressedInputs)
				{
					const bool isBlocked = (std::count(mBlockedInputs.begin(), mBlockedInputs.end(), input) != 0);
					if (!isBlocked)
					{
						assignButtonBinding(*device, input);
						return;
					}
				}

				// Unblock input that aren't pressed any more
				for (size_t i = 0; i < mBlockedInputs.size(); ++i)
				{
					const bool stillPressed = (std::count(pressedInputs.begin(), pressedInputs.end(), mBlockedInputs[i]) != 0);
					if (!stillPressed)
					{
						mBlockedInputs.erase(mBlockedInputs.begin() + i);
						--i;
					}
				}

				// Check for touches, those are meant to abort, replacing Esc if there's no keyboard
				if (anyTouch)
				{
					abortButtonBinding(*device);
				}
			}
		}
		else
		{
			GameMenuEntries::UpdateResult result = mMenuEntries.update();
			if (result == GameMenuEntries::UpdateResult::NONE)
			{
				if ((mMenuEntries.selected().mData & 0xf0) == 0x10)
				{
					result = mAssignmentType.update();
				}
			}
			else if (result == GameMenuEntries::UpdateResult::OPTION_CHANGED)
			{
				switch (mMenuEntries.selected().mData)
				{
					case ::CONTROLLER_SELECT:
					{
						const bool isGamepad = (nullptr != getSelectedDevice() && getSelectedDevice()->mType == InputConfig::DeviceType::GAMEPAD);
						mMenuEntries.getEntryByData(::VISUAL_STYLE)->setVisible(isGamepad);
						break;
					}

					case ::VISUAL_STYLE:
					{
						ConfigurationImpl::instance().mGamepadVisualStyle = mMenuEntries.selected().selected().mValue;
						break;
					}
				}
			}

			if (result == GameMenuEntries::UpdateResult::ENTRY_CHANGED)
			{
				refreshControlsDisplay();
			}

			if (result != GameMenuEntries::UpdateResult::NONE)
			{
				GameMenuBase::playMenuSound(0x5b);
			}

			enum class ButtonEffect
			{
				NONE,
				ACCEPT,
				BACK
			};
			const ButtonEffect buttonEffect = (keys.Start.justPressed() || keys.A.justPressed() || keys.X.justPressed()) ? ButtonEffect::ACCEPT :
											  (keys.Back.justPressed() || keys.B.justPressed()) ? ButtonEffect::BACK : ButtonEffect::NONE;

			if (buttonEffect != ButtonEffect::NONE)
			{
				if (buttonEffect == ButtonEffect::ACCEPT)
				{
					const GameMenuEntry& selectedEntry = mMenuEntries.selected();
					switch (selectedEntry.mData)
					{
						case ::ASSIGN_ALL:
						{
							if (nullptr != getSelectedDevice())
							{
								mCurrentlyAssigningButtonIndex = 0;
								mAppendAssignment = false;
								mAssignAll = true;
								mControlsBlocked = true;
								InputManager::instance().getPressedGamepadInputs(mBlockedInputs, *getSelectedDevice());
								++mMenuEntries.mSelectedEntryIndex;
								GameMenuBase::playMenuSound(0x63);
								refreshControlsDisplay();
							}
							break;
						}

						case ::_BACK:
						{
							goBack();
							break;
						}

						default:
						{
							if ((selectedEntry.mData & 0xf0) == 0x10 && nullptr != getSelectedDevice())
							{
								mCurrentlyAssigningButtonIndex = (selectedEntry.mData & 0x0f);
								const AssignmentType assignmentType = (AssignmentType)mAssignmentType.selected().mSelectedIndex;
								mAppendAssignment = (assignmentType == AssignmentType::APPEND);
								mAssignAll = false;
								if (assignmentType == AssignmentType::REMOVE)
								{
									// Remove
									const InputManager::RealDevice* device = getSelectedDevice();
									if (nullptr != device)
									{
										mTemp.clear();
										const InputConfig::DeviceDefinition::Button button = (InputConfig::DeviceDefinition::Button)mCurrentlyAssigningButtonIndex;
										const auto* mapping = InputManager::instance().getControlMapping(*device, button);
										if (nullptr != mapping && mapping->size() >= 2)
										{
											mTemp = *mapping;
											mTemp.pop_back();
										}
										assignButtonBindings(*device, mTemp);
									}
								}
								else
								{
									// Assign or append
									mControlsBlocked = true;
									InputManager::instance().getPressedGamepadInputs(mBlockedInputs, *getSelectedDevice());
									GameMenuBase::playMenuSound(0x63);
								}
								refreshControlsDisplay();
							}
							break;
						}
					}
				}
				else if (buttonEffect == ButtonEffect::BACK)
				{
					goBack();
					return;
				}
			}
		}
	}

	// Scrolling
	mScrolling.update(timeElapsed);

	// Fading in/out
	if (mState == State::APPEAR)
	{
		mVisibility += timeElapsed * 6.0f;
		if (mVisibility >= 1.0f)
		{
			mVisibility = 1.0f;
			mState = State::SHOW;
		}
	}
	else if (mState == State::FADE_OUT)
	{
		mVisibility -= timeElapsed * 6.0f;
		if (mVisibility <= 0.0f)
		{
			mVisibility = 0.0f;
			mOptionsMenu.removeControllerSetupMenu();
		}
	}

	// Check for changes in connected gamepads
	refreshGamepadList();
}

void ControllerSetupMenu::render()
{
	GuiBase::render();

	Drawer& drawer = EngineMain::instance().getDrawer();
	const int anchorY = roundToInt((1.0f - mVisibility) * 120.0f);
	const int startY = anchorY + 24 - mScrolling.getScrollOffsetYInt();
	const float alpha = mVisibility;
	const bool showEmptyMenu = mControllerSelectEntry->mOptions.empty();

	const InputManager::RealDevice* device = getSelectedDevice();
	const bool isGamepad = (nullptr != device && device->mType == InputConfig::DeviceType::GAMEPAD);
	const int gamepadButtonStyle = clamp(ConfigurationImpl::instance().mGamepadVisualStyle, 0, 2);

	const int baseX = 200;
	int py = startY;
	if (showEmptyMenu)
	{
		mMenuEntries.setSelectedIndexByValue(::_BACK);
		py += 80;
		drawer.printText(global::mSonicFontB, Recti(baseX, py, 0, 10), "No game controller found", 2, Color(0.6f, 0.8f, 1.0f, alpha));
		py += 15;
	}
	else
	{
		if (Application::instance().hasKeyboard())
		{
			drawer.printText(global::mSonicFontB, Recti(baseX, py, 0, 10), "Select keyboard or game controller", 2, Color(0.6f, 0.8f, 1.0f, alpha));
		}
		else
		{
			drawer.printText(global::mSonicFontB, Recti(baseX, py, 0, 10), "Select game controller", 2, Color(0.6f, 0.8f, 1.0f, alpha));
		}
		py += 11;
	}

	// Menu
	for (size_t line = 0; line < mMenuEntries.size(); ++line)
	{
		const auto& entry = mMenuEntries[line];
		if (!entry.isVisible())
			continue;
		if (showEmptyMenu && entry.mData != ::_BACK)
			continue;

		const std::string& text = entry.mOptions.empty() ? entry.mText : entry.mOptions[entry.mSelectedIndex].mText;
		const bool isSelected = ((int)line == mMenuEntries.mSelectedEntryIndex);
		if (isSelected)
		{
			mScrolling.setCurrentSelection(py - startY - 30, py + 70 - startY);
		}

		Color color = isSelected ? Color::YELLOW : Color::WHITE;
		color.a *= alpha;

		if ((entry.mData & 0xf0) == 0x10)
		{
			// Button entry
			Font& font = global::mOxyfontSmall;
			int px = isGamepad ? (baseX - 12) : baseX;

			drawer.printText(font, Recti(px - 16, py, 0, 10), entry.mText, 6, color);

			if (nullptr != device)
			{
				const size_t buttonIndex = entry.mData - 0x10;
				RMX_ASSERT(buttonIndex < device->mControlMappings.size(), "Invalid button index " << buttonIndex);
				if (buttonIndex < device->mControlMappings.size())	// It's unclear how, but an invalid index is possible and led to crashes for some players
				{
					const InputConfig::ControlMapping& mapping = device->mControlMappings[buttonIndex];
					const bool assigningThis = (mCurrentlyAssigningButtonIndex == (entry.mData & 0x0f));

					size_t numAssignmentsToShow = mapping.mAssignments.size();
					if (assigningThis && !mAppendAssignment)
					{
						numAssignmentsToShow = mapping.mNumFixedAssignments;
					}

					std::string assignmentsString1;
					std::string assignmentsString2;
					for (size_t k = 0; k < numAssignmentsToShow; ++k)
					{
						const InputConfig::Assignment& assignment = mapping.mAssignments[k];
						String str;
						assignment.getMappingString(str, device->mType);
						if (k < mapping.mNumFixedAssignments)
						{
							if (!assignmentsString1.empty())
								assignmentsString1 += ", ";
							assignmentsString1 += *str;
						}
						else
						{
							if (!assignmentsString2.empty())
								assignmentsString2 += ", ";
							else if (!assignmentsString1.empty())
								assignmentsString1 += ", ";
							assignmentsString2 += *str;
						}
					}
					if (assigningThis)
					{
						if (!assignmentsString2.empty())
							assignmentsString2 += ", ";
						else if (!assignmentsString1.empty())
							assignmentsString1 += ", ";
						assignmentsString2 += "?";
					}

					if (assignmentsString1.empty() && assignmentsString2.empty())
					{
						const Color grayColor = isSelected ? color : Color(color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, color.a);
						drawer.printText(font, Recti(baseX + 16, py, 0, 10), "none", 4, grayColor);
					}
					else
					{
						const Color cyanColor = isSelected ? color : Color(color.r * 0.7f, color.g * 0.9f, color.b * 0.9f, color.a);
						drawer.printText(font, Recti(baseX + 16, py, 0, 10), assignmentsString1, 4, cyanColor);
						drawer.printText(font, Recti(baseX + 16 + font.getWidth(assignmentsString1), py, 0, 10), assignmentsString2, 4, color);
					}

					if (isGamepad && buttonIndex < 12)
					{
						const uint64 spriteKey = DynamicSprites::getGamepadSpriteKey(buttonIndex, gamepadButtonStyle);
						drawer.drawSprite(Vec2i(px - 3, py + 4), spriteKey, Color(1.0f, 1.0f, 1.0f, alpha));
					}
				}
			}

			if (isSelected)
			{
				// Assignment type selection
				const auto& entry = mAssignmentType.selected();

				const int center = px - 100;
				const std::string& text = entry.selected().mText;
				drawer.printText(font, Recti(center, py, 0, 10), text, 5, color);

				const bool canGoLeft  = (entry.mSelectedIndex > 0);
				const bool canGoRight = (entry.mSelectedIndex < entry.mOptions.size() - 1);
				const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
				const int arrowDistance = 26 + ((offset > 3) ? (6 - offset) : offset);

				if (canGoLeft)
					drawer.printText(font, Recti(center - arrowDistance, py, 0, 10), "<", 5, color);
				if (canGoRight)
					drawer.printText(font, Recti(center + arrowDistance, py, 0, 10), ">", 5, color);
			}

			py += isGamepad ? 17 : 13;
		}
		else if (entry.mOptions.empty())
		{
			// Entry without options
			py += (entry.mData == ::_BACK) ? 10 : 4;
			drawer.printText(global::mOxyfontRegular, Recti(0, py, 400, 10), text, 5, color);

			if (isSelected)
			{
				// Draw arrows
				const int halfTextWidth = global::mOxyfontRegular.getWidth(entry.mText) / 2;
				const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
				const int arrowDistance = 16 + ((offset > 3) ? (6 - offset) : offset);
				drawer.printText(global::mOxyfontRegular, Recti(baseX - halfTextWidth - arrowDistance, py, 0, 10), ">>", 5, color);
				drawer.printText(global::mOxyfontRegular, Recti(baseX + halfTextWidth + arrowDistance, py, 0, 10), "<<", 5, color);
			}

			py += (entry.mData == ::ASSIGN_ALL) ? 20 : 16;
		}
		else
		{
			// It's an actual options entry, with multiple options to choose from
			py += 4 + entry.mMarginAbove;
			Font& font = global::mOxyfontRegular;

			const bool canGoLeft  = (entry.mSelectedIndex > 0);
			const bool canGoRight = (entry.mSelectedIndex < entry.mOptions.size() - 1);

			const int center = baseX + (entry.mText.empty() ? 0 : 88);
			int arrowDistance = 100;
			if (isSelected)
			{
				const int offset = (int)std::fmod(FTX::getTime() * 6.0f, 6.0f);
				arrowDistance += ((offset > 3) ? (6 - offset) : offset);
			}

			if (!entry.mText.empty())
			{
				// Description
				drawer.printText(font, Recti(baseX - 40, py, 0, 10), entry.mText, 6, color);
				arrowDistance -= 50;
			}

			// Value text
			{
				const std::string& text = entry.mOptions[entry.mSelectedIndex].mText;
				drawer.printText(font, Recti(center - 80, py, 160, 10), text, 5, color);
			}

			if (canGoLeft)
				drawer.printText(font, Recti(center - arrowDistance, py, 0, 10), "<", 5, color);
			if (canGoRight)
				drawer.printText(font, Recti(center + arrowDistance, py, 0, 10), ">", 5, color);

			py += (line == 0) ? 20 : 16;
		}
	}

	if (ConfigurationImpl::instance().mShowControlsDisplay)
	{
		mGameMenuControlsDisplay.render(drawer, alpha);
	}

	drawer.performRendering();
}

const InputManager::RealDevice* ControllerSetupMenu::getSelectedDevice() const
{
	if (!mControllerSelectEntry->hasSelected())
		return nullptr;

	const uint32 deviceId = mControllerSelectEntry->selected().mValue;
	if (deviceId >= 0xfff0 && deviceId < 0xfff0 + InputManager::NUM_PLAYERS)
	{
		return &InputManager::instance().getKeyboards()[deviceId - 0xfff0];
	}
	else
	{
		return InputManager::instance().getGamepadByJoystickInstanceId(deviceId);
	}
}

void ControllerSetupMenu::abortButtonBinding(const InputManager::RealDevice& device)
{
	if (mAppendAssignment)
	{
		// No change
		onAssignmentDone(device);
	}
	else
	{
		// Assign nothing
		mTemp.clear();
		assignButtonBindings(device, mTemp);
	}
}

void ControllerSetupMenu::assignButtonBinding(const InputManager::RealDevice& device, const InputConfig::Assignment& newAssignment)
{
	mTemp.clear();
	if (mAppendAssignment)
	{
		const InputConfig::DeviceDefinition::Button button = (InputConfig::DeviceDefinition::Button)mCurrentlyAssigningButtonIndex;
		const auto* mapping = InputManager::instance().getControlMapping(device, button);
		if (nullptr != mapping)
			mTemp = *mapping;
	}
	mTemp.emplace_back(newAssignment);
	assignButtonBindings(device, mTemp);
}

void ControllerSetupMenu::assignButtonBindings(const InputManager::RealDevice& device, const std::vector<InputConfig::Assignment>& newAssignments)
{
	const InputConfig::DeviceDefinition::Button button = (InputConfig::DeviceDefinition::Button)mCurrentlyAssigningButtonIndex;
	InputManager::instance().redefineControlMapping(device, button, newAssignments);

	GameMenuBase::playMenuSound(0x5b);
	onAssignmentDone(device);
}

void ControllerSetupMenu::onAssignmentDone(const InputManager::RealDevice& device)
{
	if (mAssignAll && mCurrentlyAssigningButtonIndex < 9)
	{
		++mCurrentlyAssigningButtonIndex;
		++mMenuEntries.mSelectedEntryIndex;
		InputManager::instance().getPressedGamepadInputs(mBlockedInputs, device);
	}
	else
	{
		mCurrentlyAssigningButtonIndex = -1;
		if (mAssignAll)
			++mMenuEntries.mSelectedEntryIndex;
		refreshControlsDisplay();
	}
	mControlsBlocked = true;
}

void ControllerSetupMenu::goBack()
{
	GameMenuBase::playMenuSound(0xad);
	mState = State::FADE_OUT;
}

void ControllerSetupMenu::refreshGamepadList(bool forceUpdate)
{
	const uint32 changeCounter = InputManager::instance().getGamepadsChangeCounter();
	if (mLastGamepadsChangeCounter != changeCounter || forceUpdate)
	{
		mLastGamepadsChangeCounter = changeCounter;

		GameMenuEntry& entry = *mControllerSelectEntry;
		const int oldValue = entry.hasSelected() ? entry.selected().mValue : 0;		// Default value 0 = select first controller if something goes wrong
		entry.mOptions.clear();
		if (Application::instance().hasKeyboard())
		{
			for (int k = 0; k < Configuration::instance().mNumPlayers; ++k)
			{
				entry.addOption("Keyboard Player " + std::to_string(k + 1), 0xfff0 + k);
			}
		}

		for (const InputManager::RealDevice& gamepad : InputManager::instance().getGamepads())
		{
			std::string text = gamepad.getName();
			utils::shortenTextToFit(text, global::mOxyfontRegular, 180);
			entry.addOption(text, gamepad.mSDLJoystickInstanceId);
		}
		entry.setSelectedIndexByValue(oldValue);
	}
}

void ControllerSetupMenu::refreshControlsDisplay()
{
	mGameMenuControlsDisplay.clear();

	if (mCurrentlyAssigningButtonIndex >= 0)
	{
		mGameMenuControlsDisplay.addControl("Clear", false, "input_icon_key_esc");
	}
	else
	{
		const GameMenuEntry& selectedEntry = mMenuEntries.selected();
		switch (selectedEntry.mData)
		{
			case ::CONTROLLER_SELECT:
				mGameMenuControlsDisplay.addControl("Select Keyboard or Gamepad", false, "@input_icon_button_left", "@input_icon_button_right");
				break;

			case ::ASSIGN_ALL:
				mGameMenuControlsDisplay.addControl("Assign all", false, "@input_icon_button_A");
				break;

			case ::VISUAL_STYLE:
				mGameMenuControlsDisplay.addControl("Change visual style", false, "@input_icon_button_left", "@input_icon_button_right");
				break;

			default:
				if ((selectedEntry.mData & 0xf0) == 0x10)
				{
					mGameMenuControlsDisplay.addControl("Select", false, "@input_icon_button_A");
					mGameMenuControlsDisplay.addControl("Assignment mode", false, "@input_icon_button_left", "@input_icon_button_right");
				}
				break;
		}
	}

	mGameMenuControlsDisplay.addControl("Back",	true, "@input_icon_button_B");
}
