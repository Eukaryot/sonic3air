/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class GameSettings : public SingleInstance<GameSettings>
{
public:
	inline uint32 getValue(uint32 key) const
	{
		const uint32* valuePtr = mapFind(mCurrentValues, key);
		return (nullptr != valuePtr) ? *valuePtr : (key & 0xff);
	}

	inline void setValue(uint32 key, uint32 value)
	{
		mCurrentValues[key] = value;
	}

	inline uint32& accessValue(uint32 key)
	{
		uint32* valuePtr = mapFind(mCurrentValues, key);
		if (nullptr == valuePtr)
		{
			valuePtr = &mCurrentValues[key];
			*valuePtr = (key & 0xff);
		}
		return *valuePtr;
	}

	inline void serialize(VectorBinarySerializer& serializer)
	{
		if (serializer.isReading())
		{
			const size_t numValues = (size_t)serializer.read<uint16>();
			mCurrentValues.clear();
			mCurrentValues.reserve(numValues);
			for (size_t k = 0; k < numValues; ++k)
			{
				const uint32 key = serializer.read<uint32>();
				const uint32 value = serializer.read<uint32>();
				mCurrentValues[key] = value;
			}
		}
		else
		{
			serializer.writeAs<uint16>(mCurrentValues.size());
			for (const auto& pair : mCurrentValues)
			{
				serializer.write(pair.first);
				serializer.write(pair.second);
			}
		}
	}

private:
	std::unordered_map<uint32, uint32> mCurrentValues;	// Using "SharedDatabase::Setting::Type" as keys
};
