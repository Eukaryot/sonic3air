/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/BlueSpheresRendering.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"
#include "oxygen/resources/SpriteCache.h"
#include "oxygen/simulation/EmulatorInterface.h"

//#define OUTPUT_FOG_BITMAPS


namespace
{
	static const constexpr int LOOKUP_WIDTH  = 496;
	static const constexpr int LOOKUP_HEIGHT = 224;

	static float CAMERA_POSITION_HEIGHT = 0.196f;
	static float CAMERA_POSITION_BACKANGLE = 6.2f;
	static float CAMERA_TARGET_ANGLE = 50.75f;
	static float PLANE_DISTANCE = 260.0f;
	static float GRID_SIZE = 11.5f;

	inline Color colorFromCompact(uint16 compact)
	{
		return Color((float)(compact & 0x000e) / 14.0f, (float)((compact >> 4) & 0x000e) / 14.0f, (float)((compact >> 8) & 0x000e) / 14.0f);
	}

	float getIntegerCutFraction(float center, float extend1, float extend2)
	{
		extend1 = std::fabs(extend1);
		extend2 = std::fabs(extend2);
		const float minPosition = center - extend1 - extend2;
		const float maxPosition = center + extend1 + extend2;

		const float intensityLevel = ((int)floor(minPosition) & 0x01) ? 1.0f : 0.0f;

		const float cutPosition = floor(maxPosition);
		if (minPosition >= cutPosition)
		{
			return intensityLevel;
		}

		const float innerPosition = center - std::fabs(extend1 - extend2);

		const float size = (extend1 + extend2) * 2.0f;
		const float cutOffset = (cutPosition - minPosition) / size;
		const float innerOffset = (innerPosition - minPosition) / size;

		const float centralSize = 1.0f - 2.0f * innerOffset;
		const float triangleArea = 0.5f * innerOffset;
		const float centralArea = centralSize;

		float cutArea;
		if (cutOffset < innerOffset)
		{
			const float fraction = cutOffset / innerOffset;
			cutArea = (fraction * fraction) * triangleArea;
		}
		else if (cutOffset > 1.0f - innerOffset)
		{
			const float fraction = (1.0f - cutOffset) / innerOffset;
			cutArea = triangleArea + centralArea + (1.0f - fraction * fraction) * triangleArea;
		}
		else
		{
			const float fraction = (cutOffset - innerOffset) / centralSize;
			cutArea = triangleArea + fraction * centralArea;
		}

		const float relativeArea = cutArea / (triangleArea * 2.0f + centralArea);
		return interpolate(1.0f - intensityLevel, intensityLevel, relativeArea);
	}

	void getCameraTransform(Vec3f& position, Vec3f& front, Vec3f& right, Vec3f& up)
	{
		position.set(0.0f, 0.0f, 1.0f + CAMERA_POSITION_HEIGHT);
		position.rotate(CAMERA_POSITION_BACKANGLE, 0);

		front.set(0.0f, 1.0f, 0.0f);
		front.rotate(CAMERA_POSITION_BACKANGLE - CAMERA_TARGET_ANGLE, 0);
		front.normalize();

		right.set(1.0f, 0.0f, 0.0f);

		up.cross(right, front);
		up.normalize();
	}

	void getCharacterTransform(Vec2f& position, Vec4f& transform, uint16 px, uint16 py, uint8 rotation)
	{
		const bool isRotating = (rotation & 0x3f) != 0;

		position.x = -(float)px / 256.0f;
		position.y =  (float)py / 256.0f;
		if (isRotating || (rotation & 0x40) == 0)
			position.x = round(position.x);
		if (isRotating || (rotation & 0x40) != 0)
			position.y = round(position.y);

		const float angle = (float)rotation / 128.0f * PI_FLOAT;
		transform.x =  std::cos(angle);
		transform.y = -std::sin(angle);
		transform.z =  std::sin(angle);
		transform.w =  std::cos(angle);
	}

