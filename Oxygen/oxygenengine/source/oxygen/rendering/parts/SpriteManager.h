/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/RenderItem.h"
#include <optional>


class PatternManager;

class SpriteManager
{
public:
	using Space = SpacesManager::Space;

	struct SpriteHandleData
	{
		uint32 mHandle = 0;		// Sprite handle ID
		uint64 mKey = 0;		// Sprite key
		Vec2i  mPosition;
		uint16 mRenderQueue = 0;
		bool   mFlipX = false;
		bool   mFlipY = false;
		bool   mPriorityFlag = false;
		Color  mTintColor = Color::WHITE;
		Color  mAddedColor = Color::TRANSPARENT;
		std::optional<bool> mUseGlobalComponentTint;
		BlendMode mBlendMode = BlendMode::ALPHA;
		Space  mCoordinatesSpace = Space::SCREEN;	// The coordinate system that "mPosition" is referring to
		Transform2D mTransformation;
		float  mRotation = 0.0f;
		Vec2f  mScale = Vec2f(1.0f, 1.0f);
		bool   mUseUpscaledSprite = false;			// Only supported for palette sprites
		uint64 mPrimaryPaletteKey = 0;				// Only supported for palette sprites
		uint64 mSecondaryPaletteKey = 0;			// Only supported for palette sprites
		uint16 mAtex = 0;							// Only supported for palette sprites
		uint64 mSpriteTag = 0;
		Vec2i  mTaggedSpritePosition;
	};

public:
	SpriteManager(PatternManager& patternManager, SpacesManager& spacesManager);
	inline ~SpriteManager()  { clear(); }

	void clear();
	void clearLifetimeContext(RenderItem::LifetimeContext lifetimeContext);

	void preFrameUpdate();
	void postFrameUpdate();
	void postRefreshDebugging();

	inline void setResetRenderItems(bool reset)  { mResetRenderItems = reset; }

	void drawVdpSprite(const Vec2i& position, uint8 encodedSize, uint16 patternIndex, uint16 renderQueue, const Color& tintColor = Color::WHITE, const Color& addedColor = Color::TRANSPARENT);
	void drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor = Color::WHITE, float angle = 0.0f, float scale = 1.0f);
	void drawCustomSprite(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, float angle, Vec2f scale);
	void drawCustomSpriteWithTransform(uint64 key, const Vec2i& position, uint16 atex, uint8 flags, uint16 renderQueue, const Color& tintColor, const Transform2D& transformation);
	void addSpriteMask(const Vec2i& position, const Vec2i& size, uint16 renderQueue, bool priorityFlag, Space space);

	void addRectangle(const Recti& rect, const Color& color, uint16 renderQueue, Space space, bool useGlobalComponentTint);
	void addText(std::string_view fontKeyString, uint64 fontKeyHash, const Vec2i& position, std::string_view textString, uint64 textHash, const Color& color, int alignment, int spacing, uint16 renderQueue, Space space, bool useGlobalComponentTint);
	void addViewport(const Recti& rect, uint16 renderQueue);

	uint32 addSpriteHandle(uint64 key, const Vec2i& position, uint16 renderQueue);
	SpriteHandleData* getSpriteHandleData(uint32 spriteHandle);

	void setLogicalSpriteSpace(Space space);
	void clearSpriteTag();
	void setSpriteTagWithPosition(uint64 spriteTag, const Vec2i& position);

	inline const std::vector<RenderItem*>& getRenderItems(RenderItem::LifetimeContext context) const  { return mContexts[(int)context].mItems; }
	inline const std::vector<RenderItem*>& getAddedItems() const  { return mAddedItems.mItems; }

	inline uint16 getSpriteAttributeTableBase() const  { return mSpriteAttributeTableBase; }
	inline void setSpriteAttributeTableBase(uint16 vramAddress)  { mSpriteAttributeTableBase = vramAddress; }

	void serializeSaveState(VectorBinarySerializer& serializer, uint8 formatVersion);

public:
	bool mLegacyVdpSpriteMode = false;

private:
	struct ItemSet
	{
		std::vector<RenderItem*> mItems;
	};

private:
	void clearItemSet(ItemSet& itemSet);
	void clearAllContexts();
	ItemSet& getItemsByContext(RenderItem::LifetimeContext lifetimeContext);

	renderitems::CustomSpriteInfoBase* addSpriteByKey(uint64 key);
	void checkSpriteTag(renderitems::SpriteInfo& sprite);

	void processSpriteHandles();
	void grabAddedItems();
	void collectLegacySprites();

private:
	PatternManager& mPatternManager;
	SpacesManager& mSpacesManager;

	RenderItem::LifetimeContext mCurrentContext = RenderItem::LifetimeContext::OUTSIDE_FRAME;
	Space mLogicalSpriteSpace = Space::SCREEN;
	bool mResetRenderItems = false;
	uint16 mSpriteAttributeTableBase = 0xf800;	// Only used in legacy VDP sprite mode

	PoolOfRenderItems mPoolOfRenderItems;

	ItemSet mContexts[RenderItem::NUM_CONTEXTS];
	ItemSet mAddedItems;

	uint32 mNextSpriteHandle = 1;
	std::vector<SpriteHandleData> mSpriteHandles;
	std::pair<uint32, SpriteHandleData*> mLatestSpriteHandle;

	struct TaggedSpriteData
	{
		Vec2i mPosition;
	};
	uint64 mSpriteTag = 0;
	Vec2i mTaggedSpritePosition;
	std::unordered_map<uint64, TaggedSpriteData> mTaggedSpritesLastFrame;
	std::unordered_map<uint64, TaggedSpriteData> mTaggedSpritesThisFrame;
};
