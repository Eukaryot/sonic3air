/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class EmulatorInterface;
class PaletteBitmap;


class BlueSpheresRendering
{
public:
	void startup();

	void createSprites(Vec2i screenSize);
	void writeVisibleSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation, EmulatorInterface& emulatorInterface);

private:
	bool loadLookupData();
	void performLookupCalculations();

private:
	bool mInitializedLookups = false;
	std::vector<uint8> mStraightIntensityLookup[0x20];
	std::vector<uint8> mRotationIntensityLookup[0x0f];
	int mNonOpaquePixelIndent[224] = { 0 };				// Number of pixels from the left (or right) side until reaching the first fully opaque one, for each row
	int mNumPureSkyRows = 0;

	Vec2i mLastScreenSize;
	uint32 mLastSpriteCacheChangeCounter = 0;
};
