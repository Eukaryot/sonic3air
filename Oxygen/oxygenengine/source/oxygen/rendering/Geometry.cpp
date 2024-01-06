/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/rendering/Geometry.h"


PlaneGeometry::PlaneGeometry(const Recti& activeRect, int planeIndex, bool priorityFlag, uint8 scrollOffsets, uint16 renderQueue) :
	Geometry(Type::PLANE),
	mPlaneIndex(planeIndex),
	mPriorityFlag(priorityFlag),
	mActiveRect(activeRect),
	mScrollOffsets(scrollOffsets)
{
	mRenderQueue = renderQueue;
}


SpriteGeometry::SpriteGeometry(const renderitems::SpriteInfo& spriteInfo) :
	Geometry(Type::SPRITE),
	mSpriteInfo(spriteInfo)
{
}