	uint8 getPaletteIndex(float intensity, float opacity)
	{
		const int opacityStep = roundToInt(opacity * 13.0f);
		if (opacityStep == 0)
		{
			return 0;
		}
		else if (opacityStep <= 12)
		{
			return 65 + (12 - opacityStep) * 16 + roundToInt(intensity * 14.0f);
		}
		else
		{
			const uint8 value = roundToInt(intensity * 59.0f);
			return (value + 1) + (value / 15);
		}
	}
}


void BlueSpheresRendering::startup()
{
	if (!mInitializedLookups)
	{
	#ifndef OUTPUT_FOG_BITMAPS
		// Try to load lookup data from a cache file, otherwise calculate the data
		if (loadLookupData())
		{
			mInitializedLookups = true;
		}
		else
	#endif
		{
			performLookupCalculations();
		}
	}
}

void BlueSpheresRendering::createSprites(Vec2i screenSize)
{
	SpriteCache& spriteCache = SpriteCache::instance();

	// Update sprites only if needed
	if (mLastScreenSize == screenSize && mLastSpriteCacheChangeCounter == spriteCache.getGlobalChangeCounter())
		return;

	// Perform calculations that only need to be done once
	if (!mInitializedLookups)
	{
		performLookupCalculations();
	}

	mLastScreenSize = screenSize;
	const int maxX = std::min(LOOKUP_WIDTH, screenSize.x);
	const int maxY = LOOKUP_HEIGHT;
	const int indentX = (screenSize.x - maxX) / 2;
	const int offsetX = (LOOKUP_WIDTH - maxX) / 2;

	int numPureGroundRows = 0;
	for (int row = LOOKUP_HEIGHT - 1; ; --row)
	{
		if (mNonOpaquePixelIndent[row] > offsetX)
			break;
		++numPureGroundRows;
	}

	// Build or update all sprites
	for (int index = 0; index < 0x2f; ++index)
	{
		const uint8* lookupDataBase;
		uint64 spriteKeys[2];
		if (index < 0x20)
		{
			lookupDataBase = &mStraightIntensityLookup[index][0];
			spriteKeys[0] = rmx::getMurmur2_64(String(0, "bluespheres_ground_alpha_movement_0x%02x", index));
			spriteKeys[1] = rmx::getMurmur2_64(String(0, "bluespheres_ground_opaque_movement_0x%02x", index));
		}
		else
		{
			lookupDataBase = &mRotationIntensityLookup[index - 0x20][0];
			spriteKeys[0] = rmx::getMurmur2_64(String(0, "bluespheres_ground_alpha_rotation_0x%02x", index - 0x1f));
			spriteKeys[1] = rmx::getMurmur2_64(String(0, "bluespheres_ground_opaque_rotation_0x%02x", index - 0x1f));
		}

		SpriteCache::CacheItem* items[2];
		PaletteBitmap* bitmaps[2];
		for (int k = 0; k < 2; ++k)
		{
			SpriteCache::CacheItem& item = SpriteCache::instance().getOrCreatePaletteSprite(spriteKeys[k]);
			++item.mChangeCounter;
			bitmaps[k] = &static_cast<PaletteSprite*>(item.mSprite)->accessBitmap();
			items[k] = &item;
		}

		// Ignore the first rows that are just sky, so completely transparent

		// Then we have some rows of pixels that can be anything -- sky or ground or something in between
		const int numRowsUntilPureGround = LOOKUP_HEIGHT - numPureGroundRows;
		bitmaps[0]->create(maxX, numRowsUntilPureGround - mNumPureSkyRows);
		for (int y = mNumPureSkyRows; y < numRowsUntilPureGround; ++y)
		{
			uint8* output = bitmaps[0]->getPixelPointer(indentX, y - mNumPureSkyRows);
			const uint8* lookupData = &lookupDataBase[y * LOOKUP_WIDTH + offsetX];
			memcpy(output, lookupData, maxX);
		}

		// And the rest is only ground
		bitmaps[1]->create(maxX, numPureGroundRows);
		for (int y = numRowsUntilPureGround; y < maxY; ++y)
		{
			uint8* output = bitmaps[1]->getPixelPointer(indentX, y - numRowsUntilPureGround);
			const uint8* lookupData = &lookupDataBase[y * LOOKUP_WIDTH + offsetX];
			memcpy(output, lookupData, maxX);
		}

		items[0]->mSprite->mOffset.y = screenSize.y - bitmaps[0]->getHeight() - bitmaps[1]->getHeight();
		items[1]->mSprite->mOffset.y = screenSize.y - bitmaps[1]->getHeight();
	}

	mLastSpriteCacheChangeCounter = spriteCache.getGlobalChangeCounter();
}

