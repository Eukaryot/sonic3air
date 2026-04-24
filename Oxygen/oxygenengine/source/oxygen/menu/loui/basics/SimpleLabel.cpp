/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/basics/SimpleLabel.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"


namespace loui
{
	SimpleLabel& SimpleLabel::init(const std::string_view text, FontWrapper& font, Vec2i size)
	{
		mText = text;
		mFont = &font;
		mRelativeRect.setSize(size);
		setInteractable(false);
		return *this;
	}

	void SimpleLabel::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);
	}

	void SimpleLabel::render(RenderInfo& renderInfo)
	{
		Widget::render(renderInfo);

		Font* font = (nullptr != mFont) ? mFont->getFont() : nullptr;
		if (nullptr != font)
		{
			renderInfo.mDrawer.printText(*font, mFinalScreenRect + Vec2i(0, 2), mText, 5);
		}
	}
}
