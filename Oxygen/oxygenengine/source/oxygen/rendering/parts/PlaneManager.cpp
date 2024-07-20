/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/rendering/parts/PlaneManager.h"
#include "oxygen/rendering/parts/PatternManager.h"
#include "oxygen/simulation/EmulatorInterface.h"


namespace
{
	void fillBufferByAbstraction(uint16* buffer, const Vec2i& cameraPosition, const Vec2i& screenSize)
	{
		// TODO: This is entirely S3AIR-specific
		const uint32 minCameraTileX = cameraPosition.x / 16;
		const uint32 minCameraTileY = cameraPosition.y / 16;

		const uint32 width = (screenSize.x + 15) / 16;		// Usually 32; measured in tiles, not patterns (so we have to multiply by 2 here and there to get patterns)
		const uint32 height = (screenSize.y + 15) / 16;		// Usually 16

		for (uint32 y = 0; y < height; ++y)
		{
			uint16* lineBase0 = &buffer[(y*2) * width*2];
			uint16* lineBase1 = &buffer[(y*2+1) * width*2];

			for (uint32 x = 0; x < width; ++x)
			{
				// Chunk coordinates
				const uint32 globalColumn = minCameraTileX + ((x - minCameraTileX) % width);
				const uint32 globalRow = minCameraTileY + ((y - minCameraTileY) % height);

				const uint32 chunkColumn = globalColumn / 8;
				const uint32 chunkRow = (globalRow / 8) & 0x1f;		// Looks like there are only 32 chunks in y-direction allowed in a level; that makes a maximum level height of 4096 pixels

				const uint32 chunkAddress = 0xffff0000 + EmulatorInterface::instance().readMemory16(0xffff8008 + chunkRow * 4) + chunkColumn;
				const uint8  chunkType = EmulatorInterface::instance().readMemory8(chunkAddress);

				// Tile coordinates inside chunk
				const uint32 tileColumn = globalColumn % 8;
				const uint32 tileRow = globalRow % 8;

				const uint32 tileAddress = 0xffff0000 + EmulatorInterface::instance().readMemory16(0x00f02a + chunkType * 2) + tileRow * 16 + tileColumn * 2;
				const uint16 tile = EmulatorInterface::instance().readMemory16(tileAddress);

				// Access tile graphics (consisting of 2x2 sprite patterns)
				const uint32 tilePatternBaseAddress = 0xffff9000 + (tile & 0x03ff) * 8;

				const bool flipX = (tile & 0x0400) != 0;
				const bool flipY = (tile & 0x0800) != 0;
				const uint32 mx = flipX ? 2 : 0;
				const uint32 my = flipY ? 4 : 0;
				const uint16 xorMask = (tile & 0x0c00) << 1;

				// Write 2x2 sprite patterns of this tile
				lineBase0[0] = EmulatorInterface::instance().readMemory16(tilePatternBaseAddress     + mx + my) ^ xorMask;
				lineBase0[1] = EmulatorInterface::instance().readMemory16(tilePatternBaseAddress + 2 - mx + my) ^ xorMask;
				lineBase1[0] = EmulatorInterface::instance().readMemory16(tilePatternBaseAddress + 4 + mx - my) ^ xorMask;
				lineBase1[1] = EmulatorInterface::instance().readMemory16(tilePatternBaseAddress + 6 - mx - my) ^ xorMask;

				lineBase0 += 2;
				lineBase1 += 2;
			}
		}
	}
}



PlaneManager::PlaneManager(PatternManager& patternManager) :
	mPatternManager(patternManager),
	mPlayfieldSize(64, 32)
{
}

void PlaneManager::reset()
{
	mNameTableBaseA = 0xc000;
	mNameTableBaseB = 0xe000;
	mNameTableBaseW = 0x8000;

	mPlayfieldSize.set(64, 32);
	mUsingPlaneW = false;
	mPlaneAWSplit = 0;

	resetCustomPlanes();
}