void BlueSpheresRendering::writeVisibleSpheresData(uint32 targetAddress, uint32 sourceAddress, uint16 px, uint16 py, uint8 rotation, EmulatorInterface& emulatorInterface)
{
	Vec3f cameraPosition;
	Vec3f cameraFront;
	Vec3f cameraRight;
	Vec3f cameraUp;
	getCameraTransform(cameraPosition, cameraFront, cameraRight, cameraUp);

	Vec2f characterPosition;
	Vec4f characterTransform;
	getCharacterTransform(characterPosition, characterTransform, px, py, rotation);

	uint8* originalOutputPtr = emulatorInterface.getMemoryPointer(targetAddress, true, 17 * 17 * 7);
	uint8* outputPtr = originalOutputPtr + 2;

	uint16 count = 0;
	for (int dy = -8; dy <= 8; ++dy)
	{
		for (int dx = -8; dx <= 8; ++dx)
		{
			const int x = roundToInt(characterPosition.x) + dx;
			const int y = roundToInt(characterPosition.y) + dy;

			const uint8 sphereType = emulatorInterface.readMemory8(sourceAddress + ((-x) & 0x1f) + (y & 0x1f) * 0x20);
			if (sphereType == 0)
				continue;

			const Vec2f gridPosition = characterPosition - Vec2f((float)x, (float)y);
			Vec2f pos;
			pos.x = (gridPosition.x * characterTransform.x + gridPosition.y * characterTransform.z) / GRID_SIZE;
			pos.y = (gridPosition.x * characterTransform.y + gridPosition.y * characterTransform.w) / GRID_SIZE;

			Vec3f viewCoords;
			{
				const float sqrLen = pos.sqrLen();
				if (sqrLen >= 1.0f)
					continue;

				const Vec3f worldCoords(pos.x, pos.y, std::sqrt(1.0f - sqrLen));
				const Vec3f camRelative = worldCoords - cameraPosition;
				viewCoords.x = camRelative.dot(cameraRight);
				viewCoords.y = camRelative.dot(cameraFront);
				viewCoords.z = camRelative.dot(cameraUp);
				if (viewCoords.y < 0.0f)
					continue;

				viewCoords.x *= PLANE_DISTANCE / viewCoords.y;
				viewCoords.z *= PLANE_DISTANCE / viewCoords.y;
			}

			uint16 size = roundToInt((float)0x1000 / viewCoords.y);
			if (size < 0x1400)
				continue;

			*(uint16*)(&outputPtr[0]) = swapBytes16(roundToInt(199.5f + viewCoords.x));
			*(uint16*)(&outputPtr[2]) = swapBytes16(roundToInt(111.5f - viewCoords.z));
			*(uint16*)(&outputPtr[4]) = swapBytes16(size);
			*(uint8*) (&outputPtr[6]) = sphereType;

			outputPtr += 7;
			++count;
		}
	}

	*(uint16*)(&originalOutputPtr[0]) = swapBytes16(count);
}

