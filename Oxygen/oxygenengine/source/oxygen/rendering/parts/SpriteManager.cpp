/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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


void SpriteManager::SpriteSets::clear()
{
	mVdpSprites.clear();
	mPaletteSprites.clear();
	mComponentSprites.clear();
	mSpriteMasks.clear();
}

void SpriteManager::SpriteSets::swap(SpriteSets& other)
{
	other.mVdpSprites.swap(mVdpSprites);
	other.mPaletteSprites.swap(mPaletteSprites);
	other.mComponentSprites.swap(mComponentSprites);
	other.mSpriteMasks.swap(mSpriteMasks);
}


SpriteManager::SpriteManager(PatternManager& patternManager, SpacesManager& spacesManager) :
	mPatternManager(patternManager),
	mSpacesManager(spacesManager)
{
	reset();
}

void SpriteManager::reset()
{
	mResetSprites = true;

	mCurrSpriteSets.clear();
	mNextSpriteSets.clear();
	mSprites.clear();
}

void SpriteManager::resetSprites()
{
	mResetSprites = true;

	mCurrSpriteSets.clear();
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
	if (mResetSprites)
	{
		// Move over from "next" to "current" sprite sets
		mCurrSpriteSets.swap(mNextSpriteSets);
		mNextSpriteSets.clear();

		mResetSprites = false;

		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
			mLegacyVdpSpriteMode = FTX::keyState('u') && (FTX::keyState(SDLK_LALT) || FTX::keyState(SDLK_RALT));

			// In legacy mode, we collect current sprites from VRAM, instead of the usual methods via script calls like "Renderer.drawVdpSprite"
			if (mLegacyVdpSpriteMode)
			{
				collectLegacySprites();
			}
		}

		// Mix all types of sprites together in one sorted vector
		mSprites.clear();
		for (VdpSpriteInfo& sprite : mCurrSpriteSets.mVdpSprites)
		{
			mSprites.push_back(&sprite);
		}
		for (PaletteSpriteInfo& sprite : mCurrSpriteSets.mPaletteSprites)
		{
			mSprites.push_back(&sprite);
		}
		for (ComponentSpriteInfo& sprite : mCurrSpriteSets.mComponentSprites)
		{
			mSprites.push_back(&sprite);
		}
		for (SpriteMaskInfo& sprite : mCurrSpriteSets.mSpriteMasks)
		{
			mSprites.push_back(&sprite);
		}

		// Sorting only cares about render queue, not priority!
		//  -> Because that's how the VDP works as well
		std::stable_sort(mSprites.begin(), mSprites.end(),
						 [](const SpriteInfo* a, const SpriteInfo* b) { return a->mRenderQueue > b->mRenderQueue; });

		// Reverse order, so that the sprites "in front" are rendered last
		//  -> Note that it makes a difference if this was removed and the sorting order changed, namely for sprites sharing the same render queue
		std::reverse(mSprites.begin(), mSprites.end());

		// Process coordinates of all sprites
		const Vec2i worldSpaceOffset = mSpacesManager.getWorldSpaceOffset();
		for (SpriteInfo* sprite : mSprites)
		{
			if (sprite->mCoordinatesSpace == SpriteManager::Space::WORLD)
			{
				sprite->mPosition -= worldSpaceOffset;
			}
		}
	}
	else
	{
		// When maintaining sprites, make sure to reset sprite interpolation
		for (SpriteInfo* sprite : mSprites)
		{
			sprite->mLastPositionChange.clear();
		}
	}
}

void SpriteManager::refresh()
{
}

void SpriteManager::drawVdpSprite(const Vec2i& position, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, const Color& tintColor, const Color& addedColor)
{
	if (mNextSpriteSets.mVdpSprites.size() >= 0x400)
	{
		RMX_ERROR("Reached the upper limit of " << mNextSpriteSets.mVdpSprites.size() << " VDP sprites, further sprites will be ignored", );
		return;
	}

	VdpSpriteInfo& sprite = vectorAdd(mNextSpriteSets.mVdpSprites);
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

	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		// Update "last used atex" of patterns in cache (actually that's only needed for debug output)
		const int patterns = sprite.mSize.x * sprite.mSize.y;
		for (int k = 0; k < patterns; ++k)
		{
			mPatternManager.setLastUsedAtex(sprite.mFirstPattern + k, (sprite.mFirstPattern >> 9) & 0x70);
		}
	}

	checkSpriteTag(sprite);
}

void SpriteManager::drawCustomSprite(uint64 key, const Vec2i& position, uint8 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, float scale)
{
	// Rotation
	Transform2D transformation;
	if (angle != 0.0f)
	{
		transformation.setRotationByAngle(angle);
	}
	else
	{
		transformation.setIdentity();
	}

	// Scale
	if (scale != 1.0f)
	{
		transformation.applyScale(scale);
	}

	drawCustomSpriteWithTransform(key, position, atex, flags, renderQueue, tintColor, transformation);
}

void SpriteManager::drawCustomSprite(uint64 key, const Vec2i& position, uint8 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, Vec2f scale)
{
	Transform2D transformation;
	transformation.setRotationAndScale(angle, scale);
	drawCustomSpriteWithTransform(key, position, atex, flags, renderQueue, tintColor, transformation);
}

