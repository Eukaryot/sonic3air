/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class PatternManager;


class PlaneManager
{
public:
	enum PlaneType
	{
		PLANE_B		= 0,	// Plane B, background
		PLANE_A		= 1,	// Plane A, foreground
		PLANE_W		= 2,	// Plane W, window (optionally split out of plane A)
		PLANE_DEBUG	= 3		// Additional plane containing all patterns for debug output
	};

	struct CustomPlane
	{
		Recti mRect;
		uint8 mSourcePlane = 0;
		uint8 mScrollOffsets = 0;
		uint16 mRenderQueue = 0;
	};

public:
	PlaneManager(PatternManager& patternManager);

	void reset();
	void refresh();

	void resetCustomPlanes();
	bool isPlaneUsed(int index) const;

	inline uint16 getNameTableBaseB() const  { return mNameTableBaseB; }
	inline uint16 getNameTableBaseA() const  { return mNameTableBaseA; }
	inline uint16 getNameTableBaseW() const  { return mNameTableBaseW; }
	inline void setNameTableBaseB(uint16 vramAddress)  { mNameTableBaseB = vramAddress; }
	inline void setNameTableBaseA(uint16 vramAddress)  { mNameTableBaseA = vramAddress; }
	inline void setNameTableBaseW(uint16 vramAddress)  { mNameTableBaseW = vramAddress; }

	Vec2i getPlayfieldSizeInPatterns() const;
	Vec2i getPlayfieldSizeInPixels() const;
	Vec4i getPlayfieldSizeForShaders() const;
	void setPlayfieldSizeInPatterns(const Vec2i& size);
	void setPlayfieldSizeInPixels(const Vec2i& size);

	const uint16* getPlanePatternsBuffer(uint8 index) const;

	uint16 getPlaneBaseVRAMAddress(int planeIndex) const;
	const uint16* getPlaneDataInVRAM(int planeIndex) const;
	size_t getPlaneSizeInVRAM(int planeIndex) const;

	uint16 getPatternVRAMAddress(int planeIndex, uint16 patternIndex) const;
	uint16 getPatternAtIndex(int planeIndex, uint16 patternIndex) const;
	void setPatternAtIndex(int planeIndex, uint16 patternIndex, uint16 value);

	inline bool isPlaneWBelowSplitY() const  { return mIsPlaneWBelowSplitY; }
	inline uint16 getPlaneAWSplitY() const	 { return mPlaneAWSplitY; }
	void setupPlaneW(bool isPlaneWBelowSplit, uint16 splitY);
	Recti getPlaneRect(int planeIndex, const Recti& fullscreenRect) const;

	void dumpAsPaletteBitmap(PaletteBitmap& output, int planeIndex, bool highlightPrioPatterns = false) const;

	inline bool isDefaultPlaneEnabled(uint8 index) const  { return !mDisabledDefaultPlane[index]; }
	void setDefaultPlaneEnabled(uint8 index, bool enabled);

	const std::vector<CustomPlane>& getCustomPlanes() const  { return mCustomPlanes; }
	void setupCustomPlane(const Recti& rect, uint8 sourcePlane, uint8 scrollOffsets, uint16 renderQueue);

	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion);

private:
	const uint16* getPlaneContent(int planeIndex, uint16 patternIndex = 0) const;

private:
	PatternManager& mPatternManager;

	uint16 mNameTableBaseA = 0xc000;
	uint16 mNameTableBaseB = 0xe000;
	uint16 mNameTableBaseW = 0x8000;

	Vec2i mPlayfieldSize;		// In patterns (8x8 pixels)
	uint16 mPlanePatternsBuffer[4][0x1000] = { 0 };		// Enough space to support 128 x 32 patterns (though usually only 0x800 is needed, for 64 x 32 patterns)

	bool mIsPlaneWBelowSplitY = false;	// If true, plane W is below plane A, otherwise it's above plane A
	uint16 mPlaneAWSplitY = 0;

	bool mDisabledDefaultPlane[4];
	std::vector<CustomPlane> mCustomPlanes;
};
