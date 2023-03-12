/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/SpacesManager.h"
#include "oxygen/resources/SpriteCache.h"
#include "oxygen/helper/Transform2D.h"


class PatternManager;

class SpriteManager
{
public:
	using Space = SpacesManager::Space;

	struct SpriteInfo
	{
	public:
		enum class Type
		{
			INVALID = 0,
			VDP,
			PALETTE,
			COMPONENT,
			MASK
		};

	public:
		inline Type getType() const  { return mType; }

	public:
		Vec2i  mPosition;
		bool   mPriorityFlag = false;
		uint16 mRenderQueue = 0;
		Color  mTintColor = Color::WHITE;
		Color  mAddedColor = Color::TRANSPARENT;
		bool   mUseGlobalComponentTint = true;
		bool   mFullyOpaque = false;
		Space  mCoordinatesSpace = Space::SCREEN;	// The coordinate system that "mPosition" is referring to
		Space  mLogicalSpace = Space::SCREEN;		// The coordinate system used for frame interpolation, can be different from the coordinates space

		// Frame interpolation
		bool   mHasLastPosition = false;
		Vec2i  mLastPositionChange;
		Vec2i  mInterpolatedPosition;

	protected:
		inline SpriteInfo(Type type) : mType(type) {}
		inline ~SpriteInfo() {}

	private:
		const Type mType = Type::INVALID;
	};

	struct VdpSpriteInfo : public SpriteInfo
	{
		inline VdpSpriteInfo() : SpriteInfo(Type::VDP) {}

		Vec2i  mSize;				// In columns / rows of 8 pixels
		uint16 mFirstPattern = 0;	// Incl. flip bits and atex
	};

	struct CustomSpriteInfoBase : public SpriteInfo
	{
		inline CustomSpriteInfoBase(Type type) : SpriteInfo(type) {}

		uint64 mKey = 0;
		const SpriteCache::CacheItem* mCacheItem = nullptr;
		Vec2i mSize;
		Vec2i mPivotOffset;
		bool mFlipX = false;
		bool mFlipY = false;
		Transform2D mTransformation;
		bool mUseUpscaledSprite = false;	// Currently only supported for palette sprites
	};

	struct PaletteSpriteInfo : public CustomSpriteInfoBase
	{
		inline PaletteSpriteInfo() : CustomSpriteInfoBase(Type::PALETTE) {}

		uint16 mAtex = 0;
	};

	struct ComponentSpriteInfo : public CustomSpriteInfoBase
	{
		inline ComponentSpriteInfo() : CustomSpriteInfoBase(Type::COMPONENT) {}
	};

	struct SpriteMaskInfo : public SpriteInfo
	{
		inline SpriteMaskInfo() : SpriteInfo(Type::MASK) {}

		Vec2i mSize;
		float mDepth = 0.0f;
	};

public:
	SpriteManager(PatternManager& patternManager, SpacesManager& spacesManager);

	void reset();
	void resetSprites();
	void preFrameUpdate();
	void postFrameUpdate();
	void refresh();

	void drawVdpSprite(const Vec2i& position, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, const Color& tintColor = Color::WHITE, const Color& addedColor = Color::TRANSPARENT);
	void drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor = Color::WHITE, float angle = 0.0f, float scale = 1.0f);
	void drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, Vec2f scale);
	void drawCustomSpriteWithTransform(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, const Transform2D& transformation);
	void addSpriteMask(const Vec2i& position, const Vec2i& size, uint16 renderQueue, bool priorityFlag, Space space);

	uint32 addSpriteHandle(uint64 key, const Vec2i& position, uint16 renderQueue);
	void setSpriteHandleFlags(uint32 spriteHandle, uint8 flags);
	void setSpriteHandleFlipX(uint32 spriteHandle, bool flipX);
	void setSpriteHandleFlipY(uint32 spriteHandle, bool flipY);
	void setSpriteHandleRotationScale(uint32 spriteHandle, float radians, Vec2f scale);
	void setSpriteHandleTransform(uint32 spriteHandle, const Transform2D& transformation);
	void setSpriteHandlePriorityFlag(uint32 spriteHandle, bool priorityFlag);
	void setSpriteHandleCoordinateSpace(uint32 spriteHandle, Space space);
	void setSpriteHandleUseGlobalComponentTint(uint32 spriteHandle, bool enable);
	void setSpriteHandlePaletteOffset(uint32 spriteHandle, uint16 paletteOffset);
	void setSpriteHandleTintColor(uint32 spriteHandle, Color tintColor);
	void setSpriteHandleOpacity(uint32 spriteHandle, float opacity);
	void setSpriteHandleAddedColor(uint32 spriteHandle, Color addedColor);

	void setLogicalSpriteSpace(Space space);
	void clearSpriteTag();
	void setSpriteTagWithPosition(uint64 spriteTag, const Vec2i& position);

	inline const std::vector<SpriteInfo*>& getSprites() const  { return mSprites; }

	inline uint16 getSpriteAttributeTableBase() const  { return mSpriteAttributeTableBase; }
	inline void setSpriteAttributeTableBase(uint16 vramAddress)  { mSpriteAttributeTableBase = vramAddress; }

public:
	bool mLegacyVdpSpriteMode = false;

private:
	struct SpriteSets
	{
		std::vector<VdpSpriteInfo>		 mVdpSprites;
		std::vector<PaletteSpriteInfo>	 mPaletteSprites;
		std::vector<ComponentSpriteInfo> mComponentSprites;
		std::vector<SpriteMaskInfo>		 mSpriteMasks;
		std::unordered_map<uint32, CustomSpriteInfoBase*> mSpritesByHandle;
		std::pair<uint32, CustomSpriteInfoBase*> mLatestSpritesByHandle;

		void clear();
		void swap(SpriteSets& other);
		CustomSpriteInfoBase* getSpriteByHandle(uint32 spriteHandle) const;
	};

private:
	CustomSpriteInfoBase* addSpriteByKey(uint64 key);
	void applyFlags(CustomSpriteInfoBase& sprite, uint8 flags) const;
	void checkSpriteTag(SpriteInfo& sprite);
	void collectLegacySprites();

private:
	PatternManager& mPatternManager;
	SpacesManager& mSpacesManager;

	bool mResetSprites = false;
	Space mLogicalSpriteSpace = Space::SCREEN;

	SpriteSets mCurrSpriteSets;
	SpriteSets mNextSpriteSets;
	std::vector<SpriteInfo*> mSprites;
	uint32 mNextSpriteHandle = 1;

	uint16 mSpriteAttributeTableBase = 0xf800;	// Only used in legacy VPD sprite mode

	struct TaggedSpriteData
	{
		Vec2i mPosition;
	};
	uint64 mSpriteTag = 0;
	Vec2i mTaggedSpritePosition;
	std::unordered_map<uint64, TaggedSpriteData> mTaggedSpritesLastFrame;
	std::unordered_map<uint64, TaggedSpriteData> mTaggedSpritesThisFrame;
};
