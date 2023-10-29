/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/rendering/parts/SpriteManager.h"
#include "oxygen/rendering/parts/PatternManager.h"
#include "oxygen/resources/SpriteCache.h"
#include "oxygen/simulation/EmulatorInterface.h"


void SpriteManager::SpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	serializer.serializeAs<int16>(mPosition.x);
	serializer.serializeAs<int16>(mPosition.y);
	serializer.serializeAs<uint8>(mPriorityFlag);
	serializer.serialize(mRenderQueue);
	mTintColor.serialize(serializer);
	mAddedColor.serialize(serializer);
	serializer.serializeAs<uint8>(mUseGlobalComponentTint);
	serializer.serializeAs<uint8>(mBlendMode);
	serializer.serializeAs<uint8>(mCoordinatesSpace);
	serializer.serializeAs<uint8>(mLogicalSpace);
}

void SpriteManager::VdpSpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	SpriteInfo::serialize(serializer, formatVersion);

	serializer.serializeAs<uint8>(mSize.x);
	serializer.serializeAs<uint8>(mSize.y);
	serializer.serialize(mFirstPattern);
}

void SpriteManager::CustomSpriteInfoBase::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	SpriteInfo::serialize(serializer, formatVersion);

	serializer.serialize(mKey);
	if (serializer.isReading())
	{
		mCacheItem = SpriteCache::instance().getSprite(mKey);
	}

	serializer.serializeAs<uint16>(mSize.x);
	serializer.serializeAs<uint16>(mSize.y);
	serializer.serializeAs<int16>(mPivotOffset.x);
	serializer.serializeAs<int16>(mPivotOffset.y);
	serializer.serialize(mTransformation.mMatrix.x);
	serializer.serialize(mTransformation.mMatrix.y);
	serializer.serialize(mTransformation.mMatrix.z);
	serializer.serialize(mTransformation.mMatrix.w);
	serializer.serialize(mTransformation.mInverse.x);
	serializer.serialize(mTransformation.mInverse.y);
	serializer.serialize(mTransformation.mInverse.z);
	serializer.serialize(mTransformation.mInverse.w);
	serializer.serializeAs<uint8>(mUseUpscaledSprite);
}

void SpriteManager::PaletteSpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	CustomSpriteInfoBase::serialize(serializer, formatVersion);

	serializer.serialize(mAtex);
}

void SpriteManager::SpriteMaskInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	serializer.serializeAs<uint16>(mSize.x);
	serializer.serializeAs<uint16>(mSize.y);
	serializer.serialize(mDepth);
}


SpriteManager::SpriteManager(PatternManager& patternManager, SpacesManager& spacesManager) :
	mPatternManager(patternManager),
	mSpacesManager(spacesManager)
{
	clear();
}

void SpriteManager::clear()
{
	mResetSprites = true;

	clearSpriteSet(mCurrSpriteSet);
	clearSpriteSet(mNextSpriteSet);
	mSprites.clear();
}

void SpriteManager::resetSprites()
{
	mResetSprites = true;

	clearSpriteSet(mCurrSpriteSet);
	mSprites.clear();
}

void SpriteManager::preFrameUpdate()
{
	mLogicalSpriteSpace = Space::SCREEN;
	mSpriteTag = 0;
	mTaggedSpritesLastFrame.swap(mTaggedSpritesThisFrame);
	mTaggedSpritesThisFrame.clear();
}

