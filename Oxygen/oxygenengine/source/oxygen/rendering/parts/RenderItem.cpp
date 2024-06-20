/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/parts/RenderItem.h"
#include "oxygen/simulation/EmulatorInterface.h"


void RenderItem::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	serializer.serializeAs<int16>(mPosition.x);
	serializer.serializeAs<int16>(mPosition.y);
	serializer.serialize(mRenderQueue);
	serializer.serializeAs<uint8>(mCoordinatesSpace);
	serializer.serialize(mUseGlobalComponentTint);
	serializer.serializeAs<uint8>(mLifetimeContext);
}


void renderitems::SpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	RenderItem::serialize(serializer, formatVersion);

	serializer.serializeAs<uint8>(mPriorityFlag);
	mTintColor.serialize(serializer);
	mAddedColor.serialize(serializer);
	serializer.serializeAs<uint8>(mBlendMode);
	serializer.serializeAs<uint8>(mLogicalSpace);
}

void renderitems::VdpSpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	SpriteInfo::serialize(serializer, formatVersion);

	serializer.serializeAs<uint8>(mSize.x);
	serializer.serializeAs<uint8>(mSize.y);
	serializer.serialize(mFirstPattern);
}

void renderitems::CustomSpriteInfoBase::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	SpriteInfo::serialize(serializer, formatVersion);

	serializer.serialize(mKey);
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

	if (formatVersion >= 5)
	{
		if (serializer.isReading())
		{
			bool isROMBased = serializer.read<bool>();
			if (isROMBased && nullptr == mCacheItem)
			{
				SpriteCache::ROMSpriteData romSpriteData;
				romSpriteData.serialize(serializer);
				mCacheItem = &SpriteCache::instance().setupSpriteFromROM(EmulatorInterface::instance(), romSpriteData, 0x00);
			}
			else
			{
				mCacheItem = SpriteCache::instance().getSprite(mKey);
			}
		}
		else
		{
			const bool isROMBased = (nullptr != mCacheItem && mCacheItem->mSourceInfo.mType == SpriteCache::SourceInfo::Type::ROM_DATA);
			serializer.write(isROMBased);
			if (isROMBased)
			{
				// TODO: Avoid the const_cast here
				const_cast<SpriteCache::ROMSpriteData&>(mCacheItem->mSourceInfo.mROMSpriteData).serialize(serializer);
			}
		}
	}
	else
	{
		if (serializer.isReading())
		{
			mCacheItem = SpriteCache::instance().getSprite(mKey);
		}
	}
}

void renderitems::PaletteSpriteInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	CustomSpriteInfoBase::serialize(serializer, formatVersion);

	serializer.serialize(mAtex);
}

void renderitems::SpriteMaskInfo::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	SpriteInfo::serialize(serializer, formatVersion);

	serializer.serializeAs<uint16>(mSize.x);
	serializer.serializeAs<uint16>(mSize.y);
	serializer.serialize(mDepth);
}

void renderitems::Rectangle::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	RenderItem::serialize(serializer, formatVersion);

	serializer.serializeAs<uint16>(mSize.x);
	serializer.serializeAs<uint16>(mSize.y);
	mColor.serialize(serializer);
}

void renderitems::Text::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	RenderItem::serialize(serializer, formatVersion);

	serializer.serialize(mFontKeyString);
	serializer.serialize(mTextString);
	mColor.serialize(serializer);
	serializer.serializeAs<int8>(mAlignment);
	serializer.serializeAs<int16>(mSpacing);

	if (serializer.isReading())
	{
		mFontKeyHash = rmx::getMurmur2_64(mFontKeyString);
		mTextHash = rmx::getMurmur2_64(mTextString);
	}
}

void renderitems::Viewport::serialize(VectorBinarySerializer& serializer, uint8 formatVersion)
{
	RenderItem::serialize(serializer, formatVersion);

	serializer.serializeAs<uint16>(mSize.x);
	serializer.serializeAs<uint16>(mSize.y);
}


RenderItem& PoolOfRenderItems::create(RenderItem::Type type)
{
	switch (type)
	{
		case RenderItem::Type::VDP_SPRITE:		 return mVdpSprites.createObject();
		case RenderItem::Type::PALETTE_SPRITE:	 return mPaletteSprites.createObject();
		case RenderItem::Type::COMPONENT_SPRITE: return mComponentSprites.createObject();
		case RenderItem::Type::SPRITE_MASK:		 return mSpriteMasks.createObject();
		case RenderItem::Type::RECTANGLE:		 return mRectangles.createObject();
		case RenderItem::Type::TEXT:			 return mTexts.createObject();
		case RenderItem::Type::VIEWPORT:		 return mViewports.createObject();
		default:
			RMX_ASSERT(false, "Trying to create unsupported render item type");
			return mVdpSprites.createObject();
	}
}

void PoolOfRenderItems::destroy(RenderItem& renderItem)
{
	switch (renderItem.getType())
	{
		case RenderItem::Type::VDP_SPRITE:		 mVdpSprites.destroyObject(static_cast<renderitems::VdpSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::PALETTE_SPRITE:	 mPaletteSprites.destroyObject(static_cast<renderitems::PaletteSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::COMPONENT_SPRITE: mComponentSprites.destroyObject(static_cast<renderitems::ComponentSpriteInfo&>(renderItem));  break;
		case RenderItem::Type::SPRITE_MASK:		 mSpriteMasks.destroyObject(static_cast<renderitems::SpriteMaskInfo&>(renderItem));  break;
		case RenderItem::Type::RECTANGLE:		 mRectangles.destroyObject(static_cast<renderitems::Rectangle&>(renderItem));  break;
		case RenderItem::Type::TEXT:			 mTexts.destroyObject(static_cast<renderitems::Text&>(renderItem));  break;
		case RenderItem::Type::VIEWPORT:		 mViewports.destroyObject(static_cast<renderitems::Viewport&>(renderItem));  break;
		default:
			RMX_ASSERT(false, "Trying to destroy unsupported render item type");
			break;
	}
}