bool BlueSpheresRendering::loadLookupData()
{
	std::vector<uint8> data;
	if (!FTX::FileSystem->readFile(L"data/binary/bluespheresrendering.bin", data))
		return false;

	uint16 width, height;
	std::vector<uint8> uncompressed;
	{
		VectorBinarySerializer serializer(true, data);

		char signature[5] = { 0 };
		serializer.read(signature, 4);
		int formatVersion = 3;
		if (memcmp(signature, "BSL3", 4) != 0)
		{
			if (memcmp(signature, "BSL2", 4) != 0)
				return false;

			// Version 2 did not use compression yet
			formatVersion = 2;
		}

		serializer & width;
		serializer & height;
		if (width != LOOKUP_WIDTH || height != LOOKUP_HEIGHT)
			return false;

		if (formatVersion >= 3)
		{
			const size_t compressedSize = (size_t)serializer.read<uint32>();
			ZlibDeflate::decode(uncompressed, serializer.peek(), compressedSize);
		}
		else
		{
			uncompressed.resize(data.size() - serializer.getReadPosition());
			memcpy(&uncompressed[0], &data[serializer.getReadPosition()], uncompressed.size());
		}
	}

	{
		VectorBinarySerializer serializer(true, uncompressed);
		const int pixels = width * height;

		serializer.serialize(mNumPureSkyRows);
		serializer.read(&mNonOpaquePixelIndent[0], sizeof(mNonOpaquePixelIndent));

		for (int i = 0; i < 0x2f; ++i)
		{
			std::vector<uint8>& lookup = (i < 0x20) ? mStraightIntensityLookup[i] : mRotationIntensityLookup[i - 0x20];
			lookup.resize(pixels);
			serializer.read(&lookup[0], width * height);
		}
	}

	return true;
}

