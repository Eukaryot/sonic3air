/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiWidget.h"

class Font;


namespace loui
{
	class Button : public Widget
	{
	public:
		enum class ButtonState
		{
			NORMAL,
			HOVERED,
			PRESSED,
		};

	public:
		Button& init(const std::string_view text, FontWrapper& font, Vec2i size, bool repeatable = false);

		inline bool wasPressed() const  { return mWasPressed; }

		void setText(std::string_view text);

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		std::string mText;
		FontWrapper* mFont = nullptr;
		bool mRepeatable = false;

		ButtonState mButtonState = ButtonState::NORMAL;
		bool mButtonDown = false;
		bool mWasPressed = false;
	};
}