void SpriteManager::postFrameUpdate()
{
	// Process sprite handles
	for (const auto& [key, data] : mSpritesHandles)
	{
		CustomSpriteInfoBase* spritePtr = addSpriteByKey(data.mKey);
		if (nullptr == spritePtr)
			continue;

		CustomSpriteInfoBase& sprite = *spritePtr;
		sprite.mPosition = data.mPosition;
		sprite.mRenderQueue = data.mRenderQueue;
		sprite.mPriorityFlag = data.mPriorityFlag;
		sprite.mTintColor = data.mTintColor;
		sprite.mAddedColor = data.mAddedColor;
		sprite.mUseGlobalComponentTint = data.mUseGlobalComponentTint;
		sprite.mBlendMode = data.mBlendMode;
		sprite.mCoordinatesSpace = data.mCoordinatesSpace;
		sprite.mUseUpscaledSprite = data.mUseUpscaledSprite;

		if (data.mRotation != 0.0f || data.mScale != Vec2f(1.0f, 1.0f))
		{
			sprite.mTransformation.setRotationAndScale(data.mRotation, data.mScale);
		}
		else
		{
			sprite.mTransformation = data.mTransformation;
		}

		if (data.mFlipX)
		{
			sprite.mTransformation.flipX();
		}
		if (data.mFlipY)
		{
			sprite.mTransformation.flipY();
		}

		if (sprite.getType() == RenderItem::Type::PALETTE_SPRITE)
		{
			static_cast<PaletteSpriteInfo&>(sprite).mAtex = data.mAtex;
		}

		if (data.mSpriteTag != 0)
		{
			const auto it = mTaggedSpritesLastFrame.find(data.mSpriteTag);
			if (it != mTaggedSpritesLastFrame.end())
			{
				sprite.mHasLastPosition = true;
				sprite.mLastPositionChange = data.mTaggedSpritePosition - it->second.mPosition;
			}

			TaggedSpriteData& taggedSpriteData = mTaggedSpritesThisFrame[data.mSpriteTag];
			taggedSpriteData.mPosition = data.mTaggedSpritePosition;
		}
	}
	mSpritesHandles.clear();

	if (mResetSprites)
	{
		// Replace render items in "current" with "next"
		mCurrSpriteSet.mItems.swap(mNextSpriteSet.mItems);
		clearSpriteSet(mNextSpriteSet);

		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			mLegacyVdpSpriteMode = FTX::keyState('u') && (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT));

			// In legacy mode, we collect current sprites from VRAM, instead of the usual methods via script calls like "Renderer.drawVdpSprite"
			if (mLegacyVdpSpriteMode)
			{
				collectLegacySprites();
			}
		}

		buildSortedSprites();

		// Process coordinates of all sprites
		const Vec2i worldSpaceOffset = mSpacesManager.getWorldSpaceOffset();
		for (RenderItem* renderItem : mSprites)
		{
			RMX_ASSERT(renderItem->isSprite(), "Expected a sprite");
			SpriteManager::SpriteInfo& sprite = static_cast<SpriteManager::SpriteInfo&>(*renderItem);
			if (sprite.mCoordinatesSpace == SpriteManager::Space::WORLD)
			{
				sprite.mPosition -= worldSpaceOffset;
			}
		}

		mResetSprites = false;
	}
	else
	{
		if (!mNextSpriteSet.mItems.empty())
		{
			// Add render items from "next" to "current"
			for (RenderItem* renderItem : mNextSpriteSet.mItems)
			{
				mCurrSpriteSet.mItems.push_back(renderItem);
			}
			mNextSpriteSet.mItems.clear();	// Intentionally not using "clearSpriteSet" here, as it would invalidate the copied instances

			buildSortedSprites();
		}

		// When maintaining sprites, make sure to reset sprite interpolation
		for (RenderItem* renderItem : mSprites)
		{
			RMX_ASSERT(renderItem->isSprite(), "Expected a sprite");
			SpriteManager::SpriteInfo& sprite = static_cast<SpriteManager::SpriteInfo&>(*renderItem);
			sprite.mLastPositionChange.clear();
		}
	}
}

void SpriteManager::drawVdpSprite(const Vec2i& position, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, const Color& tintColor, const Color& addedColor)
{
	if (mNextSpriteSet.mItems.size() >= 0x800)
	{
		RMX_ERROR("Reached the upper limit of " << mNextSpriteSet.mItems.size() << " sprites, further sprites will be ignored", );
		return;
	}

	VdpSpriteInfo& sprite = mPoolOfVdpSprites.createObject();
	sprite.mPosition = position;
	sprite.mSize.x = 1 + ((encodedSize >> 2) & 3);
	sprite.mSize.y = 1 + (encodedSize & 3);
	sprite.mFirstPattern = patternIndex;
	sprite.mRenderQueue = renderQueue;
	sprite.mPriorityFlag = (patternIndex & 0x8000) != 0;
	sprite.mTintColor = tintColor;
	sprite.mAddedColor = addedColor;
	sprite.mCoordinatesSpace = Space::SCREEN;
	sprite.mLogicalSpace = mLogicalSpriteSpace;
	mNextSpriteSet.mItems.push_back(&sprite);

	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		// Update "last used atex" of patterns in cache (actually that's only needed for debug output)
		const uint16 patterns = (uint16)(sprite.mSize.x * sprite.mSize.y);
		for (uint16 k = 0; k < patterns; ++k)
		{
			mPatternManager.setLastUsedAtex(sprite.mFirstPattern + k, (sprite.mFirstPattern >> 9) & 0x70);
		}
	}

	checkSpriteTag(sprite);
}

