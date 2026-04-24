/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/basics/SimpleButton.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"


namespace loui
{
	SimpleButton& SimpleButton::init(const std::string_view text, FontWrapper& font, Vec2i size)
	{
		mText = text;
		mFont = &font;
		mRelativeRect.setSize(size);
		return *this;
	}

	void SimpleButton::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		if (!updateInfo.mMousePosConsumed && mFinalScreenRect.contains(updateInfo.mMousePosition))
		{
			if (updateInfo.mLeftMouseButton.isPressed())
			{
				mButtonState = ButtonState::PRESSED;
			}
			else
			{
				mButtonState = ButtonState::HOVERED;
			}
		}
		else
		{
			mButtonState = ButtonState::NORMAL;
		}
	}

	void SimpleButton::render(RenderInfo& renderInfo)
	{
		Widget::render(renderInfo);

		Color color;
		switch (mButtonState)
		{
			default:
			case ButtonState::NORMAL:
			{
				if (mIsSelected)
					color.set(0.8f, 0.8f, 0.5f, 0.5f);
				else
					color.set(0.3f, 0.5f, 0.7f, 0.5f);
				break;
			}

			case ButtonState::HOVERED:	color.set(0.6f, 0.7f, 0.8f, 0.5f);  break;
			case ButtonState::PRESSED:	color.set(0.2f, 0.4f, 0.6f, 0.5f);  break;
		}

		renderInfo.mDrawer.drawRect(mFinalScreenRect, color);

		Font* font = (nullptr != mFont) ? mFont->getFont() : nullptr;
		if (nullptr != font)
		{
			renderInfo.mDrawer.printText(*font, mFinalScreenRect + Vec2i(0, 2), mText, 5);
		}
	}
}
