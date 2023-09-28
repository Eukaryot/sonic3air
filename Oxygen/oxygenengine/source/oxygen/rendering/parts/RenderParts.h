/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/rendering/parts/OverlayManager.h"
#include "oxygen/rendering/parts/PaletteManager.h"
#include "oxygen/rendering/parts/PatternManager.h"
#include "oxygen/rendering/parts/PlaneManager.h"
#include "oxygen/rendering/parts/ScrollOffsetsManager.h"
#include "oxygen/rendering/parts/SpacesManager.h"
#include "oxygen/rendering/parts/SpriteManager.h"


class RenderParts : public SingleInstance<RenderParts>
{
public:
	struct Viewport
	{
		Recti mRect;
		uint16 mRenderQueue = 0;
	};

public:
	RenderParts();

	inline OverlayManager&		 getOverlayManager()		{ return mOverlayManager; }
	inline PaletteManager&		 getPaletteManager()		{ return mPaletteManager; }
	inline PatternManager&		 getPatternManager()		{ return mPatternManager; }
	inline PlaneManager&		 getPlaneManager()			{ return mPlaneManager; }
	inline ScrollOffsetsManager& getScrollOffsetsManager()	{ return mScrollOffsetsManager; }
	inline SpacesManager&		 getSpacesManager()			{ return mSpacesManager; }
	inline SpriteManager&		 getSpriteManager()			{ return mSpriteManager; }

	inline bool getActiveDisplay() const	   { return mActiveDisplay; }
	inline void setActiveDisplay(bool enable)  { mActiveDisplay = enable; }

	inline bool getEnforceClearScreen() const		{ return mEnforceClearScreen; }
	inline void setEnforceClearScreen(bool enable)	{ mEnforceClearScreen = enable; }

	void addViewport(const Recti& rect, uint16 renderQueue);
	inline const std::vector<Viewport>& getViewports() const  { return mViewports; }

	void reset();
	void preFrameUpdate();
	void postFrameUpdate();
	void refresh(const RefreshParameters& refreshParameters);

	void dumpPatternsContent();
	void dumpPlaneContent(int planeIndex);

public:
	bool mLayerRendering[8];

private:
	OverlayManager		 mOverlayManager;
	PaletteManager		 mPaletteManager;
	PatternManager		 mPatternManager;
	PlaneManager		 mPlaneManager;
	ScrollOffsetsManager mScrollOffsetsManager;
	SpacesManager		 mSpacesManager;
	SpriteManager		 mSpriteManager;

	bool mActiveDisplay = true;
	bool mEnforceClearScreen = false;

	std::vector<Viewport> mViewports;
};
