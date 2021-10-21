/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class EmulatorInterface;


class BlueSpheresRendering
{
public:
	void startup();

	void renderToBitmap(Bitmap& bitmapOpaque, Bitmap& bitmapAlpha, int screenWidth, uint16 px, uint16 py, uint8 rotation, uint16 fieldColorA, uint16 fieldColorB);
	void writeVisibleSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation, EmulatorInterface& emulatorInterface);

private:
	bool loadLookupData();
	void performLookupCalculations();

private:
	bool mInitializedLookups = false;
	std::vector<uint8> mVisibilityLookup;
	std::vector<uint8> mStraightIntensityLookup[0x20];
	std::vector<uint8> mRotationIntensityLookup[0x0f];
	int mNumPureSkyRows;
	int mNumPureGroundRows;
	int mPureRowsForWidth = 0;

	bool mLastFiltering = false;
	uint16 mLastFieldColorA = 0;
	uint16 mLastFieldColorB = 0;
	uint32 mMixedFieldColorLookup[0x100];
	uint32 mMixedFieldColorLookupInverse[0x100];
};
