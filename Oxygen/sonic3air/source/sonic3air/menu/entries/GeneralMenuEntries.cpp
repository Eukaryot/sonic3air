/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/entries/GeneralMenuEntries.h"
#include "sonic3air/menu/SharedResources.h"


InputFieldMenuEntry::InputFieldMenuEntry()
{
	mMenuEntryType = MENU_ENTRY_TYPE;
}

InputFieldMenuEntry& InputFieldMenuEntry::initEntry(std::wstring_view defaultText)
{
	mTextInputHandler.setText(defaultText, true);
	return *this;
}

void InputFieldMenuEntry::keyboard(const rmx::KeyboardEvent& ev)
{
	mTextInputHandler.keyboard(ev);
}

void InputFieldMenuEntry::textinput(const rmx::TextInputEvent& ev)
{
	mTextInputHandler.textinput(ev);
}

void InputFieldMenuEntry::renderEntry(RenderContext& renderContext)
{
	const Recti textRect(renderContext.mCurrentPosition, Vec2i(200, 11));
	Drawer& drawer = *renderContext.mDrawer;
	Font& font = global::mOxyfontSmall;
	const int cursorOffset = font.getWidth(mTextInputHandler.getText(), 0, (int)mTextInputHandler.getCursorPosition());

	if (renderContext.mIsSelected)
	{
		drawer.printText(font, textRect, std::wstring_view(mTextInputHandler.getText()).substr(0, mTextInputHandler.getCursorPosition()));
		drawer.printText(font, textRect + Vec2i(cursorOffset+2, 0), std::wstring_view(mTextInputHandler.getText()).substr(mTextInputHandler.getCursorPosition()));
		if (std::fmod(FTX::getTime(), 1.0f) < 0.6f)
		{
			drawer.printText(font, textRect + Vec2i(cursorOffset-2, font.getLineHeight()-2), "^", 1, Color(1.0f, 1.0f, 0.0f));
		}

		if (mTextInputHandler.getMarkedRangeStart().has_value())
		{
			const int endOffset = font.getWidth(mTextInputHandler.getText(), 0, (int)*mTextInputHandler.getMarkedRangeStart());
			Recti rect(textRect.x + cursorOffset, textRect.y - 2, endOffset - cursorOffset, textRect.height);
			if (rect.width < 0)
			{
				rect.width = -rect.width + 1;
				rect.x -= rect.width;
			}
			else
			{
				rect.x += 3;
				rect.width += 1;
			}
			drawer.drawRect(rect, Color(1.0f, 1.0f, 0.0f, 0.6f));
		}
	}
	else
	{
		drawer.printText(font, textRect, mTextInputHandler.getText());
	}

	renderContext.mCurrentPosition.y += 14;
}