void BlueSpheresRendering::performLookupCalculations()
{
	const int width  = LOOKUP_WIDTH;
	const int height = LOOKUP_HEIGHT;
	const int pixels = width * height;

	struct CachedPixelData
	{
		bool mHitsGround = false;
		float mFogAlpha = 0.0f;
		Vec3f mHitPosition;
		Vec3f mHitTangentX;
		Vec3f mHitTangentY;
	};
	std::vector<CachedPixelData> cachedPixelData;
	std::vector<float> visibilityLookup;
	cachedPixelData.resize(pixels);
	visibilityLookup.resize(pixels);

	// First fill visibility lookup and cached pixel data, which we need for the further calculations
	{
		Vec3f cameraPosition;
		Vec3f cameraFront;
		Vec3f cameraRight;
		Vec3f cameraUp;
		getCameraTransform(cameraPosition, cameraFront, cameraRight, cameraUp);

	#ifdef OUTPUT_FOG_BITMAPS
		Bitmap fogBackgroundBitmap;
		Bitmap fogForegroundBitmap;
		fogBackgroundBitmap.create(width, height, 0);
		fogForegroundBitmap.create(width, height, 0);
	#endif

		for (int pixelIndex = 0; pixelIndex < pixels; ++pixelIndex)
		{
			CachedPixelData& pixelData = cachedPixelData[pixelIndex];

			const float fx = ((float)(pixelIndex % width) - (float)(width - 1) * 0.5f) / PLANE_DISTANCE;
			const float fy = ((float)(pixelIndex / width) - (float)(height - 1) * 0.5f) / PLANE_DISTANCE;

			const Vec3f viewstart = cameraPosition;
			const Vec3f viewdir = (cameraFront + cameraRight * fx - cameraUp * fy).normalized();

			const float centerDistanceAlongView = cameraPosition.dot(viewdir);
			const float horizonDistance = (cameraPosition - viewdir * centerDistanceAlongView).length() - 1.0f;

			pixelData.mHitsGround = (horizonDistance < 0.0f);
			float groundVisibility = 0.0f;

			if (pixelData.mHitsGround)
			{
				// Intersection with unit sphere, using quadratic formula
				//  -> Coefficient a would be viewdir.sqrLen(), but that is 1
				const float b = 2.0f * viewstart.dot(viewdir);
				const float c = viewstart.sqrLen() - 1.0f;

				const float D = b * b - 4.0f * c;
				RMX_CHECK(D >= 0.0f, "D must be at least 0 here", );

				const float distance = (-b - sqrt(D)) / 2.0f;
				const Vec3f hitPoint = viewstart + viewdir * distance;
				const Vec3f hitNormal = hitPoint.normalized();

				pixelData.mHitPosition = hitPoint;

				// Calculate tangent vectors in 3D space representing projection of half a pixel to the right and up
				const float normalV = -hitNormal.dot(viewdir);
				const float normalR = hitNormal.dot(cameraRight);
				const float normalU = hitNormal.dot(cameraUp);
				const float pixelExtend = 0.5f * distance / PLANE_DISTANCE;

				pixelData.mHitTangentX = (cameraRight + viewdir * normalR / normalV) * pixelExtend;
				pixelData.mHitTangentY = (cameraUp + viewdir * normalU / normalV) * pixelExtend;

				// Ground is visible (at least partially)
				groundVisibility = saturate(-horizonDistance * 500.0f);
				const float exp = std::exp(-saturate(distance - 0.4f) * 2.0f);
				const float alpha = saturate(1.0f - exp * exp);
				pixelData.mFogAlpha = alpha;

			#ifdef OUTPUT_FOG_BITMAPS
				fogForegroundBitmap.getData()[pixelIndex] = 0xffffff + (roundToInt(alpha * groundVisibility * 192.0f) << 24);
			#endif
			}

			if (groundVisibility < 1.0f)
			{
				// Sky is visible (at least partially)
				const float alpha = saturate(1.0f - horizonDistance * 50.0f) * 0.4f;
				pixelData.mFogAlpha = interpolate(alpha, pixelData.mFogAlpha, groundVisibility);

			#ifdef OUTPUT_FOG_BITMAPS
				fogBackgroundBitmap.getData()[pixelIndex] = 0xffffff + (roundToInt(alpha * 255.0f) << 24);
			#endif
			}

			visibilityLookup[pixelIndex] = groundVisibility;
		}

		// How many rows are just pure sky, with empty pixels?
		mNumPureSkyRows = 0;
		int row = 0;
		{
			bool allSky = true;
			for (; row < 224; ++row)
			{
				const float* ptr = &visibilityLookup[row * LOOKUP_WIDTH];
				for (int x = 0; x < LOOKUP_WIDTH / 2; ++x)
				{
					if (ptr[x] > 0.001f)
					{
						allSky = false;
						break;
					}
				}
				if (!allSky)
					break;

				mNonOpaquePixelIndent[row] = LOOKUP_WIDTH / 2;
				++mNumPureSkyRows;
			}
		}

		// All remaining rows contain at least some parts of the ground
		//  -> Now get the number of not fully opaque pixels from the left of each row
		for (; row < 224; ++row)
		{
			const float* ptr = &visibilityLookup[row * LOOKUP_WIDTH];
			int indent = 0;
			for (; indent < LOOKUP_WIDTH / 2; ++indent)
			{
				if (ptr[indent] > 0.999f)
					break;
			}
			mNonOpaquePixelIndent[row] = indent;
		}

	#ifdef OUTPUT_FOG_BITMAPS
		fogForegroundBitmap.save(L"glow_foreground.png");
		fogBackgroundBitmap.save(L"glow_background.png");
	#endif
	}

	// Calculate lookups for straight movement
	for (int movementStep = 0; movementStep < 0x20; ++movementStep)
	{
		std::vector<uint8>& lookupTable = mStraightIntensityLookup[movementStep];
		if (lookupTable.empty())
		{
			lookupTable.resize(pixels, 0);
			uint8* lookupData = &lookupTable[0];

			const float characterPositionY = (float)movementStep / 32.0f;

			for (int pixelIndex = 0; pixelIndex < pixels; ++pixelIndex)
			{
				CachedPixelData& pixelData = cachedPixelData[pixelIndex];
				if (pixelData.mHitsGround)
				{
					Vec2f gridPosition;
					Vec2f gridExtendX;
					Vec2f gridExtendY;
					gridPosition.x = pixelData.mHitPosition.x * GRID_SIZE;
					gridPosition.y = pixelData.mHitPosition.y * GRID_SIZE + characterPositionY;
					gridExtendX.x  = pixelData.mHitTangentX.x * GRID_SIZE;
					gridExtendX.y  = pixelData.mHitTangentX.y * GRID_SIZE;
					gridExtendY.x  = pixelData.mHitTangentY.x * GRID_SIZE;
					gridExtendY.y  = pixelData.mHitTangentY.y * GRID_SIZE;

					constexpr float extendsFactor = 0.9f;	// Slightly shorten the extends to reduce the filter's blurriness
					const float cutFractionX = getIntegerCutFraction(gridPosition.x, gridExtendX.x * extendsFactor, gridExtendY.x * extendsFactor);
					const float cutFractionY = getIntegerCutFraction(gridPosition.y, gridExtendX.y * extendsFactor, gridExtendY.y * extendsFactor);
					const float intensity = interpolate(cutFractionX, 1.0f - cutFractionX, cutFractionY);

					lookupData[pixelIndex] = getPaletteIndex(intensity, visibilityLookup[pixelIndex]);
				}
			}
		}
	}

	// Calculate lookups for rotation
	for (int rotationStep = 1; rotationStep < 0x10; ++rotationStep)
	{
		std::vector<uint8>& lookupTable = mRotationIntensityLookup[rotationStep-1];
		if (lookupTable.empty())
		{
			lookupTable.resize(pixels, 0);
			uint8* lookupData = &lookupTable[0];

			const float angle = (float)rotationStep / 32.0f * PI_FLOAT;
			const float sine   = std::sin(angle) * GRID_SIZE;
			const float cosine = std::cos(angle) * GRID_SIZE;

			for (int pixelIndex = 0; pixelIndex < pixels; ++pixelIndex)
			{
				CachedPixelData& pixelData = cachedPixelData[pixelIndex];
				if (pixelData.mHitsGround)
				{
					Vec2f gridPosition;
					Vec2f gridExtendX;
					Vec2f gridExtendY;
					gridPosition.x = pixelData.mHitPosition.x * cosine - pixelData.mHitPosition.y * sine;
					gridPosition.y = pixelData.mHitPosition.x * sine   + pixelData.mHitPosition.y * cosine;
					gridExtendX.x  = pixelData.mHitTangentX.x * cosine - pixelData.mHitTangentX.y * sine;
					gridExtendX.y  = pixelData.mHitTangentX.x * sine   + pixelData.mHitTangentX.y * cosine;
					gridExtendY.x  = pixelData.mHitTangentY.x * cosine - pixelData.mHitTangentY.y * sine;
					gridExtendY.y  = pixelData.mHitTangentY.x * sine   + pixelData.mHitTangentY.y * cosine;

					constexpr float extendsFactor = 0.9f;	// Slightly shorten the extends to reduce the filter's blurriness
					const float cutFractionX = getIntegerCutFraction(gridPosition.x, gridExtendX.x * extendsFactor, gridExtendY.x * extendsFactor);
					const float cutFractionY = getIntegerCutFraction(gridPosition.y, gridExtendX.y * extendsFactor, gridExtendY.y * extendsFactor);
					const float intensity = interpolate(cutFractionX, 1.0f - cutFractionX, cutFractionY);

					lookupData[pixelIndex] = getPaletteIndex(intensity, visibilityLookup[pixelIndex]);
				}
			}
		}
	}

	// Save the results to a cache file
	{
		std::vector<uint8> data;
		{
			VectorBinarySerializer serializer(false, data);

			serializer.write(mNumPureSkyRows);
			serializer.write(&mNonOpaquePixelIndent[0], sizeof(mNonOpaquePixelIndent));

			for (int i = 0; i < 0x2f; ++i)
			{
				std::vector<uint8>& lookup = (i < 0x20) ? mStraightIntensityLookup[i] : mRotationIntensityLookup[i - 0x20];
				serializer.write(&lookup[0], width * height);
			}
		}

		{
			// Save with compression
			std::vector<uint8> compressed;
			ZlibDeflate::encode(compressed, &data[0], data.size(), 9);

			data.clear();
			VectorBinarySerializer serializer(false, data);

			serializer.write("BSL3", 4);
			serializer.write<uint16>(LOOKUP_WIDTH);
			serializer.write<uint16>(LOOKUP_HEIGHT);
			serializer.writeAs<uint32>(compressed.size());
			serializer.write(&compressed[0], compressed.size());

			FTX::FileSystem->saveFile(L"data/binary/bluespheresrendering.bin", data);
		}
	}

	// Done
	mInitializedLookups = true;
}
