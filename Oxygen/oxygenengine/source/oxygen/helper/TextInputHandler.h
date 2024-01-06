/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>
#include <optional>


class TextInputHandler
{
public:
	inline const std::wstring& getText() const   { return mText; }
	void setText(std::wstring_view text, bool moveCursorToEnd = false);

	inline size_t getCursorPosition() const  { return mCursorPosition; }
	void setCursorPosition(size_t position);
	const std::optional<size_t>& getMarkedRangeStart() const  { return mMarkedRangeStart; }

	void keyboard(const rmx::KeyboardEvent& ev);
	void textinput(const rmx::TextInputEvent& ev);

private:
	void insertText(std::wstring_view text);
	void moveCursorTo(size_t position, bool considerShift = true);
	void deleteMarkedRange();

private:
	std::wstring mText;
	size_t mCursorPosition = 0;
	std::optional<size_t> mMarkedRangeStart;
};