void SpriteManager::drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, float scale)
{
	// Rotation
	Transform2D transformation;
	if (angle != 0.0f)
	{
		transformation.setRotationByAngle(angle);
	}

	// Scale
	if (scale != 1.0f)
	{
		transformation.applyScale(scale);
	}

	drawCustomSpriteWithTransform(key, position, atex, flags, renderQueue, tintColor, transformation);
}

void SpriteManager::drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, Vec2f scale)
{
	Transform2D transformation;
	transformation.setRotationAndScale(angle, scale);
	drawCustomSpriteWithTransform(key, position, atex, flags, renderQueue, tintColor, transformation);
}

void SpriteManager::drawCustomSpriteWithTransform(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, const Transform2D& transformation)
{
	// Flags:
	//  - 0x01 = Flip X
	//  - 0x02 = Flip Y
	//  - 0x08 = Pixel upscaling
	//  - 0x10 = Fully opaque
	//  - 0x20 = World space
	//  - 0x40 = Priority flag
	//  - 0x80 = Use global tint

	CustomSpriteInfoBase* spritePtr = addSpriteByKey(key);
	if (nullptr == spritePtr)
		return;

	CustomSpriteInfoBase& sprite = *spritePtr;
	if (sprite.getType() == RenderItem::Type::PALETTE_SPRITE)
	{
		static_cast<PaletteSpriteInfo&>(sprite).mAtex = atex;
	}

	sprite.mPosition = position;
	sprite.mRenderQueue = renderQueue;
	sprite.mTransformation = transformation;
	sprite.mTintColor = tintColor;

	sprite.mPriorityFlag = (flags & 0x40) != 0;
	sprite.mBlendMode = (flags & 0x10) ? BlendMode::OPAQUE : BlendMode::ALPHA;
	sprite.mCoordinatesSpace = ((flags & 0x20) != 0) ? Space::WORLD : Space::SCREEN;
	sprite.mUseGlobalComponentTint = (flags & 0x80) == 0;

	// Flip X / Y
	const bool flipX = (flags & 0x01) != 0;
	const bool flipY = (flags & 0x02) != 0;
	if (flipX)
	{
		sprite.mTransformation.flipX();
	}
	if (flipY)
	{
		sprite.mTransformation.flipY();
	}

	// For smoother rotation, use upscaled version of this sprite when actually rotating
	//  -> This is basically the Fast SpriteRot algorithm (also see "applyScale3x" in PaletteSprite.cpp)
	//  -> Currently only implemented for palette sprites, but could be added for component sprites as well if needed
	if (!sprite.mCacheItem->mUsesComponentSprite && sprite.mTransformation.hasNontrivialRotationOrScale() && (flags & 0x08) == 0)
	{
		sprite.mUseUpscaledSprite = true;

		const constexpr float SCALE = 1.0f / 3.0f;
		const Vec2f transformedOffset = sprite.mTransformation.transformVector(sprite.mCacheItem->mSprite->mOffset);
		sprite.mPosition.x += roundToInt(transformedOffset.x * (1.0f - SCALE));
		sprite.mPosition.y += roundToInt(transformedOffset.y * (1.0f - SCALE));
		sprite.mSize *= 3;
		sprite.mTransformation.applyScale(SCALE);
	}

	checkSpriteTag(sprite);
}

void SpriteManager::addSpriteMask(const Vec2i& position, const Vec2i& size, uint16 renderQueue, bool priorityFlag, Space space)
{
	// TODO: Originally, the number of sprite masks was 0x40
	if (mNextSpriteSet.mItems.size() >= 0x800)
	{
		RMX_ERROR("Reached the upper limit of " << mNextSpriteSet.mItems.size() << " sprites, further sprites will be ignored", );
		return;
	}

	SpriteMaskInfo& sprite = mPoolOfSpriteMasks.createObject();
	sprite.mPosition = position;
	sprite.mSize = size;
	sprite.mRenderQueue = renderQueue;
	sprite.mDepth = priorityFlag ? 0.6f : 0.1f;
	sprite.mCoordinatesSpace = space;
	sprite.mLogicalSpace = mLogicalSpriteSpace;
	mNextSpriteSet.mItems.push_back(&sprite);
}

