/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"

#include "oxygen/helper/TextInputHandler.h"


class InputFieldMenuEntry : public GameMenuEntry
{
public:
	static const constexpr uint32 MENU_ENTRY_TYPE = rmx::compileTimeFNV_32("InputFieldMenuEntry");

public:
	InputFieldMenuEntry();
	InputFieldMenuEntry& initEntry(Vec2i size, std::wstring_view defaultText, std::wstring_view placeholderText);

	void keyboard(const rmx::KeyboardEvent& ev) override;
	void textinput(const rmx::TextInputEvent& ev) override;
	void renderEntry(RenderContext& renderContext) override;

private:
	Vec2i mSize;
	std::wstring mPlaceholderText;

	TextInputHandler mTextInputHandler;
};
