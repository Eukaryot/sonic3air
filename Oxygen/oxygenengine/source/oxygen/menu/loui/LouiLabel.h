/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiWidget.h"


namespace loui
{
	class Label : public Widget
	{
	public:
		Label& init(const std::string_view text, FontWrapper& font, Vec2i size);

		void setText(std::string_view text);

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		std::string mText;
		FontWrapper* mFont = nullptr;
	};
}
