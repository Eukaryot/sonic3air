/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/helper/Transform2D.h"
#include "oxygen/rendering/parts/SpacesManager.h"
#include "oxygen/resources/SpriteCollection.h"

class PaletteBase;


struct RenderItem
{
public:
	using Space = SpacesManager::Space;

	enum class Type
	{
		INVALID = 0,
		VDP_SPRITE,
		PALETTE_SPRITE,
		COMPONENT_SPRITE,
		SPRITE_MASK,
		RECTANGLE,
		TEXT,
		VIEWPORT
	};

	enum class LifetimeContext : uint8
	{
		DEFAULT = 0,		// Default context for rendering inside frame simulation
		CUSTOM_1 = 1,		// Custom usage by scripts
		CUSTOM_2 = 2,		// Custom usage by scripts
		OUTSIDE_FRAME = 3,	// Debug output rendered outside of frame simulation
	};
	static const uint8 NUM_LIFETIME_CONTEXTS = 4;

public:
	inline Type getType() const   { return mRenderItemType; }
	inline bool isSprite() const  { return (mRenderItemType >= Type::VDP_SPRITE && mRenderItemType <= Type::COMPONENT_SPRITE); }

	virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion);

public:
	Vec2i mPosition;
	uint16 mRenderQueue = 0;
	SpacesManager::Space mCoordinatesSpace = SpacesManager::Space::WORLD;	// The coordinate system that the render item's position / rect is referring to
	bool mUseGlobalComponentTint = true;
	LifetimeContext mLifetimeContext = LifetimeContext::DEFAULT;

protected:
	inline RenderItem(Type type) : mRenderItemType(type) {}

private:
	const Type mRenderItemType = Type::INVALID;
};


namespace renderitems
{

	struct SpriteInfo : public RenderItem
	{
	public:
		bool  mPriorityFlag = false;
		Color mTintColor = Color::WHITE;
		Color mAddedColor = Color::TRANSPARENT;
		BlendMode mBlendMode = BlendMode::ALPHA;
		Space mLogicalSpace = Space::SCREEN;		// The coordinate system used for frame interpolation, can be different from the coordinates space

		// Frame interpolation
		bool  mHasLastPosition = false;
		Vec2i mLastPositionChange;
		Vec2i mInterpolatedPosition;

	protected:
		inline SpriteInfo(Type type) : RenderItem(type) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;
	};

	struct VdpSpriteInfo : public SpriteInfo
	{
		inline VdpSpriteInfo() : SpriteInfo(Type::VDP_SPRITE) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		Vec2i  mSize;				// In columns / rows of 8 pixels
		uint16 mFirstPattern = 0;	// Incl. flip bits and atex
	};

	struct CustomSpriteInfoBase : public SpriteInfo
	{
		inline CustomSpriteInfoBase(Type type) : SpriteInfo(type) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		uint64 mKey = 0;
		const SpriteCollection::Item* mCacheItem = nullptr;
		Vec2i mSize;
		Vec2i mPivotOffset;
		Transform2D mTransformation;
		bool mUseUpscaledSprite = false;	// Currently only supported for palette sprites
	};

	struct PaletteSpriteInfo : public CustomSpriteInfoBase
	{
		inline PaletteSpriteInfo() : CustomSpriteInfoBase(Type::PALETTE_SPRITE) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		const PaletteBase* mPrimaryPalette = nullptr;
		const PaletteBase* mSecondaryPalette = nullptr;
		uint16 mAtex = 0;
	};

	struct ComponentSpriteInfo : public CustomSpriteInfoBase
	{
		inline ComponentSpriteInfo() : CustomSpriteInfoBase(Type::COMPONENT_SPRITE) {}
	};

	struct SpriteMaskInfo : public SpriteInfo
	{
		inline SpriteMaskInfo() : SpriteInfo(Type::SPRITE_MASK) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		Vec2i mSize;
		float mDepth = 0.0f;
	};

	struct Rectangle : public RenderItem
	{
		inline Rectangle() : RenderItem(Type::RECTANGLE) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		Vec2i mSize;
		Color mColor;
	};

	struct Text : public RenderItem
	{
		inline Text() : RenderItem(Type::TEXT) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		std::string mFontKeyString;
		uint64 mFontKeyHash = 0;
		std::string mTextString;
		uint64 mTextHash = 0;
		Color mColor;
		int mAlignment = 1;
		int mSpacing = 0;
	};

	struct Viewport : public RenderItem
	{
		inline Viewport() : RenderItem(Type::VIEWPORT) {}
		virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) override;

		Vec2i mSize;
	};

}


struct PoolOfRenderItems
{
	RenderItem& create(RenderItem::Type type);
	void destroy(RenderItem& renderItem);

	ObjectPool<renderitems::VdpSpriteInfo>		 mVdpSprites;
	ObjectPool<renderitems::PaletteSpriteInfo>	 mPaletteSprites;
	ObjectPool<renderitems::ComponentSpriteInfo> mComponentSprites;
	ObjectPool<renderitems::SpriteMaskInfo>		 mSpriteMasks;
	ObjectPool<renderitems::Rectangle>			 mRectangles;
	ObjectPool<renderitems::Text>				 mTexts;
	ObjectPool<renderitems::Viewport>			 mViewports;
};
