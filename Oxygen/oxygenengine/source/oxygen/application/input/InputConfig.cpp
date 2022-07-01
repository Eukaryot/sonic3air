/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/InputConfig.h"


namespace
{
	struct AssignmentLookup
	{
		static inline const std::vector<std::pair<String, uint32>> KEYS_AND_IDENTIFIERS =
		{
			// This list uses the same sorting as the SDLK_* definitions
			{ "Enter",			SDLK_RETURN },
			{ "Esc",			SDLK_ESCAPE },
			{ "Backspace",		SDLK_BACKSPACE },
			{ "Tab",			SDLK_TAB },
			{ "Space",			SDLK_SPACE },
			{ "Exclaim",		SDLK_EXCLAIM },
			{ "QuoteDbl",		SDLK_QUOTEDBL },
			{ "Hash",			SDLK_HASH },
			{ "Percent",		SDLK_PERCENT },
			{ "Dollar",			SDLK_DOLLAR },
			{ "Ampersand",		SDLK_AMPERSAND },
			{ "Quote",			SDLK_QUOTE },
			{ "LeftParen",		SDLK_LEFTPAREN },
			{ "RightParen",		SDLK_RIGHTPAREN },
			{ "Asterisk",		SDLK_ASTERISK },
			{ "Plus",			SDLK_PLUS },
			{ "Comma",			SDLK_COMMA },
			{ "Minus",			SDLK_MINUS },
			{ "Perios",			SDLK_PERIOD },
			{ "Slash",			SDLK_SLASH },
			{ "0",				SDLK_0 },
			{ "1",				SDLK_1 },
			{ "2",				SDLK_2 },
			{ "3",				SDLK_3 },
			{ "4",				SDLK_4 },
			{ "5",				SDLK_5 },
			{ "6",				SDLK_6 },
			{ "7",				SDLK_7 },
			{ "8",				SDLK_8 },
			{ "9",				SDLK_9 },
			{ "Colon",			SDLK_COLON },
			{ "Semicolon",		SDLK_SEMICOLON },
			{ "Less",			SDLK_LESS },
			{ "Equals",			SDLK_EQUALS },
			{ "Greater",		SDLK_GREATER },
			{ "Question",		SDLK_QUESTION },
			{ "At",				SDLK_AT },
			{ "LeftBracket",	SDLK_LEFTBRACKET },
			{ "Backslash",		SDLK_BACKSLASH },
			{ "RightBracket",	SDLK_RIGHTBRACKET },
			{ "Caret",			SDLK_CARET },
			{ "Underscore",		SDLK_UNDERSCORE },
			{ "BackQuote",		SDLK_BACKQUOTE },
			{ "A",				SDLK_a },
			{ "B",				SDLK_b },
			{ "C",				SDLK_c },
			{ "D",				SDLK_d },
			{ "E",				SDLK_e },
			{ "F",				SDLK_f },
			{ "G",				SDLK_g },
			{ "H",				SDLK_h },
			{ "I",				SDLK_i },
			{ "J",				SDLK_j },
			{ "K",				SDLK_k },
			{ "L",				SDLK_l },
			{ "M",				SDLK_m },
			{ "N",				SDLK_n },
			{ "O",				SDLK_o },
			{ "P",				SDLK_p },
			{ "Q",				SDLK_q },
			{ "R",				SDLK_r },
			{ "S",				SDLK_s },
			{ "T",				SDLK_t },
			{ "U",				SDLK_u },
			{ "V",				SDLK_v },
			{ "W",				SDLK_w },
			{ "X",				SDLK_x },
			{ "Y",				SDLK_y },
			{ "Z",				SDLK_z },
			{ "CapsLock",		SDLK_CAPSLOCK },
			// Function keys and some others like PrintScreen intentionally not available
			{ "Insert",			SDLK_INSERT },
			{ "Home",			SDLK_HOME },
			{ "PageUp",			SDLK_PAGEUP },
			{ "Delete",			SDLK_DELETE },
			{ "End",			SDLK_END },
			{ "PageDown",		SDLK_PAGEDOWN },
			{ "Up",				SDLK_UP },
			{ "Down",			SDLK_DOWN },
			{ "Left",			SDLK_LEFT },
			{ "Right",			SDLK_RIGHT },
			{ "NumpadDivide",	SDLK_KP_DIVIDE },
			{ "NumpadMultiply", SDLK_KP_MULTIPLY },
			{ "NumpadMinus",	SDLK_KP_MINUS },
			{ "NumpadPlus",		SDLK_KP_PLUS },
			{ "NumpadEnter",	SDLK_KP_ENTER },
			{ "Numpad1",		SDLK_KP_1 },
			{ "Numpad2",		SDLK_KP_2 },
			{ "Numpad3",		SDLK_KP_3 },
			{ "Numpad4",		SDLK_KP_4 },
			{ "Numpad5",		SDLK_KP_5 },
			{ "Numpad6",		SDLK_KP_6 },
			{ "Numpad7",		SDLK_KP_7 },
			{ "Numpad8",		SDLK_KP_8 },
			{ "Numpad9",		SDLK_KP_9 },
			{ "Numpad0",		SDLK_KP_0 },
			{ "NumpadPeriod",	SDLK_KP_PERIOD },
			// Most of the rest here is left out, as it does not seem important enough
			{ "LeftShift",		SDLK_LSHIFT },
			{ "RightShift",		SDLK_RSHIFT },
			{ "LeftCtrl",		SDLK_LCTRL },
			{ "RightCtrl",		SDLK_RCTRL },
			{ "LeftAlt",		SDLK_LALT },
			{ "RightAlt",		SDLK_RALT },
		};
		static inline std::map<String, uint32> mKeyByIdentifier;	// Uses lowercase strings for better comparability
		static inline std::map<uint32, String> mIdentifierByKey;