void SpriteManager::drawCustomSpriteWithTransform(uint64 key, const Vec2i& position, uint8 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, const Transform2D& transformation)
{
	// Flags:
	//  - 0x01 = Flip X
	//  - 0x02 = Flip Y
	//  - 0x08 = Pixel upscaling
	//  - 0x10 = Fully opaque	-- TODO: This is only supported by software renderer
	//  - 0x20 = World space
	//  - 0x40 = Priority flag
	//  - 0x80 = Use global tint

	const SpriteCache::CacheItem* item = SpriteCache::instance().getSprite(key);
	if (nullptr == item)
		return;

	CustomSpriteInfoBase* spritePtr;
	if (item->mUsesComponentSprite)
	{
		if (mNextSpriteSets.mComponentSprites.size() >= 0x400)
		{
			RMX_ERROR("Reached the upper limit of " << mNextSpriteSets.mComponentSprites.size() << " component sprites, further sprites will be ignored", );
			return;
		}

		spritePtr = &vectorAdd(mNextSpriteSets.mComponentSprites);
	}
	else
	{
		if (mNextSpriteSets.mPaletteSprites.size() >= 0x400)
		{
			RMX_ERROR("Reached the upper limit of " << mNextSpriteSets.mPaletteSprites.size() << " palette sprites, further sprites will be ignored", );
			return;
		}

		PaletteSpriteInfo& info = vectorAdd(mNextSpriteSets.mPaletteSprites);
		info.mAtex = atex;
		spritePtr = &info;
	}
	CustomSpriteInfoBase& sprite = *spritePtr;

	sprite.mKey = key;
	sprite.mCacheItem = item;
	sprite.mRenderQueue = renderQueue;
	sprite.mPriorityFlag = (flags & 0x40) != 0;
	sprite.mPosition = position;
	sprite.mPivotOffset = item->mSprite->mOffset;
	sprite.mFullyOpaque = (flags & 0x10) != 0;
	sprite.mCoordinatesSpace = ((flags & 0x20) != 0) ? Space::WORLD : Space::SCREEN;
	sprite.mLogicalSpace = mLogicalSpriteSpace;
	sprite.mTransformation = transformation;

	if (item->mUsesComponentSprite)
	{
		ComponentSprite& sourceSprite = *static_cast<ComponentSprite*>(item->mSprite);
		sprite.mSize = sourceSprite.getBitmap().getSize();
	}
	else
	{
		PaletteSprite& sourceSprite = *static_cast<PaletteSprite*>(item->mSprite);
		sprite.mSize = sourceSprite.getBitmap().getSize();
	}

	// Flip X / Y
	if ((flags & 0x01) != 0)
	{
		sprite.mTransformation.flipX();
	}
	if ((flags & 0x02) != 0)
	{
		sprite.mTransformation.flipY();
	}

	// For smoother rotation, use upscaled version of this sprite when actually rotating
	//  -> This is basically the Fast SpriteRot algorithm (also see "applyScale3x" in PaletteSprite.cpp)
	//  -> Currently only implemented for palette sprites, but could be added for component sprites as well if needed
	if (!item->mUsesComponentSprite && sprite.mTransformation.hasRotationOrScale() && (flags & 0x08) == 0)
	{
		sprite.mUseUpscaledSprite = true;

		const constexpr float SCALE = 1.0f / 3.0f;
		const Vec2f transformedOffset = sprite.mTransformation.transformVector(item->mSprite->mOffset);
		sprite.mPosition.x += roundToInt(transformedOffset.x * (1.0f - SCALE));
		sprite.mPosition.y += roundToInt(transformedOffset.y * (1.0f - SCALE));
		sprite.mSize *= 3;
		sprite.mTransformation.applyScale(SCALE);
	}

	sprite.mTintColor = tintColor;
	//sprite.mAddedColor = addedColor;
	sprite.mUseGlobalComponentTint = (flags & 0x80) == 0;

	checkSpriteTag(sprite);
}

void SpriteManager::addSpriteMask(const Vec2i& position, const Vec2i& size, uint16 renderQueue, bool priorityFlag, Space space)
{
	if (mNextSpriteSets.mSpriteMasks.size() >= 0x40)
	{
		RMX_ERROR("Reached the upper limit of " << mNextSpriteSets.mSpriteMasks.size() << " sprite masks, further sprites will be ignored", );
		return;
	}

	SpriteMaskInfo& sprite = vectorAdd(mNextSpriteSets.mSpriteMasks);
	sprite.mPosition = position;
	sprite.mSize = size;
	sprite.mRenderQueue = renderQueue;
	sprite.mDepth = priorityFlag ? 0.6f : 0.1f;
	sprite.mCoordinatesSpace = space;
	sprite.mLogicalSpace = mLogicalSpriteSpace;
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

		TaggedSpriteData& data = mTaggedSpritesThisFrame[mSpriteTag];
		data.mPosition = mTaggedSpritePosition;
	}
}

void SpriteManager::collectLegacySprites()
{
	// Collect sprites to render
	mCurrSpriteSets.mVdpSprites.clear();

	for (int link = 0;;)
	{
		const uint16* data = (uint16*)&EmulatorInterface::instance().getVRam()[mSpriteAttributeTableBase + link * 2];

		VdpSpriteInfo& sprite = vectorAdd(mCurrSpriteSets.mVdpSprites);
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

		// Update "last used atex" of patterns in cache (actually that's only needed for debug output)
		{
			const int patterns = sprite.mSize.x * sprite.mSize.y;
			for (int k = 0; k < patterns; ++k)
			{
				mPatternManager.setLastUsedAtex(data[2] + k, (data[2] >> 9) & 0x70);
			}
		}

		// Go to next sprite
		link = (data[1] & 0x7f) << 2;
		if ((link == 0) || (link >= 320))
			break;
		if (mCurrSpriteSets.mVdpSprites.size() >= 0x100)	// Sanity check
			break;
	}
}