void PlaneManager::refresh()
{
	// Build plane pattern textures
	const bool isDeveloperMode = EngineMain::getDelegate().useDeveloperFeatures();
	const int numPatterns = mPlayfieldSize.x * mPlayfieldSize.y;
	for (int index = 0; index < 4; ++index)
	{
		uint16* buffer = mPlanePatternsBuffer[index];
		switch (index)
		{
			case PLANE_DEBUG:
			{
				if (isDeveloperMode)
				{
					for (uint16 k = 0; k < 0x800; ++k)
					{
						buffer[k] = k + ((uint16)mPatternManager.getLastUsedAtex(k) << 9);
					}
				}
				break;
			}

			default:
			{
				const uint16* src = getPlaneContent(index);
				memcpy(buffer, src, numPatterns * sizeof(uint16));
				if (isDeveloperMode)
				{
					for (int k = 0; k < numPatterns; ++k)
					{
						mPatternManager.setLastUsedAtex(src[k], (src[k] >> 9) & 0x70);
					}
				}
				break;
			}
		}
	}
}

void PlaneManager::resetCustomPlanes()
{
	for (int i = 0; i < 4; ++i)
		mDisabledDefaultPlane[i] = false;
	mCustomPlanes.clear();
}

bool PlaneManager::isPlaneUsed(int index) const
{
	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		if (index > PLANE_DEBUG)
			return false;
	}
	else
	{
		if (index >= PLANE_DEBUG)
			return false;
	}

	if (index == PLANE_W)
		return mUsingPlaneW;

	return true;
}

Vec2i PlaneManager::getPlayfieldSizeInPatterns() const
{
	return mPlayfieldSize;
}

Vec2i PlaneManager::getPlayfieldSizeInPixels() const
{
	const Vec2i playfieldSize = getPlayfieldSizeInPatterns();
	return Vec2i(playfieldSize.x * 8, playfieldSize.y * 8);
}

Vec4i PlaneManager::getPlayfieldSizeForShaders() const
{
	const Vec2i playfieldSize = getPlayfieldSizeInPatterns();
	return Vec4i(playfieldSize.x * 8, playfieldSize.y * 8, playfieldSize.x, playfieldSize.y);
}

void PlaneManager::setPlayfieldSizeInPatterns(const Vec2i& size)
{
	mPlayfieldSize = size;
}

void PlaneManager::setPlayfieldSizeInPixels(const Vec2i& size)
{
	setPlayfieldSizeInPatterns(size / 8);
}

const uint16* PlaneManager::getPlanePatternsBuffer(uint8 index) const
{
	RMX_ASSERT(index < 4, "Invalid plane index " << index);
	return mPlanePatternsBuffer[index];
}

uint16 PlaneManager::getPlaneBaseVRAMAddress(int planeIndex) const
{
	switch (planeIndex)
	{
		case PLANE_B:  return mNameTableBaseB;
		case PLANE_A:  return mNameTableBaseA;
		case PLANE_W:  return mNameTableBaseW;
	}
	RMX_ERROR("Invalid plane index", RMX_REACT_THROW);
	return 0;
}

const uint16* PlaneManager::getPlaneDataInVRAM(int planeIndex) const
{
	return (const uint16*)(EmulatorInterface::instance().getVRam() + getPlaneBaseVRAMAddress(planeIndex));
}

size_t PlaneManager::getPlaneSizeInVRAM(int planeIndex) const
{
	return (size_t)(mPlayfieldSize.x * mPlayfieldSize.y * 2);
}

uint16 PlaneManager::getPatternVRAMAddress(int planeIndex, uint16 patternIndex) const
{
	return getPlaneBaseVRAMAddress(planeIndex) + patternIndex * 2;
}

uint16 PlaneManager::getPatternAtIndex(int planeIndex, uint16 patternIndex) const
{
	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		if (planeIndex == PLANE_DEBUG)
			return patternIndex;
	}
	return *getPlaneContent(planeIndex, patternIndex);
}

void PlaneManager::setPatternAtIndex(int planeIndex, uint16 patternIndex, uint16 value)
{
	EmulatorInterface::instance().writeVRam16(getPatternVRAMAddress(planeIndex, patternIndex), value);
}

