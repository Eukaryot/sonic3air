/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/BlueSpheresRendering.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Configuration.h"
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

void BlueSpheresRendering::renderToBitmap(Bitmap& bitmapOpaque, Bitmap& bitmapAlpha, int screenWidth, uint16 px, uint16 py, uint8 rotation, uint16 fieldColorA, uint16 fieldColorB)
{
#if 0
	// This is meant for fine-tuning the camera parameters
	if (FTX::keyState(SDLK_DOWN))
		CAMERA_POSITION_HEIGHT -= FTX::getTimeDifference() * 0.03f;
	if (FTX::keyState(SDLK_UP))
		CAMERA_POSITION_HEIGHT += FTX::getTimeDifference() * 0.03f;

	if (FTX::keyState(SDLK_LEFT))
		CAMERA_POSITION_BACKANGLE -= FTX::getTimeDifference() * 5.0f;
	if (FTX::keyState(SDLK_RIGHT))
		CAMERA_POSITION_BACKANGLE += FTX::getTimeDifference() * 5.0f;

	if (FTX::keyState('w'))
		CAMERA_TARGET_ANGLE -= FTX::getTimeDifference() * 2.0f;
	if (FTX::keyState('s'))
		CAMERA_TARGET_ANGLE += FTX::getTimeDifference() * 2.0f;

	if (FTX::keyState('a'))
		PLANE_DISTANCE -= FTX::getTimeDifference() * 30.0f;
	if (FTX::keyState('d'))
		PLANE_DISTANCE += FTX::getTimeDifference() * 30.0f;

	if (FTX::keyState('q'))
		GRID_SIZE -= FTX::getTimeDifference() * 0.8f;
	if (FTX::keyState('e'))
		GRID_SIZE += FTX::getTimeDifference() * 0.8f;
#endif

	// Perform calculations that only need to be done once
	if (!mInitializedLookups)
	{
		performLookupCalculations();
	}

	const int maxX = std::min(LOOKUP_WIDTH, screenWidth);
	const int maxY = LOOKUP_HEIGHT;
	const int indentX = (screenWidth - maxX) / 2;
	const int offsetX = (LOOKUP_WIDTH - maxX) / 2;

	if (mPureRowsForWidth != screenWidth)
	{
		mNumPureSkyRows = 0;
		for (int row = 0; ; ++row)
		{
			const uint8* lookup = &mVisibilityLookup[row * LOOKUP_WIDTH + offsetX];
			const uint8* end = &mVisibilityLookup[(row + 1) * LOOKUP_WIDTH - offsetX - 1];
			for (; lookup <= end; ++lookup)
			{
				if (*lookup != 0)
					break;
			}
			if (lookup <= end)
				break;
			++mNumPureSkyRows;
		}

		mNumPureGroundRows = 0;
		for (int row = LOOKUP_HEIGHT - 1; ; --row)
		{
			const uint8* lookup = &mVisibilityLookup[row * LOOKUP_WIDTH + offsetX];
			const uint8* end = &mVisibilityLookup[(row + 1) * LOOKUP_WIDTH - offsetX - 1];
			for (; lookup <= end; ++lookup)
			{
				if (*lookup != 0xff)
					break;
			}
			if (lookup <= end)
				break;
			++mNumPureGroundRows;
		}

		mPureRowsForWidth = screenWidth;
	}

	// Refresh mixed field color lookup
	{
		const bool useFiltering = (SharedDatabase::getSettingValue(SharedDatabase::Setting::SETTING_BS_VISUAL_STYLE) & 0x01) != 0;
		if (fieldColorA != mLastFieldColorA || fieldColorB != mLastFieldColorB || useFiltering != mLastFiltering)
		{
			const Color colorA = colorFromCompact(fieldColorA);
			const Color colorB = colorFromCompact(fieldColorB);
			for (int i = 0; i < 0x100; ++i)
			{
				const float intensity = useFiltering ? ((float)i / 255.0f) : (i >= 128 ? 1.0f : 0.0f);
				const uint32 color = (Color::interpolateColor(colorA, colorB, intensity).getABGR32() & 0x00ffffff) | 0xff000000;
				mMixedFieldColorLookup[i] = color;
				mMixedFieldColorLookupInverse[0xff - i] = color;
			}

			mLastFieldColorA = fieldColorA;
			mLastFieldColorB = fieldColorB;
			mLastFiltering = useFiltering;
		}
	}

	// Output the bitmaps
	{
		int movementStep = 0;
		int rotationStep = 0;
		bool parity = false;
		{
			const bool isRotating = (rotation & 0x3f) != 0;
			if (isRotating || (rotation & 0x40) == 0)
				px = (px + 0x80) & 0xff00;
			if (isRotating || (rotation & 0x40) != 0)
				py = (py + 0x80) & 0xff00;

			parity = (((px + py) & 0x100) != 0) == ((rotation & 0x40) != 0);

			if (isRotating)
			{
				rotationStep = (rotation & 0x3f) / 4;
			}
			else
			{
				if ((rotation & 0x80) == 0)
				{
					movementStep = (0xff - ((rotation & 0x40) ? px : py) & 0xff) / 8;
					parity = !parity;
				}
				else
				{
					movementStep = (((rotation & 0x40) ? px : py) & 0xff) / 8;
				}
			}
		}

		const uint32* mixedFieldColorLookup = parity ? mMixedFieldColorLookupInverse : mMixedFieldColorLookup;
		const uint8* lookupDataBase;
		if (rotationStep != 0)
		{
			lookupDataBase = &mRotationIntensityLookup[rotationStep - 1][0];
		}
		else
		{
			lookupDataBase = &mStraightIntensityLookup[movementStep][0];
		}

		// Ignore the first that are just sky, so completely transparent

		// Then we have some rows of pixels that can be anything -- sky or ground or something in between
		const int numRowsUntilPureGround = LOOKUP_HEIGHT - mNumPureGroundRows;
		bitmapAlpha.create(maxX, numRowsUntilPureGround - mNumPureSkyRows);
		for (int y = mNumPureSkyRows; y < numRowsUntilPureGround; ++y)
		{
			uint32* output = &bitmapAlpha.mData[(y - mNumPureSkyRows) * bitmapAlpha.mWidth + indentX];
			const uint32* outputEnd = output + maxX;
			const uint8* lookupData = &lookupDataBase[y * LOOKUP_WIDTH + offsetX];
			const uint8* visibilityData = &mVisibilityLookup[y * LOOKUP_WIDTH + offsetX];

			for (; output != outputEnd; ++output, ++lookupData, ++visibilityData)
			{
				*output = (mixedFieldColorLookup[*lookupData] & 0x00ffffff) | ((uint32)*visibilityData << 24);
			}
		}

		// And the rest is only ground
		bitmapOpaque.create(maxX, mNumPureGroundRows);
		for (int y = numRowsUntilPureGround; y < maxY; ++y)
		{
			uint32* output = &bitmapOpaque.mData[(y - numRowsUntilPureGround) * bitmapOpaque.mWidth + indentX];
			const uint32* outputEnd = output + maxX;
			const uint8* lookupData = &lookupDataBase[y * LOOKUP_WIDTH + offsetX];

			for (; output != outputEnd; ++output, ++lookupData)
			{
				*output = mixedFieldColorLookup[*lookupData];
			}
		}
	}
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
	if (!FTX::FileSystem->readFile(L"data/cache/bluespheresrendering.bin", data))
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

		mVisibilityLookup.resize(pixels);
		serializer.read(&mVisibilityLookup[0], mVisibilityLookup.size());

		// Read left halfs
		for (int i = 0; i < 0x2f; ++i)
		{
			std::vector<uint8>& lookup = (i < 0x20) ? mStraightIntensityLookup[i] : mRotationIntensityLookup[i - 0x20];
			lookup.resize(pixels);
			for (int y = 0; y < height; ++y)
			{
				uint8* lookupData = &lookup[y * width];
				serializer.read(lookupData, width / 2);
			}
		}

		// Reconstruct the right halfs, which are just mirror images of the left half of either:
		//  - the same lookup, but inverted (for straight), or
		//  - another angle's lookups (for rotation)
		for (int i = 0; i < 0x20; ++i)
		{
			for (int y = 0; y < height; ++y)
			{
				uint8* lookupData = &mStraightIntensityLookup[i][y * width];
				for (int x = 0; x < width / 2; ++x)
				{
					lookupData[width - x - 1] = 255 - lookupData[x];
				}
			}
		}
		for (int i = 0; i < 0x0f; ++i)
		{
			for (int y = 0; y < height; ++y)
			{
				uint8* lookupDataDst = &mRotationIntensityLookup[0x0e - i][y * width];
				uint8* lookupDataSrc = &mRotationIntensityLookup[i][y * width];
				for (int x = 0; x < width / 2; ++x)
				{
					lookupDataDst[width - x - 1] = lookupDataSrc[x];
				}
			}
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
		bool mHitsGround;
		float mFogAlpha;
		Vec3f mHitPosition;
		Vec3f mHitTangentX;
		Vec3f mHitTangentY;
	};
	std::vector<CachedPixelData> cachedPixelData;
	cachedPixelData.resize(pixels);
	mVisibilityLookup.resize(pixels);

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
				fogForegroundBitmap.mData[pixelIndex] = 0xffffff + (roundToInt(alpha * groundVisibility * 192.0f) << 24);
			#endif
			}

			if (groundVisibility < 1.0f)
			{
				// Sky is visible (at least partially)
				const float alpha = saturate(1.0f - horizonDistance * 50.0f) * 0.4f;
				pixelData.mFogAlpha = interpolate(alpha, pixelData.mFogAlpha, groundVisibility);

			#ifdef OUTPUT_FOG_BITMAPS
				fogBackgroundBitmap.mData[pixelIndex] = 0xffffff + (roundToInt(alpha * 255.0f) << 24);
			#endif
			}

			mVisibilityLookup[pixelIndex] = roundToInt(groundVisibility * 255.0f);
		}

	#ifdef OUTPUT_FOG_BITMAPS
		fogForegroundBitmap.save(L"foreground.png");
		fogBackgroundBitmap.save(L"background.png");
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

					lookupData[pixelIndex] = (int)(intensity * 255.5f);
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

					lookupData[pixelIndex] = (int)(intensity * 255.5f);
				}
			}
		}
	}

	// Save the results to a cache file
	{
		std::vector<uint8> data;
		{
			VectorBinarySerializer serializer(false, data);

			serializer.write(&mVisibilityLookup[0], mVisibilityLookup.size());

			// Save only the left half of each frame, the right half can be reconstructed in all cases
			for (int i = 0; i < 0x20; ++i)
			{
				for (int y = 0; y < height; ++y)
				{
					serializer.write(&mStraightIntensityLookup[i][y * width], width / 2);
				}
			}
			for (int i = 0; i < 0x0f; ++i)
			{
				for (int y = 0; y < height; ++y)
				{
					serializer.write(&mRotationIntensityLookup[i][y * width], width / 2);
				}
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

			FTX::FileSystem->saveFile(L"data/cache/bluespheresrendering.bin", data);
		}
	}

	// Done
	mInitializedLookups = true;
}
