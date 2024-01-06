/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
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
#include "oxygen/simulation/LogDisplay.h"


SpriteManager::SpriteManager(PatternManager& patternManager, SpacesManager& spacesManager) :
	mPatternManager(patternManager),
	mSpacesManager(spacesManager)
{
	clear();
}

void SpriteManager::clear()
{
	mResetRenderItems = false;
	clearAllContexts();
	clearItemSet(mAddedItems);
}

void SpriteManager::clearLifetimeContext(RenderItem::LifetimeContext lifetimeContext)
{
	clearItemSet(mContexts[(int)lifetimeContext]);
}

void SpriteManager::preFrameUpdate()
{
	mCurrentContext = RenderItem::LifetimeContext::DEFAULT;
	mLogicalSpriteSpace = Space::SCREEN;
	mLoggedLimitWarning = false;
	mSpriteTag = 0;
	mTaggedSpritesLastFrame.swap(mTaggedSpritesThisFrame);
	mTaggedSpritesThisFrame.clear();
}

void SpriteManager::postFrameUpdate()
{
	// Process sprite handles
	processSpriteHandles();

	// Process render items
	{
		if (mResetRenderItems)
		{
			clearAllContexts();
		}

		// Apply added render items
		grabAddedItems();

		clearLifetimeContext(RenderItem::LifetimeContext::OUTSIDE_FRAME);
		mCurrentContext = RenderItem::LifetimeContext::OUTSIDE_FRAME;

		if (mResetRenderItems)
		{
			if (EngineMain::getDelegate().useDeveloperFeatures())
			{
				mLegacyVdpSpriteMode = FTX::keyState('u') && (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT));

				// In legacy mode, we collect current sprites from VRAM, instead of the usual methods via script calls like "Renderer.drawVdpSprite"
				if (mLegacyVdpSpriteMode)
				{
					collectLegacySprites();
				}
			}

			mResetRenderItems = false;
		}
		else
		{
			// When maintaining sprites, make sure to reset sprite interpolation
			for (int contextIndex = 0; contextIndex < RenderItem::NUM_CONTEXTS; ++contextIndex)
			{
				for (RenderItem* renderItem : mContexts[contextIndex].mItems)
				{
					if (renderItem->isSprite())
					{
						renderitems::SpriteInfo& sprite = static_cast<renderitems::SpriteInfo&>(*renderItem);
						sprite.mLastPositionChange.clear();
					}
				}
			}
		}
	}
}

void SpriteManager::postRefreshDebugging()
{
	grabAddedItems();
}