const uint16* PlaneManager::getPlaneContent(int planeIndex, uint16 patternIndex) const
{
	return (uint16*)(EmulatorInterface::instance().getVRam() + getPatternVRAMAddress(planeIndex, patternIndex));
}

void PlaneManager::setupPlaneW(bool use, uint16 splitY)
{
	mUsingPlaneW = use;
	mPlaneAWSplit = splitY;
}

void PlaneManager::dumpAsPaletteBitmap(PaletteBitmap& output, int planeIndex, bool highlightPrioPatterns) const
{
	Vec2i bitmapSize;
	if (planeIndex <= PLANE_A)
	{
		bitmapSize = getPlayfieldSizeInPixels();
	}
	else
	{
		bitmapSize.set(512, 256);
	}
	output.create(bitmapSize.x, bitmapSize.y);

	const PatternManager::CacheItem* patternCache = mPatternManager.getPatternCache();
	const uint16 numPatternsPerLine = (uint16)(bitmapSize.x / 8);

	uint8* dest = output.getData();
	for (int y = 0; y < bitmapSize.y; ++y)
	{
		for (int x = 0; x < bitmapSize.x; x += 8, dest += 8)
		{
			const uint16 patternIndex = getPatternAtIndex(planeIndex, (x / 8) + (y / 8) * numPatternsPerLine);
			const PatternManager::CacheItem::Pattern& pattern = patternCache[patternIndex & 0x07ff].mFlipVariation[(patternIndex >> 11) & 3];
			const uint8* srcPatternPixels = &pattern.mPixels[(x & 0x07) + (y & 0x07) * 8];
			const uint8 atex = (patternIndex >> 9) & 0x30;

			for (int k = 0; k < 8; ++k)
			{
				const uint8 colorIndex = srcPatternPixels[k];
				dest[k] = colorIndex + atex;
			}

			const bool lowerBrightness = (highlightPrioPatterns && (patternIndex & 0x8000) == 0);
			if (lowerBrightness)
			{
				for (int k = 0; k < 8; ++k)
				{
					dest[k] |= 0x80;
				}
			}
		}
	}
}

void PlaneManager::setDefaultPlaneEnabled(uint8 index, bool enabled)
{
	mDisabledDefaultPlane[index] = !enabled;
}

void PlaneManager::setupCustomPlane(const Recti& rect, uint8 sourcePlane, uint8 scrollOffsets, uint16 renderQueue)
{
	CustomPlane& plane = vectorAdd(mCustomPlanes);
	plane.mRect = rect;
	plane.mSourcePlane = sourcePlane;
	plane.mScrollOffsets = scrollOffsets;
	plane.mRenderQueue = renderQueue;
}

void PlaneManager::serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	serializer.serialize(mNameTableBaseA);
	serializer.serialize(mNameTableBaseB);

	if (serializer.isReading())
	{
		Vec2i playfieldSize;
		playfieldSize.x = serializer.read<uint16>();
		playfieldSize.y = serializer.read<uint16>();
		setPlayfieldSizeInPixels(playfieldSize);
	}
	else
	{
		const Vec2i playfieldSize = getPlayfieldSizeInPixels();
		serializer.write<uint16>(playfieldSize.x);
		serializer.write<uint16>(playfieldSize.y);
	}

	if (formatVersion >= 4)
	{
		serializer.serialize(mNameTableBaseW);
		serializer.serializeAs<uint8>(mUsingPlaneW);
		serializer.serialize(mPlaneAWSplit);

		for (int k = 0; k < 4; ++k)
		{
			serializer.serialize(mDisabledDefaultPlane[k]);
		}

		serializer.serializeArraySize(mCustomPlanes, 64);
		for (CustomPlane& customPlane : mCustomPlanes)
		{
			serializer.serializeAs<int16>(customPlane.mRect.x);
			serializer.serializeAs<int16>(customPlane.mRect.y);
			serializer.serializeAs<int16>(customPlane.mRect.width);
			serializer.serializeAs<int16>(customPlane.mRect.height);
			serializer.serialize(customPlane.mSourcePlane);
			serializer.serialize(customPlane.mScrollOffsets);
			serializer.serialize(customPlane.mRenderQueue);
		}
	}
}
