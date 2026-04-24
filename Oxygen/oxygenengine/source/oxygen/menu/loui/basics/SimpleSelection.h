/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/basics/SimpleButton.h"
#include "oxygen/menu/loui/basics/SimpleLabel.h"


namespace loui
{
	class SimpleSelection : public Widget
	{
	public:
		SimpleSelection& init(const std::string_view text, FontWrapper& font, Vec2i size);

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		virtual void applyLayouting() override;

	protected:
		SimpleLabel mTitleLabel;
		SimpleLabel mValueLabel;
		SimpleButton mButtonLeft;
		SimpleButton mButtonRight;

		bool mIsHovered = false;
		int mValue = 1;
	};
}