uint32 SpriteManager::addSpriteHandle(uint64 key, const Vec2i& position, uint16 renderQueue)
{
	const uint32 spriteHandle = mNextSpriteHandle;
	++mNextSpriteHandle;
	if (mNextSpriteHandle == 0)
		++mNextSpriteHandle;

	SpriteHandleData& data = mSpritesHandles[spriteHandle];
	data.mKey = key;
	data.mPosition = position;
	data.mRenderQueue = renderQueue;
	data.mSpriteTag = mSpriteTag;

	mLatestSpriteHandle = std::make_pair(spriteHandle, &data);
	return spriteHandle;
}

SpriteManager::SpriteHandleData* SpriteManager::getSpriteHandleData(uint32 spriteHandle)
{
	// Use the quick lookup if possible
	if (mLatestSpriteHandle.first == spriteHandle)
		return mLatestSpriteHandle.second;

	// Otherwise search the map
	const auto it = mSpritesHandles.find(spriteHandle);
	if (it == mSpritesHandles.end())
		return nullptr;

	SpriteHandleData& data = it->second;
	mLatestSpriteHandle = std::make_pair(spriteHandle, &data);
	return &data;
}

void SpriteManager::setLogicalSpriteSpace(Space space)
{
	mLogicalSpriteSpace = space;
}

void SpriteManager::clearSpriteTag()
{
	mSpriteTag = 0;
}

void SpriteManager::setSpriteTagWithPosition(uint64 spriteTag, const Vec2i& position)
{
	mSpriteTag = spriteTag;
	mTaggedSpritePosition = position;
}

void SpriteManager::clearSpriteSet(SpriteSet& spriteSet)
{
	for (RenderItem* renderItem : spriteSet.mItems)
		destroyRenderItem(*renderItem);
	spriteSet.mItems.clear();
}

void SpriteManager::serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	if (formatVersion >= 4)
	{
		serializer.serializeArraySize(mCurrSpriteSet.mItems);
		if (serializer.isReading())
		{
			for (RenderItem*& renderItem : mCurrSpriteSet.mItems)
			{
				const RenderItem::Type type = (RenderItem::Type)serializer.read<uint8>();
				switch (type)
				{
					case RenderItem::Type::VDP_SPRITE:		 renderItem = &mPoolOfVdpSprites.createObject();  break;
					case RenderItem::Type::PALETTE_SPRITE:	 renderItem = &mPoolOfPaletteSprites.createObject();  break;
					case RenderItem::Type::COMPONENT_SPRITE: renderItem = &mPoolOfComponentSprites.createObject();  break;
					case RenderItem::Type::SPRITE_MASK:		 renderItem = &mPoolOfSpriteMasks.createObject();  break;
					default:  return;
				}
				renderItem->serialize(serializer, formatVersion);
			}
		}
		else
		{
			for (RenderItem* renderItem : mCurrSpriteSet.mItems)
			{
				serializer.writeAs<uint8>(renderItem->getType());
				renderItem->serialize(serializer, formatVersion);
			}
		}

		if (serializer.isReading())
		{
			buildSortedSprites();
		}
	}
}

