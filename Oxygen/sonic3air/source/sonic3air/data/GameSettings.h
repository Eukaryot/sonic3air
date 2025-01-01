/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class GameSettings : public SingleInstance<GameSettings>
{
public:
	std::map<uint32, uint32> mCurrentValues;	// Using "SharedDatabase::Setting::Type" as keys

public:
	inline uint32 getValue(uint32 settingId) const
	{
		const uint32* value = mapFind(mCurrentValues, settingId);
		return (nullptr != value) ? *value : (settingId & 0xff);
	}

	inline void setValue(uint32 key, uint32 value)
	{
		mCurrentValues[key] = value;
	}
};
