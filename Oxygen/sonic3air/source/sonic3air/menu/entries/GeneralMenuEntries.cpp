/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
	Font& font = global::mOxyfontSmall;
	const int cursorOffset = font.getWidth(mTextInputHandler.getText(), 0, mTextInputHandler.getCursorPosition());

	if (renderContext.mIsSelected)
	{
		renderContext.mDrawer->printText(font, textRect, std::wstring_view(mTextInputHandler.getText()).substr(0, mTextInputHandler.getCursorPosition()));
		renderContext.mDrawer->printText(font, textRect + Vec2i(cursorOffset+4, 0), std::wstring_view(mTextInputHandler.getText()).substr(mTextInputHandler.getCursorPosition()));
		if (std::fmod(FTX::getTime(), 1.0f) < 0.6f)
		{
			renderContext.mDrawer->drawRect(Recti(textRect.x + cursorOffset, textRect.y - 2, 3, textRect.height), Color::BLACK);
			renderContext.mDrawer->drawRect(Recti(textRect.x + cursorOffset + 1, textRect.y - 1, 1, textRect.height - 2), Color::WHITE);
		}

		if (mTextInputHandler.getMarkedRangeStart().has_value())
		{
			const int endOffset = font.getWidth(mTextInputHandler.getText(), 0, *mTextInputHandler.getMarkedRangeStart());
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
			renderContext.mDrawer->drawRect(rect, Color(1.0f, 1.0f, 0.0f, 0.6f));
		}
	}
	else
	{
		renderContext.mDrawer->printText(font, textRect, mTextInputHandler.getText());
	}

	renderContext.mCurrentPosition.y += 14;
}