		static inline uint32 getKeyByIdentifier(const String& identifier)
		{
			if (mKeyByIdentifier.empty())
			{
				// Lazy initialization
				for (const auto& pair : KEYS_AND_IDENTIFIERS)
				{
					String key = pair.first;
					key.lowerCase();
					mKeyByIdentifier[key] = pair.second;
				}
			}

			// Search in the lookup map
			{
				static String key;		// Static to avoid reallocations
				key = identifier;
				key.lowerCase();
				const auto it = mKeyByIdentifier.find(key);
				return (it == mKeyByIdentifier.end()) ? 0 : it->second;
			}
		}

		static inline const String& getIdentifierByKey(uint32 key)
		{
			if (mIdentifierByKey.empty())
			{
				// Lazy initialization
				for (const auto& pair : KEYS_AND_IDENTIFIERS)
				{
					mIdentifierByKey[pair.second] = pair.first;
				}
			}

			// Search in the lookup map
			{
				static String EMPTY_STRING;
				const auto it = mIdentifierByKey.find(key);
				return (it == mIdentifierByKey.end()) ? EMPTY_STRING : it->second;
			}
		}
	};
}



void InputConfig::Assignment::getMappingString(String& outString, DeviceType deviceType) const
{
	switch (deviceType)
	{
		case DeviceType::KEYBOARD:
		{
			outString = AssignmentLookup::getIdentifierByKey(mIndex);
			if (outString.empty())
			{
				outString << "Key" << mIndex;
			}
			break;
		}

		case DeviceType::GAMEPAD:
		{
			switch (mType)
			{
				case Type::AXIS:
				{
					outString.clear() << "Axis" << mIndex;
					break;
				}
				case Type::BUTTON:
				{
					outString.clear() << "Button" << mIndex;
					break;
				}
				case Type::POV:
				{
					outString.clear() << "Pov" << rmx::log2(mIndex & 0xff);
					break;
				}
			}
			break;
		}
	}
}

bool InputConfig::Assignment::setFromMappingString(Assignment& output, const String& mappingString, DeviceType deviceType)
{
	switch (deviceType)
	{
		case DeviceType::KEYBOARD:
		{
			const uint32 key = AssignmentLookup::getKeyByIdentifier(mappingString);
			if (key != 0)
			{
				output = Assignment(Type::BUTTON, key);
				return true;
			}
			else if (mappingString.startsWith("Key"))
			{
				String str = mappingString.getSubString(3, mappingString.length() - 3);
				output = Assignment(Type::BUTTON, str.parseInt());
				return true;
			}
			break;
		}

		case DeviceType::GAMEPAD:
		{
			if (mappingString.startsWith("Axis"))
			{
				String str = mappingString.getSubString(4, mappingString.length() - 4);
				output = Assignment(Type::AXIS, str.parseInt());
				return true;
			}
			else if (mappingString.startsWith("Button"))
			{
				String str = mappingString.getSubString(6, mappingString.length() - 6);
				output = Assignment(Type::BUTTON, str.parseInt());
				return true;
			}
			else if (mappingString.startsWith("Pov"))
			{
				String str = mappingString.getSubString(3, mappingString.length() - 3);
				output = Assignment(Type::POV, (1 << str.parseInt()));
				return true;
			}
			break;
		}
	}
	return false;
}



void InputConfig::setupDefaultDeviceDefinitions(std::vector<DeviceDefinition>& outDeviceDefinitions)
{
	outDeviceDefinitions.clear();
	{
		DeviceDefinition& deviceDefinition = vectorAdd(outDeviceDefinitions);
		deviceDefinition.mDeviceType = DeviceType::KEYBOARD;
		deviceDefinition.mIdentifier = "Keyboard1";
		setupDefaultKeyboardMappings(deviceDefinition, 0);
	}
	{
		DeviceDefinition& deviceDefinition = vectorAdd(outDeviceDefinitions);
		deviceDefinition.mDeviceType = DeviceType::KEYBOARD;
		deviceDefinition.mIdentifier = "Keyboard2";
		setupDefaultKeyboardMappings(deviceDefinition, 1);
	}
}