void SpriteManager::drawVdpSprite(const Vec2i& position, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, const Color& tintColor, const Color& addedColor)
{
	if (!checkRenderItemLimit())
		return;

	renderitems::VdpSpriteInfo& sprite = mPoolOfRenderItems.mVdpSprites.createObject();
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
	mAddedItems.mItems.push_back(&sprite);

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

	renderitems::CustomSpriteInfoBase* spritePtr = addSpriteByKey(key);
	if (nullptr == spritePtr)
		return;

	renderitems::CustomSpriteInfoBase& sprite = *spritePtr;
	if (sprite.getType() == RenderItem::Type::PALETTE_SPRITE)
	{
		static_cast<renderitems::PaletteSpriteInfo&>(sprite).mAtex = atex;
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
	if (!checkRenderItemLimit())
		return;

	renderitems::SpriteMaskInfo& sprite = mPoolOfRenderItems.mSpriteMasks.createObject();
	sprite.mPosition = position;
	sprite.mSize = size;
	sprite.mRenderQueue = renderQueue;
	sprite.mDepth = priorityFlag ? 0.6f : 0.1f;
	sprite.mCoordinatesSpace = space;
	sprite.mLogicalSpace = mLogicalSpriteSpace;
	mAddedItems.mItems.push_back(&sprite);
}

void SpriteManager::addRectangle(const Recti& rect, const Color& color, uint16 renderQueue, Space space, bool useGlobalComponentTint)
{
	if (!checkRenderItemLimit())
		return;

	renderitems::Rectangle& newRect = mPoolOfRenderItems.mRectangles.createObject();
	newRect.mPosition = rect.getPos();
	newRect.mSize = rect.getSize();
	newRect.mColor = color;
	newRect.mRenderQueue = renderQueue;
	newRect.mCoordinatesSpace = space;
	newRect.mLifetimeContext = mCurrentContext;
	newRect.mUseGlobalComponentTint = useGlobalComponentTint;
	mAddedItems.mItems.push_back(&newRect);
}

void SpriteManager::addText(std::string_view fontKeyString, uint64 fontKeyHash, const Vec2i& position, std::string_view textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space, bool useGlobalComponentTint)
{
	if (!checkRenderItemLimit())
		return;

	renderitems::Text& newText = mPoolOfRenderItems.mTexts.createObject();
	newText.mFontKeyString = fontKeyString;
	newText.mFontKeyHash = fontKeyHash;
	newText.mPosition = position;
	newText.mTextHash = textHash;
	newText.mTextString = textString;
	newText.mColor = color;
	newText.mAlignment = alignment;
	newText.mSpacing = spacing;
	newText.mRenderQueue = renderQueue;
	newText.mCoordinatesSpace = space;
	newText.mLifetimeContext = mCurrentContext;
	newText.mUseGlobalComponentTint = useGlobalComponentTint;
	mAddedItems.mItems.push_back(&newText);
}

uint32 SpriteManager::addSpriteHandle(uint64 key, const Vec2i& position, uint16 renderQueue)
{
	const uint32 spriteHandle = mNextSpriteHandle;
	++mNextSpriteHandle;
	if (mNextSpriteHandle == 0)
		++mNextSpriteHandle;

	SpriteHandleData& data = vectorAdd(mSpritesHandles);
	data.mHandle = mNextSpriteHandle;
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

	// Otherwise search for the handle
	//  -> TODO: At least for now, the sprite handles are always in order, so we could do a binary search here
	SpriteHandleData* data = nullptr;
	for (auto it = mSpritesHandles.rbegin(); it != mSpritesHandles.rend(); ++it)
	{
		if (it->mHandle == spriteHandle)
		{
			data = &*it;
			break;
		}
	}

	if (nullptr != data)
		mLatestSpriteHandle = std::make_pair(spriteHandle, data);

	return data;
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

void SpriteManager::clearItemSet(ItemSet& itemSet)
{
	for (RenderItem* renderItem : itemSet.mItems)
	{
		mPoolOfRenderItems.destroy(*renderItem);
	}
	itemSet.mItems.clear();
}

void SpriteManager::clearAllContexts()
{
	for (int contextIndex = 0; contextIndex < RenderItem::NUM_CONTEXTS; ++contextIndex)
	{
		clearItemSet(mContexts[contextIndex]);
	}
}

void SpriteManager::serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	if (serializer.isReading())
	{
		clear();
	}

	if (formatVersion >= 4)
	{
		for (int contextIndex = 0; contextIndex < RenderItem::NUM_CONTEXTS; ++contextIndex)
		{
			std::vector<RenderItem*>& renderItems = mContexts[contextIndex].mItems;
			serializer.serializeArraySize(renderItems);
			if (serializer.isReading())
			{
				for (RenderItem*& renderItem : renderItems)
				{
					const RenderItem::Type type = (RenderItem::Type)serializer.read<uint8>();
					renderItem = &mPoolOfRenderItems.create(type);
					renderItem->serialize(serializer, formatVersion);
				}
			}
			else
			{
				for (RenderItem* renderItem : renderItems)
				{
					serializer.writeAs<uint8>(renderItem->getType());
					renderItem->serialize(serializer, formatVersion);
				}
			}
		}
	}
}

SpriteManager::ItemSet& SpriteManager::getItemsByContext(RenderItem::LifetimeContext lifetimeContext)
{
	RMX_ASSERT((int)lifetimeContext < RenderItem::NUM_CONTEXTS, "Invalid lifetime context " << (int)lifetimeContext);
	return mContexts[(int)lifetimeContext];
}

renderitems::CustomSpriteInfoBase* SpriteManager::addSpriteByKey(uint64 key)
{
	const SpriteCache::CacheItem* item = SpriteCache::instance().getSprite(key);
	if (nullptr != item)
	{
		if (checkRenderItemLimit())
		{
			if (item->mUsesComponentSprite)
			{
				renderitems::ComponentSpriteInfo& sprite = mPoolOfRenderItems.mComponentSprites.createObject();
				sprite.mKey = key;
				sprite.mCacheItem = item;
				sprite.mPivotOffset = item->mSprite->mOffset;
				sprite.mSize = static_cast<ComponentSprite*>(item->mSprite)->getBitmap().getSize();
				sprite.mLogicalSpace = mLogicalSpriteSpace;
				mAddedItems.mItems.push_back(&sprite);
				return &sprite;
			}
			else
			{
				renderitems::PaletteSpriteInfo& sprite = mPoolOfRenderItems.mPaletteSprites.createObject();
				sprite.mKey = key;
				sprite.mCacheItem = item;
				sprite.mPivotOffset = item->mSprite->mOffset;
				sprite.mSize = static_cast<PaletteSprite*>(sprite.mCacheItem->mSprite)->getBitmap().getSize();
				sprite.mLogicalSpace = mLogicalSpriteSpace;
				mAddedItems.mItems.push_back(&sprite);
				return &sprite;
			}
		}
	}
	return nullptr;
}

void SpriteManager::checkSpriteTag(renderitems::SpriteInfo& sprite)
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

bool SpriteManager::checkRenderItemLimit()
{
	const constexpr size_t LIMIT = 2048;
	if (mAddedItems.mItems.size() < LIMIT)
	{
		// Everything's okay
		return true;
	}
	else
	{
		// Reached the limit
		if (!mLoggedLimitWarning)
		{
			if (EngineMain::getDelegate().useDeveloperFeatures())
				LogDisplay::instance().setLogDisplay("Warning: Exceeded the upper limit of " + std::to_string(LIMIT) + " items to render, further ones will be ignored");
			mLoggedLimitWarning = true;
		}
		return false;
	}
}

void SpriteManager::processSpriteHandles()
{
	for (const SpriteHandleData& data : mSpritesHandles)
	{
		renderitems::CustomSpriteInfoBase* spritePtr = addSpriteByKey(data.mKey);
		if (nullptr == spritePtr)
			continue;

		renderitems::CustomSpriteInfoBase& sprite = *spritePtr;
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
			static_cast<renderitems::PaletteSpriteInfo&>(sprite).mAtex = data.mAtex;
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
}

void SpriteManager::grabAddedItems()
{
	const Vec2i worldSpaceOffset = mSpacesManager.getWorldSpaceOffset();

	// Add render items from "next" to "current", in reverse order
	for (auto it = mAddedItems.mItems.rbegin(); it != mAddedItems.mItems.rend(); ++it)
	{
		RenderItem* renderItem = *it;

		// Process coordinates if in world space
		if (renderItem->mCoordinatesSpace == SpriteManager::Space::WORLD)
		{
			// Move to screen space
			renderItem->mPosition -= worldSpaceOffset;
			renderItem->mCoordinatesSpace = SpriteManager::Space::SCREEN;
		}

		// Add to the right context
		ItemSet& itemSet = getItemsByContext(renderItem->mLifetimeContext);
		itemSet.mItems.push_back(renderItem);
	}

	mAddedItems.mItems.clear();		// Intentionally not using anything like "clearLifetimeContext" here, as it would invalidate the copied instances
}

void SpriteManager::collectLegacySprites()
{
	// Collect sprites to render
	int numSpritesAdded = 0;
	for (int link = 0;;)
	{
		const uint16* data = (uint16*)&EmulatorInterface::instance().getVRam()[mSpriteAttributeTableBase + link * 2];

		renderitems::VdpSpriteInfo& sprite = mPoolOfRenderItems.mVdpSprites.createObject();
		sprite.mRenderQueue = 0x2100 - numSpritesAdded;
		sprite.mPriorityFlag = (data[2] & 0x8000) != 0;

		// Note that for accurate emulation, this would be 0x1ff instead of 0x3ff
		//  -> But it limits effective maximum sprite x-position to 383 = 0x17f, which is a bit too low for widescreen with e.g. 400px
		//  -> So we're taking an extra bit; looks like it is not used otherwise anyway
		sprite.mPosition.x = (data[3] & 0x3ff) - 0x80;
		sprite.mPosition.y = (data[0] & 0x1ff) - 0x80;

		sprite.mSize.x = 1 + ((data[1] >> 10) & 3);
		sprite.mSize.y = 1 + ((data[1] >> 8) & 3);

		sprite.mFirstPattern = data[2];

		mContexts[(int)RenderItem::LifetimeContext::DEFAULT].mItems.push_back(&sprite);
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
