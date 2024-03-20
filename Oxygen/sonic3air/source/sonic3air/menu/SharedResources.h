/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/drawing/DrawerTexture.h"


namespace global
{
	struct ZoneActPreviewKey
	{
		uint8 mZone = 0;
		uint8 mAct = 0;
		uint8 mImage = 0;

		inline bool operator<(const ZoneActPreviewKey& other) const  { return (mZone != other.mZone) ? (mZone < other.mZone) : (mAct != other.mAct) ? (mAct < other.mAct) : (mImage < other.mImage); }
	};

	extern Font mSmallfont;
	extern Font mSmallfontSemiOutlined;
	extern Font mSmallfontRect;
	extern Font mOxyfontNarrowSimple;
	extern Font mOxyfontNarrow;
	extern Font mOxyfontTinySimple;
	extern Font mOxyfontTiny;
	extern Font mOxyfontTinyRect;
	extern Font mOxyfontSmallNoOutline;
	extern Font mOxyfontSmall;
	extern Font mOxyfontRegular;
	extern Font mSonicFontB;
	extern Font mSonicFontC;

	extern DrawerTexture mMainMenuBackgroundSeparator;
	extern DrawerTexture mDataSelectBackground;
	extern DrawerTexture mDataSelectAltBackground;
	extern DrawerTexture mLevelSelectBackground;
	extern DrawerTexture mOptionsTopBar;
	extern DrawerTexture mAchievementsFrame;
	extern DrawerTexture mTimeAttackResultsBG;

	extern std::map<ZoneActPreviewKey, uint64> mZoneActPreviewSpriteKeys;
	extern std::map<uint32, DrawerTexture> mAchievementImage;
	extern std::map<uint32, DrawerTexture> mSecretImage;

	void loadSharedResources();
}
