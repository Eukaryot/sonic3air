/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/options/OptionsEntry.h"
#include "sonic3air/Game.h"
#include "oxygen/application/modding/Mod.h"


void OptionEntry::loadValue()
{
	if (nullptr == mGameMenuEntry)
		return;

	switch (mType)
	{
		case OptionEntry::Type::SETTING:
		{
			const uint32 value = Game::instance().getSetting(mSetting, true);
			mGameMenuEntry->setSelectedIndexByValue(value);
			break;
		}

		case OptionEntry::Type::SETTING_BITMASK:
		{
			const uint32 value = Game::instance().getSetting(mSetting, true);
			for (size_t index = 0; index < mGameMenuEntry->mOptions.size(); ++index)
			{
				GameMenuEntry::Option& option = mGameMenuEntry->mOptions[index];
				const bool setBits = (option.mValue & 0x80000000) != 0;
				const uint32 bitmask = (option.mValue & 0x7fffffff);
				if (setBits)
				{
					if ((value & bitmask) == bitmask)
					{
						mGameMenuEntry->mSelectedIndex = index;
						break;
					}
				}
				else
				{
					if ((value & bitmask) == 0)
					{
						mGameMenuEntry->mSelectedIndex = index;
						break;
					}
				}

				// Fallback
				if ((value & bitmask) != 0)
				{
					mGameMenuEntry->mSelectedIndex = index;
				}
			}
			break;
		}

		case OptionEntry::Type::CONFIG_INT:
		{
			int* ptr = reinterpret_cast<int*>(mValuePointer);
			const uint32 value = (uint32)*ptr;
			mGameMenuEntry->setSelectedIndexByValue(value);
			break;
		}

		case OptionEntry::Type::CONFIG_ENUM_8:
		{
			uint8* ptr = reinterpret_cast<uint8*>(mValuePointer);
			const uint32 value = (uint32)*ptr;
			mGameMenuEntry->setSelectedIndexByValue(value);
			break;
		}

		case OptionEntry::Type::CONFIG_PERCENT:
		{
			float* ptr = reinterpret_cast<float*>(mValuePointer);
			const float floatPercent = (*ptr) * 100.0f;
			const uint32 value = (uint32)clamp(roundToInt(floatPercent), 0, 100);
			mGameMenuEntry->setSelectedIndexByValue(value);
			break;
		}

		case OptionEntry::Type::MOD_SETTING:
		{
			Mod::Setting* ptr = reinterpret_cast<Mod::Setting*>(mValuePointer);
			mGameMenuEntry->setSelectedIndexByValue(ptr->mCurrentValue);
			break;
		}

		default:
			break;
	}
}

void OptionEntry::applyValue()
{
	if (nullptr == mGameMenuEntry)
		return;
	const uint32 value = mGameMenuEntry->selected().mValue;

	switch (mType)
	{
		case OptionEntry::Type::SETTING:
		{
			Game::instance().setSetting(mSetting, value);
			break;
		}

		case OptionEntry::Type::SETTING_BITMASK:
		{
			uint32 finalValue = Game::instance().getSetting(mSetting, true);
			const bool setBits = (value & 0x80000000) != 0;
			const uint32 bitmask = (value & 0x7fffffff);
			if (setBits)
			{
				// Set bits
				finalValue |= bitmask;
			}
			else
			{
				// Clear bits
				finalValue &= ~bitmask;
			}
			Game::instance().setSetting(mSetting, finalValue);
			break;
		}

		case OptionEntry::Type::CONFIG_INT:
		{
			int* ptr = reinterpret_cast<int*>(mValuePointer);
			*ptr = (int)value;
			break;
		}

		case OptionEntry::Type::CONFIG_ENUM_8:
		{
			uint8* ptr = reinterpret_cast<uint8*>(mValuePointer);
			*ptr = (uint8)value;
			break;
		}

		case OptionEntry::Type::CONFIG_PERCENT:
		{
			float* ptr = reinterpret_cast<float*>(mValuePointer);
			*ptr = (float)value / 100.0f;
			break;
		}

		case OptionEntry::Type::MOD_SETTING:
		{
			Mod::Setting* ptr = reinterpret_cast<Mod::Setting*>(mValuePointer);
			ptr->mCurrentValue = value;
			break;
		}

		default:
			break;
	}
}
