/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/SpacesManager.h"


struct RenderItem
{
public:
	enum class Type
	{
		INVALID = 0,
		VDP_SPRITE,
		PALETTE_SPRITE,
		COMPONENT_SPRITE,
		SPRITE_MASK,
		RECTANGLE,
		TEXT
	};

	enum class LifetimeContext : uint8
	{
		DEFAULT = 0,		// Default context for rendering inside frame simulation
		CUSTOM_1 = 1,		// Custom usage by scripts
		CUSTOM_2 = 2,		// Custom usage by scripts
		OUTSIDE_FRAME = 3,	// Debug output rendered outside of frame simulation
	};
	static const uint8 NUM_CONTEXTS = 4;
	
public:
	inline Type getType() const   { return mRenderItemType; }
	inline bool isSprite() const  { return (mRenderItemType >= Type::VDP_SPRITE && mRenderItemType <= Type::COMPONENT_SPRITE); }

	inline virtual void serialize(VectorBinarySerializer& serializer, uint8 formatVersion) {}

public:
	uint16 mRenderQueue = 0;
	SpacesManager::Space mCoordinatesSpace = SpacesManager::Space::WORLD;	// The coordinate system that the render item's position / rect is referring to
	bool mUseGlobalComponentTint = true;
	LifetimeContext mLifetimeContext = LifetimeContext::DEFAULT;

protected:
	inline RenderItem(Type type) : mRenderItemType(type) {}

private:
	const Type mRenderItemType = Type::INVALID;
};
