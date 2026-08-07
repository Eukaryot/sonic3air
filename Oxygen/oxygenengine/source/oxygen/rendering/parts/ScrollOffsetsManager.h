/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/RenderPartsDefinitions.h"


class PlaneManager;

class ScrollOffsetsManager
{
public:
	ScrollOffsetsManager(PlaneManager& planeManager);

	void reset();
	void refresh(const RefreshParameters& refreshParameters);
	void preFrameUpdate();
	void postFrameUpdate();

	void resetOverwriteFlags();

	inline bool getVerticalScrolling() const				{ return mVerticalScrolling; }
	inline void setVerticalScrolling(bool enable)			{ mVerticalScrolling = enable; }

	inline uint8 getHorizontalScrollMask() const			{ return mHorizontalScrollMask; }
	inline void setHorizontalScrollMask(uint8 scrollMask)	{ mHorizontalScrollMask = scrollMask; }

	inline uint16 getHorizontalScrollTableBase() const				{ return mHorizontalScrollTableBase; }
	inline void setHorizontalScrollTableBase(uint16 vramAddress)	{ mHorizontalScrollTableBase = vramAddress; }

	bool getHorizontalScrollNoRepeat(int setIndex) const;
	void setHorizontalScrollNoRepeat(int setIndex, bool enable);

	void overwriteScrollOffsetH(int setIndex, int index, uint16 value);
	void overwriteScrollOffsetV(int setIndex, int index, uint16 value);

	const uint16* getScrollOffsetsH(int setIndex) const;
	const uint16* getScrollOffsetsV(int setIndex) const;

	inline const Vec2i& getPlaneWScrollOffset() const			 { return mScrollOffsetW; }
	inline void setPlaneWScrollOffset(const Vec2i& scrollOffset) { mScrollOffsetW = scrollOffset; }

	inline int16 getVerticalScrollOffsetBias() const	{ return mVerticalScrollOffsetBias; }
	void setVerticalScrollOffsetBias(int16 bias);

	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion);

private:
	PlaneManager& mPlaneManager;

	bool mVerticalScrolling = false;
	uint8 mHorizontalScrollMask = 0xff;
	uint16 mHorizontalScrollTableBase = 0xf000;

	struct ScrollOffsetSet
	{
		uint16 mScrollOffsetsH[0x100]   = { 0 };	// One scroll offset per single pixel line
		bool mExplicitOverwriteH[0x100] = { 0 };	// One flag per horizontal scroll offset; set if it was explicitly overwritten
		uint16 mScrollOffsetsV[0x20]    = { 0 };	// One scroll offset per row of 0x10 pixels
		bool mExplicitOverwriteV[0x20]  = { 0 };	// One flag per vertical scroll offset; set if it was explicitly overwritten
		bool mHorizontalScrollNoRepeat  = false;
	};
	ScrollOffsetSet mSets[4];		// First two are for the planes, the others are used for certain effects that require an additional set of scroll offsets
	Vec2i mScrollOffsetW;
	int16 mVerticalScrollOffsetBias = 0;

	// Experimental frame interpolation support
	struct InterpolatedScrollOffsetSet
	{
		bool mValid = false;
		bool mHasLastScrollOffsets = false;
		uint16 mInterpolatedScrollOffsetsH[0x100] = { 0 };
		uint16 mInterpolatedScrollOffsetsV[0x20] = { 0 };
		uint16 mLastScrollOffsetsH[0x100] = { 0 };
		uint16 mLastScrollOffsetsV[0x20] = { 0 };
		int16 mDifferenceScrollOffsetsH[0x100] = { 0 };
		int16 mDifferenceScrollOffsetsV[0x20] = { 0 };
	};
	InterpolatedScrollOffsetSet mInterpolatedSets[4];
};