void SpriteManager::destroyRenderItem(RenderItem& renderItem)
{
	switch (renderItem.getType())
	{
		case RenderItem::Type::VDP_SPRITE:		 mPoolOfVdpSprites.destroyObject(static_cast<VdpSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::PALETTE_SPRITE:	 mPoolOfPaletteSprites.destroyObject(static_cast<PaletteSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::COMPONENT_SPRITE: mPoolOfComponentSprites.destroyObject(static_cast<ComponentSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::SPRITE_MASK:		 mPoolOfSpriteMasks.destroyObject(static_cast<SpriteMaskInfo&>(renderItem));  break;
		default:
			RMX_ASSERT(false, "Trying to destroy unsupported render item type");
			break;
	}
}

SpriteManager::CustomSpriteInfoBase* SpriteManager::addSpriteByKey(uint64 key)
{
	const SpriteCache::CacheItem* item = SpriteCache::instance().getSprite(key);
	if (nullptr != item)
	{
		if (mNextSpriteSet.mItems.size() < 0x800)
		{
			if (item->mUsesComponentSprite)
			{
				ComponentSpriteInfo& sprite = mPoolOfComponentSprites.createObject();
				sprite.mKey = key;
				sprite.mCacheItem = item;
				sprite.mPivotOffset = item->mSprite->mOffset;
				sprite.mSize = static_cast<ComponentSprite*>(item->mSprite)->getBitmap().getSize();
				sprite.mLogicalSpace = mLogicalSpriteSpace;
				mNextSpriteSet.mItems.push_back(&sprite);
				return &sprite;
			}
			else
			{
				PaletteSpriteInfo& sprite = mPoolOfPaletteSprites.createObject();
				sprite.mKey = key;
				sprite.mCacheItem = item;
				sprite.mPivotOffset = item->mSprite->mOffset;
				sprite.mSize = static_cast<PaletteSprite*>(sprite.mCacheItem->mSprite)->getBitmap().getSize();
				sprite.mLogicalSpace = mLogicalSpriteSpace;
				mNextSpriteSet.mItems.push_back(&sprite);
				return &sprite;
			}
		}
		RMX_ERROR("Reached the upper limit of " << mNextSpriteSet.mItems.size() << " sprites, further sprites will be ignored", );
	}
	return nullptr;
}

void SpriteManager::checkSpriteTag(SpriteInfo& sprite)
{
	if (mSpriteTag != 0)
	{
		const auto it = mTaggedSpritesLastFrame.find(mSpriteTag);
		if (it != mTaggedSpritesLastFrame.end())
		{
			sprite.mHasLastPosition = true;
			sprite.mLastPositionChange = mTaggedSpritePosition - it->second.mPosition;
		}

		TaggedSpriteData& taggedSpriteData = mTaggedSpritesThisFrame[mSpriteTag];
		taggedSpriteData.mPosition = mTaggedSpritePosition;
	}
}

void SpriteManager::collectLegacySprites()
{
	// Collect sprites to render
	int numSpritesAdded = 0;
	for (int link = 0;;)
	{
		const uint16* data = (uint16*)&EmulatorInterface::instance().getVRam()[mSpriteAttributeTableBase + link * 2];

		VdpSpriteInfo& sprite = mPoolOfVdpSprites.createObject();
		sprite.mRenderQueue = 0x2000;
		sprite.mPriorityFlag = (data[2] & 0x8000) != 0;

		// Note that for accurate emulation, this would be 0x1ff instead of 0x3ff
		//  -> But it limits effective maximum sprite x-position to 383 = 0x17f, which is a bit too low for widescreen with e.g. 400px
		//  -> So we're taking an extra bit; looks like it is not used otherwise anyway
		sprite.mPosition.x = (data[3] & 0x3ff) - 0x80;
		sprite.mPosition.y = (data[0] & 0x1ff) - 0x80;

		sprite.mSize.x = 1 + ((data[1] >> 10) & 3);
		sprite.mSize.y = 1 + ((data[1] >> 8) & 3);

		sprite.mFirstPattern = data[2];

		mCurrSpriteSet.mItems.push_back(&sprite);
		++numSpritesAdded;

		// Update "last used atex" of patterns in cache (actually that's only needed for debug output)
		{
			const uint16 patterns = (uint16)(sprite.mSize.x * sprite.mSize.y);
			for (uint16 k = 0; k < patterns; ++k)
			{
				mPatternManager.setLastUsedAtex(data[2] + k, (data[2] >> 9) & 0x70);
			}
		}

		// Go to next sprite
		link = (data[1] & 0x7f) << 2;
		if ((link == 0) || (link >= 320))
			break;
		if (numSpritesAdded >= 0x100)	// Sanity check
			break;
	}
}

void SpriteManager::buildSortedSprites()
{
	// Mix all types of sprites together in one sorted vector
	mSprites = mCurrSpriteSet.mItems;

	// Sorting only cares about render queue, not priority!
	//  -> Because that's how the VDP works as well
	std::stable_sort(mSprites.begin(), mSprites.end(),
		[](const RenderItem* a, const RenderItem* b) { return a->mRenderQueue > b->mRenderQueue; });

	// Reverse order, so that the sprites "in front" are rendered last
	//  -> Note that it makes a difference if this was removed and the sorting order changed, namely for sprites sharing the same render queue
	std::reverse(mSprites.begin(), mSprites.end());
}
