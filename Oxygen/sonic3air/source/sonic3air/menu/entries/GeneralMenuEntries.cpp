/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/entries/GeneralMenuEntries.h"
#include "sonic3air/menu/SharedResources.h"

#include "oxygen/application/Application.h"


InputFieldMenuEntry::InputFieldMenuEntry()
{
	mMenuEntryType = MENU_ENTRY_TYPE;
}

InputFieldMenuEntry& InputFieldMenuEntry::initEntry(Vec2i size, std::wstring_view defaultText, std::wstring_view placeholderText)
{
	mSize = size;
	mPlaceholderText = placeholderText;
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
	// Require active text input
	Application::instance().requestActiveTextInput();

	const Recti outerRect(renderContext.mCurrentPosition - Vec2i(3, 0), mSize);
	const Recti textRect(outerRect.getPos() + Vec2i(5, 1), outerRect.getSize() - Vec2i(10, 2));

	Drawer& drawer = *renderContext.mDrawer;
	Font& font = global::mOxyfontSmall;
	const int cursorOffset = font.getWidth(mTextInputHandler.getText(), 0, (int)mTextInputHandler.getCursorPosition());

	if (renderContext.mIsSelected)
	{
		drawer.drawRect(outerRect, Color(0.0f, 0.0f, 0.0f, 0.2f));

		drawer.printText(font, textRect, std::wstring_view(mTextInputHandler.getText()).substr(0, mTextInputHandler.getCursorPosition()), 4);
		drawer.printText(font, textRect + Vec2i(cursorOffset+2, 0), std::wstring_view(mTextInputHandler.getText()).substr(mTextInputHandler.getCursorPosition()), 4);
		if (std::fmod(FTX::getTime(), 1.0f) < 0.6f)
		{
			drawer.printText(font, textRect + Vec2i(cursorOffset-2, font.getLineHeight()-2), "^", 4, Color(1.0f, 1.0f, 0.0f));
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
		drawer.drawRect(outerRect, Color(0.0f, 0.0f, 0.0f, 0.1f));

		if (mTextInputHandler.getText().empty())
		{
			drawer.printText(global::mOxyfontSmallNoOutline, textRect, mPlaceholderText, 4, Color(0.4f, 0.6f, 0.6f, 1.0f));
		}
		else
		{
			drawer.printText(font, textRect, mTextInputHandler.getText(), 4);
		}
	}

	renderContext.mCurrentPosition.y += 6;
}