void InputConfig::setupDefaultKeyboardMappings(DeviceDefinition& outDeviceDefinition, int keyboardIndex)
{
	ControlMapping* mappings = outDeviceDefinition.mMappings;
	for (size_t k = 0; k < (size_t)DeviceDefinition::Button::_NUM; ++k)
	{
		mappings[k].mAssignments.clear();
	}

	if (keyboardIndex == 0)
	{
		// Setup fixed and modifyable assignments for keyboard 1
		const std::vector<std::pair<DeviceDefinition::Button, Assignment>> FIXED_ASSIGNMENTS =
		{
			{ DeviceDefinition::Button::UP,		{ Assignment::Type::BUTTON, SDLK_UP     } },
			{ DeviceDefinition::Button::DOWN,	{ Assignment::Type::BUTTON, SDLK_DOWN   } },
			{ DeviceDefinition::Button::LEFT,	{ Assignment::Type::BUTTON, SDLK_LEFT   } },
			{ DeviceDefinition::Button::RIGHT,	{ Assignment::Type::BUTTON, SDLK_RIGHT  } },
			{ DeviceDefinition::Button::START,	{ Assignment::Type::BUTTON, SDLK_RETURN } },
			{ DeviceDefinition::Button::BACK,	{ Assignment::Type::BUTTON, SDLK_ESCAPE } },
		};
		const std::vector<std::pair<DeviceDefinition::Button, Assignment>> MODIFYABLE_ASSIGNMENTS =
		{
			{ DeviceDefinition::Button::A,		{ Assignment::Type::BUTTON, SDLK_a } },
			{ DeviceDefinition::Button::B,		{ Assignment::Type::BUTTON, SDLK_s } },
			{ DeviceDefinition::Button::X,		{ Assignment::Type::BUTTON, SDLK_d } },
			{ DeviceDefinition::Button::X,		{ Assignment::Type::BUTTON, SDLK_q } },
			{ DeviceDefinition::Button::Y,		{ Assignment::Type::BUTTON, SDLK_w } },
			{ DeviceDefinition::Button::BACK,	{ Assignment::Type::BUTTON, SDLK_BACKSPACE } },
		};

		for (const auto& pair : FIXED_ASSIGNMENTS)
		{
			mappings[(size_t)pair.first].mAssignments.push_back(pair.second);
		}
		for (size_t k = 0; k < (size_t)DeviceDefinition::Button::_NUM; ++k)
		{
			mappings[k].mNumFixedAssignments = mappings[k].mAssignments.size();
		}
		for (const auto& pair : MODIFYABLE_ASSIGNMENTS)
		{
			mappings[(size_t)pair.first].mAssignments.push_back(pair.second);
		}
	}
	else
	{
		// Leave keyboard 2 empty
	}
}

void InputConfig::clearAssignments(DeviceDefinition& deviceDefinition, size_t buttonIndex)
{
	// Remove all assignments, except for the fixed ones at the start
	RMX_ASSERT(buttonIndex < (size_t)DeviceDefinition::Button::_NUM, "Invalid button index " << buttonIndex);
	ControlMapping& mapping = deviceDefinition.mMappings[buttonIndex];
	mapping.mAssignments.resize(mapping.mNumFixedAssignments);
}

void InputConfig::addAssignment(DeviceDefinition& deviceDefinition, size_t buttonIndex, const Assignment& newAssignment, bool removeDuplicates)
{
	RMX_ASSERT(buttonIndex < (size_t)DeviceDefinition::Button::_NUM, "Invalid button index " << buttonIndex);

	// Check for duplicates in same button
	{
		std::vector<Assignment>& buttonMappings = deviceDefinition.mMappings[buttonIndex].mAssignments;
		if (std::count(buttonMappings.begin(), buttonMappings.end(), newAssignment) != 0)
		{
			// Already added
			return;
		}
	}

	// Check for duplicates in other buttons
	bool canBeAdded = true;
	if (removeDuplicates)
	{
		for (size_t k = 0; k < (size_t)DeviceDefinition::Button::_NUM; ++k)
		{
			if (k != buttonIndex)
			{
				std::vector<Assignment>& buttonMappings = deviceDefinition.mMappings[k].mAssignments;
				for (size_t j = 0; j < buttonMappings.size(); ++j)
				{
					if (buttonMappings[j] == newAssignment)
					{
						// Remove this duplicate if possible, or don't if it's a fixed assignment
						if (j < deviceDefinition.mMappings[k].mNumFixedAssignments)
						{
							// Existing assignment can't be replaced
							canBeAdded = false;
						}
						else
						{
							// Remove existing assignment
							buttonMappings.erase(buttonMappings.begin() + j);
						}
						break;
					}
				}
			}
		}
	}

	if (canBeAdded)
	{
		// Add assignment
		deviceDefinition.mMappings[buttonIndex].mAssignments.push_back(newAssignment);
	}
}

void InputConfig::setAssignments(DeviceDefinition& deviceDefinition, size_t buttonIndex, const std::vector<Assignment>& assignments, bool removeDuplicates)
{
	clearAssignments(deviceDefinition, buttonIndex);
	for (const Assignment& newAssignment : assignments)
	{
		addAssignment(deviceDefinition, buttonIndex, newAssignment, removeDuplicates);
	}
}
