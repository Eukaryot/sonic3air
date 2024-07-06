/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class SpacesManager
{
public:
	enum class Space
	{
		SCREEN,
		WORLD
	};

public:
	inline const Vec2i& getWorldSpaceOffset() const  { return mWorldSpaceOffset; }
	inline void setWorldSpaceOffset(const Vec2i& offset)  { mWorldSpaceOffset = offset; }

	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
	{
		if (formatVersion >= 6)
		{
			serializer.serializeAs<int32>(mWorldSpaceOffset.x);
			serializer.serializeAs<int32>(mWorldSpaceOffset.y);
		}
	}

private:
	Vec2i mWorldSpaceOffset;
};
