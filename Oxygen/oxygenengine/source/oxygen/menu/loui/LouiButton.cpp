/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"


namespace loui
{
	Button& Button::init(const std::string_view text, FontWrapper& font, Vec2i size, bool repeatable)
	{
		mText = text;
		mFont = &font;
		mRelativeRect.setSize(size);
		mRepeatable = repeatable;
		return *this;
	}

	void Button::setText(std::string_view text)
	{
		mText = text;
	}

	void Button::update(UpdateInfo& updateInfo)
	{
		Widget::update(updateInfo);

		mWasPressed = false;
		if (!isInteractable())
		{
			mButtonState = ButtonState::NORMAL;
			mButtonDown = false;
		}
		else
		{
			if (!updateInfo.mMousePosConsumed)
			{
				const bool hovered = mFinalRect.contains(updateInfo.mMousePosition);
				if (mRepeatable)
				{
					if (hovered && updateInfo.mLeftMouseButton.justPressed())
					{
						// Mouse button pressed
						mWasPressed = true;
						mButtonDown = true;
					}
					else if (hovered && mButtonDown && updateInfo.mLeftMouseButton.justPressedOrRepeat())
					{
						// Mouse button repeat
						mWasPressed = true;
					}
					else if (mButtonDown && !updateInfo.mLeftMouseButton.isPressed())
					{
						// Mouse button released
						mButtonDown = false;
					}
				}
				else
				{
					if (hovered && updateInfo.mLeftMouseButton.justPressed())
					{
						// Mouse button pressed
						mButtonDown = true;
					}
					else if (mButtonDown && !updateInfo.mLeftMouseButton.isPressed())
					{
						// Mouse button released
						mWasPressed = hovered;
						mButtonDown = false;
					}
				}

				if (hovered)
				{
					mButtonState = mButtonDown ? ButtonState::PRESSED : ButtonState::HOVERED;
					updateInfo.mMousePosConsumed = true;
					updateInfo.mLeftMouseButton.consume();
				}
				else
				{
					mButtonState = ButtonState::NORMAL;
				}
			}

			// Evaluate keyboard / controller button press
			if (hasFocus())
			{
				const bool pressedByButton = mRepeatable ? updateInfo.mButtonA.justPressedOrRepeat() : updateInfo.mButtonA.justPressed();
				if (pressedByButton)
				{
					mWasPressed = true;
					// TODO: Add visual feedback
				}
			}
		}
	}

	void Button::render(RenderInfo& renderInfo)
	{
		Widget::render(renderInfo);

		Color color;
		switch (mButtonState)
		{
			default:
			case ButtonState::NORMAL:
			{
				if (hasFocus() && renderInfo.mShowFocus)
					color.set(0.8f, 0.8f, 0.5f, 0.5f);
				else
					color.set(0.3f, 0.5f, 0.7f, 0.5f);
				break;
			}

			case ButtonState::HOVERED:	color.set(0.6f, 0.7f, 0.8f, 0.5f);  break;
			case ButtonState::PRESSED:	color.set(0.2f, 0.4f, 0.6f, 0.5f);  break;
		}

		renderInfo.mDrawer.drawRect(mFinalRect, color);

		Font* font = (nullptr != mFont) ? mFont->getFont() : nullptr;
		if (nullptr != font)
		{
			renderInfo.mDrawer.printText(*font, mFinalRect + Vec2i(0, 1), mText, 5);
		}
	}
}
